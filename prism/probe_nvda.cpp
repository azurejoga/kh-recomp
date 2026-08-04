// Quick NVDA-backend probe (mirrors prism's own nvda_backend_demo, minus
// std::println which lacks MinGW symbols). Links the prebuilt prism.dll.
#include <cstdio>
#include <prism.h>

int main() {
    auto* ctx = prism_init(nullptr);
    if (!ctx) {
        std::printf("PRISM: init failed\n");
        return 1;
    }
    std::printf("PRISM: registry count = %zu\n", prism_registry_count(ctx));
    for (size_t i = 0; i < prism_registry_count(ctx); i++) {
        PrismBackendId id = prism_registry_id_at(ctx, i);
        std::printf("  backend[%zu] id=%016llx name=%s priority=%d\n", i,
                    (unsigned long long)id, prism_registry_name(ctx, id),
                    prism_registry_priority(ctx, id));
    }
    if (!prism_registry_exists(ctx, PRISM_BACKEND_NVDA)) {
        std::printf("PRISM: NVDA not compiled in!\n");
        prism_shutdown(ctx);
        return 1;
    }
    auto* be = prism_registry_acquire(ctx, PRISM_BACKEND_NVDA);
    if (!be) {
        std::printf("PRISM: NVDA acquire failed\n");
        prism_shutdown(ctx);
        return 1;
    }
    PrismError er = prism_backend_initialize(be);
    std::printf("PRISM: NVDA initialize -> %d (%s)\n", (int)er,
                er == PRISM_OK ? "OK" : prism_error_string(er));
    if (er == PRISM_OK) {
        er = prism_backend_speak(be, "Teste de acessibilidade do Kingdom Hearts", true);
        std::printf("PRISM: speak -> %d (%s)\n", (int)er,
                    er == PRISM_OK ? "OK" : prism_error_string(er));
    }
    prism_backend_free(be);
    prism_shutdown(ctx);
    return 0;
}
