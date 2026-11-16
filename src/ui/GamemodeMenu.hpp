#pragma once
#include "core/Types.hpp"
#include <raylib.h>

namespace mc {

class TextureAtlas;

// F3-hold gamemode switcher (like vanilla MC 1.20).
// Hold F3 to open, press F4 to cycle, release to fade out + apply.
// 4 slots: Survival (sword), Creative (block), Spectator (eye), Adventure (inactive grass).
class GamemodeMenu {
public:
    bool load();
    void unload();

    bool isOpen() const noexcept { return m_state != State::Closed; }
    float fadeAlpha() const noexcept { return m_fade; }

    // Call every frame. currentMode is used to initialise selection on open.
    void update(float dt, GameMode currentMode);
    void draw(const TextureAtlas& atlas) const;

    // Returns true once after fade-out completes, writes the applied GameMode.
    bool takeModeChange(GameMode& outMode);
    GameMode selectedMode() const noexcept { return slotToMode(m_selected); }

private:
    enum class State : uint8_t { Closed, Open, Closing };

    static bool   isActiveSlot(int slot) noexcept { return slot != 3; }
    static GameMode slotToMode(int slot) noexcept;
    static int    modeToSlot(GameMode mode) noexcept;
    void cycle() noexcept;

    State   m_state   = State::Closed;
    float   m_fade    = 0.0f;
    int     m_selected = 0;     // 0=Survival, 1=Creative, 2=Spectator, 3=Adventure(inactive)
    bool    m_modeChanged = false;
    GameMode m_pendingMode = GameMode::Survival;

    Texture2D m_bgTex{};      // gamemode_switcher.png
    Texture2D m_swordTex{};   // diamond_sword.png
    Texture2D m_eyeTex{};     // ender_eye.png
    bool m_loaded = false;
};

} // namespace mc