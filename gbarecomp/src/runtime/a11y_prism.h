// a11y_prism.h — Screen-reader bridge (Prism: platform-agnostic TTS/braille).
//
// Fase 1 (PoC): wrap the Prism C API so the game can speak dialog/menu text
// through whatever screen reader is active (NVDA preferred, SAPI fallback).
// Activated with Shift+A (see host_window.cpp HK_ACCESSIBILITY).
#pragma once

namespace gbarecomp {

// Returns true if Prism initialized and at least one backend is usable.
bool a11y_init();
void a11y_shutdown();

// Speak text through the active screen reader. interrupt=true cancels
// pending speech first (use for dialog text changes).
void a11y_speak(const char* text, bool interrupt);

// True once a11y_init() succeeded. No-op calls are safe before init.
bool a11y_active();

}  // namespace gbarecomp
