#pragma once
#include "core/Types.hpp"   // GameMode
#include <raylib.h>

namespace mc {

// Hearts HUD using icons.png (vanilla 1.20.1 layout).
// 10 hearts (20 HP), drawn left-aligned above hotbar.
// Hidden in Creative / Spectator.
class HealthBar {
public:
    bool load();
    void unload();

    void draw(float health, GameMode mode) const;

private:
    Texture2D m_iconsTex{};
    bool m_loaded = false;
};

} // namespace mc