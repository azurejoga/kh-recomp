// Harness that links the real gbarecomp a11y_prism.cpp (the exact code baked
// into KHRecomp.exe) and exercises init + speak, so we validate the actual
// game code, not a re-implementation.
#include <cstdio>
#include "a11y_prism.h"

int main() {
    if (!gbarecomp::a11y_init()) {
        std::printf("A11Y: init FAILED\n");
        return 1;
    }
    std::printf("A11Y: init OK active=%d\n", gbarecomp::a11y_active());
    gbarecomp::a11y_speak("Teste de acessibilidade Kingdom Hearts", true);
    std::printf("A11Y: spoke\n");
    gbarecomp::a11y_shutdown();
    std::printf("A11Y: shutdown OK\n");
    return 0;
}