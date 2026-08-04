// Compare: acquire_best (what the game used) vs explicit NVDA (what works).
#include <cstdio>
#include <prism.h>

int main() {
    // Path A: config-init + acquire_best (game's failing path)
    {
        PrismConfig cfg = prism_config_init();
        auto* ctx = prism_init(&cfg);
        std::printf("A: ctx=%p\n", (void*)ctx);
        if (ctx) {
            auto* be = prism_registry_acquire_best(ctx);
            std::printf("A: acquire_best=%p\n", (void*)be);
            if (be) {
                std::printf("A: name=%s\n", prism_backend_name(be));
                PrismError er = prism_backend_initialize(be);
                std::printf("A: initialize -> %d (%s)\n", (int)er,
                            er == PRISM_OK ? "OK" : prism_error_string(er));
                prism_backend_free(be);
            }
            prism_shutdown(ctx);
        }
    }
    // Path B: null config + explicit NVDA (demo path, known working)
    {
        auto* ctx = prism_init(nullptr);
        std::printf("B: ctx=%p\n", (void*)ctx);
        if (ctx) {
            auto* be = prism_registry_acquire(ctx, PRISM_BACKEND_NVDA);
            std::printf("B: nvda acquire=%p\n", (void*)be);
            if (be) {
                PrismError er = prism_backend_initialize(be);
                std::printf("B: initialize -> %d (%s)\n", (int)er,
                            er == PRISM_OK ? "OK" : prism_error_string(er));
                if (er == PRISM_OK)
                    prism_backend_speak(be, "Teste caminho B", true);
                prism_backend_free(be);
            }
            prism_shutdown(ctx);
        }
    }
    return 0;
}
