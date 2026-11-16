#pragma once
#include "core/Types.hpp"
#include <raylib.h>

namespace mc {

class TextureAtlas;

// F3+F4 gamemode switcher (like vanilla MC).
// Shows 4 slots: Grass (Survival), Sword (Adventure inactive),
// Stone block (Creative), Ender Eye (Spectator inactive).
class GamemodeMenu {
public:
    bool load();
    void unload();

    bool isOpen() const noexcept { return m_open; }

    void open();        // EnableCursor() + GetMouseDelta()
    void close();       // triggers takeModeChange

    void update(float dt);
    void draw(const TextureAtlas& atlas) const;

    // Resets the auto-close timer (call while F3 is held).
    void resetTimer() noexcept { m_timer = 2.5f; }

    // Cycle to the next active gamemode (Survival ↔ Creative).
    void cycle();

    // Returns true once after close(), writes the selected GameMode.
    bool takeModeChange(GameMode& outMode);

private:
    bool m_open    = false;
    float m_timer = 0.0f;
    int   m_selected = 0;      // slot index: 0=Survival, 2=Creative
    bool  m_modeChanged = false;
    GameMode m_pendingMode = GameMode::Survival;

    Texture2D m_swordTex{};
    Texture2D m_eyeTex{};
    bool m_loaded = false;
};

} // namespace mc