#pragma once
#include <raylib.h>

namespace mc {

// GUI scale: smallest integer scale where the classic 320x240 reference
// layout fits on screen. Vanilla-style, shared by all UI screens.
inline int guiScale() noexcept {
    const int w = GetScreenWidth(), h = GetScreenHeight();
    int s = 1;
    while (s < 6 && (s + 1) * 320 <= w && (s + 1) * 240 <= h) ++s;
    return s;
}

} // namespace mc
