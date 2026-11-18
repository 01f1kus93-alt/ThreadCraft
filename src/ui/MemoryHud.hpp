#pragma once
#include <raylib.h>

namespace mc {

// Top-left HUD line "Mem: <process RSS> / <system total>".
// Values are cached and refreshed once per second, independent of frame rate.
class MemoryHud {
public:
    void update(float dt);
    void draw() const;

private:
    float m_accum = 0.0f;    // seconds since last refresh
    bool  m_valid = false;   // last read succeeded
    char  m_text[128] = {};  // cached formatted line
};

} // namespace mc
