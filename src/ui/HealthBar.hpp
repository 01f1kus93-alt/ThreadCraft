#pragma once
#include <raylib.h>

namespace mc {

// Procedural 9x9 heart textures (full red + empty dark) for the health HUD.
// Drawn above the hotbar, left-aligned (10 hearts = 20 HP).
class HealthBar {
public:
    bool load();
    void unload();

    void draw(float health) const;

private:
    Texture2D m_fullHeart{};
    Texture2D m_emptyHeart{};
    bool m_loaded = false;
};

} // namespace mc