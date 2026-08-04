// a11y_prism.cpp — Screen-reader bridge implementation (Prism C API).
//
// Links the prebuilt Prism release DLL (see prism/ in the repo root).
// Important: prism_registry_acquire() returns an *uninitialized* backend that
// must go through prism_backend_initialize() before use. (Using
// prism_registry_acquire_best() returns an already-initialized backend, so
// calling initialize again fails with PRISM_ERROR_ALREADY_INITIALIZED / 15.)
// We follow Prism's own nvda_backend_demo: prism_init(nullptr) then explicit
// acquire + initialize, falling back NVDA -> OneCore -> SAPI, then speaker.
#include "a11y_prism.h"

#include <cstdio>
#include <cstring>

#include "prism.h"

namespace gbarecomp {

namespace {

struct PrismHandles {
    PrismContext* ctx = nullptr;
    PrismBackend* backend = nullptr;
    bool ok = false;
};

PrismHandles g_;

// Backend ids in the order we prefer them. NVDA is first because the user runs
// NVDA; the later entries are sane fallbacks for a machine without it.
constexpr PrismBackendId kPreferredBackends[] = {
    PRISM_BACKEND_NVDA,
    PRISM_BACKEND_ONE_CORE,
    PRISM_BACKEND_SAPI,
};

bool try_acquire_and_init(PrismContext* ctx, PrismBackendId id) {
    PrismBackend* be = prism_registry_acquire(ctx, id);
    if (!be) return false;
    if (!prism_registry_exists(ctx, id)) {
        prism_backend_free(be);
        return false;
    }
    PrismError er = prism_backend_initialize(be);
    if (er != PRISM_OK) {
        std::fprintf(stderr, "[a11y] backend %s init error %d (%s)\n",
                     prism_registry_name(ctx, id), (int)er,
                     prism_error_string(er));
        prism_backend_free(be);
        return false;
    }
    g_.backend = be;
    return true;
}

}  // namespace

bool a11y_init() {
    if (g_.ok) return true;

    PrismContext* ctx = prism_init(nullptr);
    if (!ctx) {
        std::fprintf(stderr, "[a11y] prism_init failed\n");
        return false;
    }
    g_.ctx = ctx;

    for (PrismBackendId id : kPreferredBackends) {
        if (prism_registry_exists(ctx, id) && try_acquire_and_init(ctx, id)) {
            g_.ok = true;
            std::fprintf(stderr, "[a11y] active backend: %s\n",
                         prism_backend_name(g_.backend));
            return true;
        }
    }
    // Last resort: any available backend designed for this platform.
    std::fprintf(stderr, "[a11y] no preferred backend init'd; last resort\n");
    PrismBackend* be = prism_registry_acquire_best(ctx);
    if (be) {
        g_.backend = be;  // acquire_best returns already-initialized backend
        g_.ok = true;
        std::fprintf(stderr, "[a11y] active backend (best): %s\n",
                     prism_backend_name(be));
        return true;
    }
    std::fprintf(stderr, "[a11y] no prism backend available\n");
    return false;
}

void a11y_shutdown() {
    if (g_.backend) {
        prism_backend_stop(g_.backend);
        prism_backend_free(g_.backend);
    }
    if (g_.ctx) prism_shutdown(g_.ctx);
    g_ = {};
}

void a11y_speak(const char* text, bool interrupt) {
    if (!g_.ok || !text || !*text) return;
    PrismError er = prism_backend_speak(g_.backend, text, interrupt);
    if (er != PRISM_OK)
        std::fprintf(stderr, "[a11y] speak error %d (%s)\n", (int)er,
                     prism_error_string(er));
}

bool a11y_active() {
    return g_.ok;
}

}  // namespace gbarecomp