// overlay_compile.cpp — see overlay_compile.h.

#include "overlay_compile.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>

#include "overlay_emit.h"   // emit_overlay_c
#include "../gba/crc32.h"   // gba::crc32

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

// Baked at configure time (target_compile_definitions): the gbarecomp source
// root, so the runtime compiler can find the overlay shim headers
// (overlay_runtime_arm.h / overlay_abi.h in src/runtime, runtime_arm_types.h in
// src/armv4t) without any source-tree discovery at runtime. Per the plan's
// scope, a portable/bundled toolchain + header embedding is Stage-2b deferred.
#ifndef GBARECOMP_SRC_DIR
#  define GBARECOMP_SRC_DIR "."
#endif

namespace fs = std::filesystem;

namespace gbarecomp {

namespace {

// Directory of the running executable, for locating release-bundled assets (the
// overlay_toolchain/ the packager stages next to the exe). "" if unresolved.
std::string exe_dir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, sizeof(buf));
    if (n > 0 && n < sizeof(buf)) return fs::path(buf).parent_path().string();
#endif
    return "";
}

// True under GBARECOMP_HEAL_BACKEND=auto-no-gcc: simulate a shipped, source-less,
// gcc-less player box ON a dev machine — force the bundled tcc + bundled include
// even though the dev source + gcc are present. Mirrors psxrecomp's
// OVERLAY_BACKEND_AUTO_NO_GCC. (resolve_backend() maps the same value to tcc.)
bool heal_simulate_shipped() {
    const char* be = std::getenv("GBARECOMP_HEAL_BACKEND");
    return be && std::strcmp(be, "auto-no-gcc") == 0;
}

// The C++ compiler used to build overlay shared objects. GBARECOMP_HEAL_CXX
// overrides for non-default installs.
//
// The default used to be the Windows dev box's absolute msys2 path
// unconditionally, so on macOS and Linux every heal attempt died with
//
//     sh: C:/msys64/mingw64/bin/g++.exe: No such file or directory
//
// which surfaces as `gcc exit 32512` (127 << 8, "command not found") and leaves
// the session permanently on the interpreter bridge: coverage can never reach
// FULLY STATIC off Windows, however many misses are merged into the config.
// Prefer a bare compiler name elsewhere and let PATH resolve it.
std::string gxx_path() {
    if (const char* e = std::getenv("GBARECOMP_HEAL_CXX")) {
        if (e[0]) return e;
    }
#if defined(_WIN32)
    return "C:/msys64/mingw64/bin/g++.exe";
#else
    return "c++";
#endif
}

// The bundled, toolchain-free C compiler used to build overlay DLLs on a player
// box with no g++. GBARECOMP_HEAL_TCC overrides; otherwise prefer the tcc the
// release packager staged next to the exe (<exe_dir>/overlay_toolchain/tcc/
// tcc.exe), falling back to a `tcc` on PATH for a dev box that has one.
std::string tcc_path() {
    if (const char* e = std::getenv("GBARECOMP_HEAL_TCC")) {
        if (e[0]) return e;
    }
    const std::string ed = exe_dir();
    if (!ed.empty()) {
        fs::path cand = fs::path(ed) / "overlay_toolchain" / "tcc" / "tcc.exe";
        std::error_code ec;
        if (fs::exists(cand, ec)) return cand.string();
    }
    return "tcc";
}

// Include flags for compiling an overlay's emitted C. On a dev box the baked
// GBARECOMP_SRC_DIR points at the engine source (shim headers in src/runtime +
// src/armv4t). On a SHIPPED, source-less box those don't exist, so fall back to
// the headers the release packager flattened into <exe>/overlay_toolchain/
// include (beside the bundled tcc). Used by BOTH gcc and tcc, so the gcc shipped
// path is fixed too. Returns a leading-space-prefixed flag string.
std::string overlay_include_flags() {
    const std::string ed = exe_dir();
    const std::string bundled =
        ed.empty() ? std::string()
                   : " -I\"" + (fs::path(ed) / "overlay_toolchain" / "include")
                                   .generic_string() + "\"";
    // Shipped simulation: ignore the dev source, use the bundled headers.
    if (heal_simulate_shipped() && !bundled.empty()) return bundled;

    const std::string src = GBARECOMP_SRC_DIR;
    std::error_code ec;
    if (fs::exists(fs::path(src) / "src" / "runtime" / "overlay_runtime_arm.h", ec))
        return " -I\"" + src + "/src/runtime\" -I\"" + src + "/src/armv4t\"";
    if (!bundled.empty()) return bundled;
    return " -I\"" + src + "/src/runtime\" -I\"" + src + "/src/armv4t\"";  // last resort
}

#ifdef _WIN32
// Build a child environment block for the compiler child. The g++ driver (in a
// MinGW bin dir) spawns cc1plus, which loads libmpc-3.dll / libisl-23.dll /
// libmpfr-6.dll that live beside g++.exe. If that dir is off the inherited
// PATH (e.g. the host PATH points only at llvm-mingw / WinLibs gcc), cc1plus
// dies with "libmpc-3.dll not found" and every heal fails. Prepend the
// compiler's own dir to PATH in the child so its runtime DLLs always resolve.
// Returns a well-formed doubly-NUL-terminated block, empty if it cannot be
// built (callers then inherit).
static std::vector<char> build_child_env(const std::string& extra_bin_dir) {
    std::vector<char> out;
    if (extra_bin_dir.empty()) return out;

    std::string prepend = extra_bin_dir;
    if (!prepend.empty() && (prepend.back() == '/' || prepend.back() == '\\'))
        prepend.pop_back();

    // First pass: emit every "=X:..." drive/meta entry (must lead the block).
    std::vector<std::string> body;      // normal VAR=value entries
    std::string path_value;             // original PATH value, if any
    bool path_seen = false;
    {
        wchar_t* env = GetEnvironmentStringsW();
        if (!env) return out;
        for (const wchar_t* p = env; *p; p += wcslen(p) + 1) {
            const std::wstring entry(p);   // wide entry ("=C:=C:\\", "PATH=...", ...)
            // Narrow the wide entry safely. Converting via iterators into
            // std::string wchar_t -> char (basic_string::_M_construct) reads
            // wchar_t*s as if they were char* and can throw length_error /
            // walk off the buffer. Use the canonical wide->ANSI API instead.
            int need = WideCharToMultiByte(CP_ACP, 0, entry.data(),
                                           static_cast<int>(entry.size()),
                                           nullptr, 0, nullptr, nullptr);
            std::string el(static_cast<std::size_t>(need > 0 ? need : 0), '\0');
            if (need > 0)
                WideCharToMultiByte(CP_ACP, 0, entry.c_str(),
                                    static_cast<int>(entry.size()), &el[0],
                                    need, nullptr, nullptr);
            if (entry.size() && entry[0] == L'=') {  // "=C:=C:\" etc.
                out.insert(out.end(), el.begin(), el.end());
                out.push_back('\0');
            } else if (entry.rfind(L"PATH=", 0) == 0) {
                path_seen = true;
                path_value = (entry.size() > 5)
                                 ? el.substr(5)
                                 : std::string();
            } else {
                body.push_back(el);
            }
        }
        FreeEnvironmentStringsW(env);
    }

    // PATH = our dir ; original PATH.
    std::string new_path = prepend + ";";
    if (path_seen) new_path += path_value;
    body.push_back("PATH=" + new_path);

    for (const std::string& e : body) {
        out.insert(out.end(), e.begin(), e.end());
        out.push_back('\0');
    }
    out.push_back('\0');  // final terminator (block is double-NUL'd)
    return out;
}
// Directory of the C++ compiler driver (used to prepend to the child PATH so
// cc1plus finds its runtime DLLs). Empty if unresolved.
std::string compiler_bin_dir() {
    if (const char* e = std::getenv("GBARECOMP_HEAL_CXX")) {
        std::string s(e);
        std::size_t slash = s.find_last_of("/\\");
        if (slash != std::string::npos) return s.substr(0, slash);
    }
#if defined(_WIN32)
    return "C:/msys64/mingw64/bin";
#else
    return "";
#endif
}

// Spawn a child process (NOT system()), redirect stdout+stderr to logpath,
// block until exit, return the exit code (-1 on spawn failure).
int run_process(const std::string& cmdline, const std::string& logpath,
                std::string* err) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hlog = CreateFileA(logpath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                              &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE hin = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ, &sa,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = hin;
    si.hStdOutput = hlog;
    si.hStdError  = hlog;

    PROCESS_INFORMATION pi{};
    std::vector<char> cl(cmdline.begin(), cmdline.end());
    cl.push_back('\0');

    // Give cc1plus the compiler's own bin dir on PATH so its bignum runtime
    // DLLs (libmpc/libisl/libmpfr) resolve even when the host PATH targets a
    // different gcc. build_child_env returns an empty block => inherit.
    std::vector<char> child_env = build_child_env(compiler_bin_dir());
    char* envp = child_env.empty() ? nullptr : child_env.data();

    BOOL ok = CreateProcessA(nullptr, cl.data(), nullptr, nullptr,
                             /*bInheritHandles=*/TRUE, CREATE_NO_WINDOW,
                             envp, nullptr, &si, &pi);
    if (!ok) {
        if (err) *err = "CreateProcess(compiler) failed: " +
                        std::to_string(GetLastError());
        if (hlog != INVALID_HANDLE_VALUE) CloseHandle(hlog);
        if (hin  != INVALID_HANDLE_VALUE) CloseHandle(hin);
        return -1;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    if (hlog != INVALID_HANDLE_VALUE) CloseHandle(hlog);
    if (hin  != INVALID_HANDLE_VALUE) CloseHandle(hin);
    return static_cast<int>(code);
}

bool load_and_resolve(const std::string& dll, uint32_t pc,
                      const GbaOverlayCallbacks* cb,
                      void** out_module, void (**out_fn)(void),
                      std::string* err) {
    HMODULE h = LoadLibraryA(dll.c_str());
    if (!h) {
        if (err) *err = "LoadLibrary(" + dll + ") failed: " +
                        std::to_string(GetLastError());
        return false;
    }
    auto abi = reinterpret_cast<uint32_t (*)(void)>(
        reinterpret_cast<void*>(GetProcAddress(h, "overlay_abi")));
    if (!abi || abi() != GBA_OVERLAY_ABI_VERSION) {
        if (err) *err = "ABI mismatch in " + dll + " (dll=" +
                        std::to_string(abi ? abi() : 0u) + " runtime=" +
                        std::to_string(GBA_OVERLAY_ABI_VERSION) +
                        ") — rejecting + deleting stale cache entry";
        FreeLibrary(h);
        DeleteFileA(dll.c_str());
        return false;
    }
    auto init = reinterpret_cast<void (*)(const GbaOverlayCallbacks*)>(
        reinterpret_cast<void*>(GetProcAddress(h, "overlay_init")));
    if (!init) {
        if (err) *err = "no overlay_init in " + dll;
        FreeLibrary(h);
        return false;
    }
    init(cb);

    char fname[24];
    std::snprintf(fname, sizeof(fname), "func_%08X", pc);
    auto fn = reinterpret_cast<void (*)(void)>(
        reinterpret_cast<void*>(GetProcAddress(h, fname)));
    if (!fn) {
        if (err) *err = std::string("no ") + fname + " export in " + dll;
        FreeLibrary(h);
        return false;
    }
    *out_module = reinterpret_cast<void*>(h);
    *out_fn = fn;
    return true;
}
#else
#include <dlfcn.h>

int run_process(const std::string& cmdline, const std::string& logpath,
                std::string*) {
    std::string c = cmdline + " > \"" + logpath + "\" 2>&1";
    return std::system(c.c_str());
}
// POSIX mirror of the Win32 path above: dlopen/dlsym for
// LoadLibrary/GetProcAddress. Until this existed, every heal off Windows failed
// with "overlay loading unimplemented on this platform", so a macOS or Linux
// session could compile an overlay and still never run it -- coverage was
// permanently NOT_STATIC no matter what the config contained.
bool load_and_resolve(const std::string& dll, uint32_t pc,
                      const GbaOverlayCallbacks* cb,
                      void** out_module, void (**out_fn)(void),
                      std::string* err) {
    // RTLD_LOCAL so an overlay's symbols cannot collide with the next one's:
    // every overlay exports the same overlay_abi / overlay_init names.
    void* h = dlopen(dll.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        const char* e = dlerror();
        if (err) *err = "dlopen(" + dll + ") failed: " + (e ? e : "unknown");
        return false;
    }
    auto abi = reinterpret_cast<uint32_t (*)(void)>(dlsym(h, "overlay_abi"));
    if (!abi || abi() != GBA_OVERLAY_ABI_VERSION) {
        if (err) *err = "ABI mismatch in " + dll + " (so=" +
                        std::to_string(abi ? abi() : 0u) + " runtime=" +
                        std::to_string(GBA_OVERLAY_ABI_VERSION) +
                        ") — rejecting + deleting stale cache entry";
        dlclose(h);
        std::error_code ec;
        fs::remove(dll, ec);
        return false;
    }
    auto init = reinterpret_cast<void (*)(const GbaOverlayCallbacks*)>(
        dlsym(h, "overlay_init"));
    if (!init) {
        if (err) *err = "no overlay_init in " + dll;
        dlclose(h);
        return false;
    }
    init(cb);

    char fname[24];
    std::snprintf(fname, sizeof(fname), "func_%08X", pc);
    auto fn = reinterpret_cast<void (*)(void)>(dlsym(h, fname));
    if (!fn) {
        if (err) *err = std::string("no ") + fname + " export in " + dll;
        dlclose(h);
        return false;
    }
    *out_module = h;
    *out_fn = fn;
    return true;
}
#endif

}  // namespace

const char* heal_backend_name(HealBackend b) {
    return b == HealBackend::Tcc ? "tcc" : "gcc";
}

bool overlay_compile_one(const OverlayWorkItem& w,
                         const std::string& cache_dir,
                         const GbaOverlayCallbacks* cb,
                         bool compile_if_missing,
                         HealBackend backend,
                         OverlayCompiled* out,
                         std::string* err) {
    const uint8_t* image = !w.owned_bytes.empty() ? w.owned_bytes.data()
                                                  : w.bytes;
    const std::size_t image_size = !w.owned_bytes.empty() ? w.owned_bytes.size()
                                                          : w.size;
    if (!image || image_size == 0) {
        if (err) *err = "no code image for the overlay function";
        return false;
    }
    if (w.pc < w.base ||
        static_cast<std::size_t>(w.pc - w.base) >= image_size) {
        if (err) *err = "overlay PC is outside the supplied code image";
        return false;
    }

    // Discover the function extent + emit its C against the live image. The
    // single-seed finder yields the same instruction range the offline corpus
    // would, which is what makes the healed body's per-instruction fingerprint
    // byte-identical to the static build.
    uint32_t end = 0;
    std::string c_text =
        emit_overlay_c(w.pc, w.thumb, image, image_size, w.base, &end);
    if (c_text.empty() || end <= w.pc) {
        if (err) *err = "function finder found no entry at the miss PC";
        return false;
    }
    if (static_cast<std::size_t>(end - w.base) > image_size) {
        if (err) *err = "overlay function extends past the supplied code image";
        return false;
    }

    // CRC32 of the compiled-from bytes [pc, end) — keys the cache filename so a
    // changed image produces a distinct file (a stale DLL is simply orphaned).
    const uint32_t crc =
        gba::crc32(image + (w.pc - w.base), end - w.pc);

    char stem[40];
    std::snprintf(stem, sizeof(stem), "%08X_%08X_%c",
                  w.pc, crc, w.thumb ? 't' : 'a');
    const fs::path dir(cache_dir);
    const fs::path dll = dir / (std::string(stem) + ".dll");

    std::error_code ec;
    if (!fs::exists(dll, ec)) {
        if (!compile_if_missing) {
            // Warm-scan, load-only: not on disk → let it heal at runtime.
            if (err) *err = "no cached DLL (load-only)";
            return false;
        }
        fs::create_directories(dir, ec);

        const fs::path cpath   = dir / (std::string(stem) + ".c");
        const fs::path logpath = dir / (std::string(stem) + ".log");
        const fs::path dlltmp  = dir / (std::string(stem) + ".dll.tmp");

        {
            std::FILE* f = std::fopen(cpath.string().c_str(), "wb");
            if (!f) {
                if (err) *err = "cannot write " + cpath.string();
                return false;
            }
            std::fwrite(c_text.data(), 1, c_text.size(), f);
            std::fclose(f);
        }

        const std::string inc = overlay_include_flags();
        std::string cmd;
        if (backend == HealBackend::Tcc) {
            // tcc: a self-contained C compiler (own linker + headers), so it
            // needs no host toolchain. The overlay is emitted C-clean (the
            // extern \"C\" wrappers are __cplusplus-guarded), so tcc builds it
            // as C; `tcc -shared` exports the global overlay_abi / overlay_init
            // / func_<pc> symbols the loader resolves. No -O (tcc has no real
            // optimizer) and no -x c++ (it is a C compiler).
            cmd =
                "\"" + tcc_path() + "\" -shared" + inc +
                " -o \"" + dlltmp.generic_string() + "\""
                " \"" + cpath.generic_string() + "\"";
        } else {
            // Platform-specific tail. --export-all-symbols is a PE/MinGW
            // linker flag: Apple's ld64 rejects it outright, and on ELF it is
            // meaningless because a shared object exports its non-hidden
            // symbols anyway. ELF does need -fPIC for -shared, which Windows
            // and macOS do not (both are PIC by default).
#if defined(_WIN32)
            const char* plat = " -Wl,--export-all-symbols";
#elif defined(__APPLE__)
            const char* plat = "";
#else
            const char* plat = " -fPIC";
#endif
            cmd =
                "\"" + gxx_path() + "\""
                " -O2 -std=gnu++17 -fno-exceptions -fno-rtti -shared" + inc +
                " -o \"" + dlltmp.generic_string() + "\""
                // -x c++: under gcc the emitted body compiles as C++ to match
                // the static corpus's C++ semantics exactly. Explicit so it
                // never depends on the driver's .c-suffix handling.
                " -x c++ \"" + cpath.generic_string() + "\"" +
                plat;
        }

        const int rc = run_process(cmd, logpath.string(), err);
        if (rc != 0) {
            if (err) {
                *err = std::string(heal_backend_name(backend)) + " exit " +
                       std::to_string(rc) + " compiling " + cpath.string() +
                       " — see " + logpath.string();
            }
            fs::remove(dlltmp, ec);
            return false;
        }
        // Atomic publish: only a fully-linked DLL ever appears at the final path.
        fs::rename(dlltmp, dll, ec);
        if (ec) {
            // A racing producer may have published first; tolerate that.
            if (!fs::exists(dll)) {
                if (err) *err = "rename " + dlltmp.string() + " -> " +
                                dll.string() + " failed: " + ec.message();
                return false;
            }
            fs::remove(dlltmp, ec);
        }
    }

    void* module = nullptr;
    void (*fn)(void) = nullptr;
    if (!load_and_resolve(dll.string(), w.pc, cb, &module, &fn, err)) {
        return false;
    }

    out->pc     = w.pc;
    out->thumb  = w.thumb;
    out->crc    = crc;
    out->end    = end;
    out->module = module;
    out->fn     = fn;
    return true;
}

}  // namespace gbarecomp
