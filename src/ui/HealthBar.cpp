#include "ui/HealthBar.hpp"
#include "ui/GuiScale.hpp"
#include <raylib.h>

namespace mc {

namespace {

constexpr int HEART_W   = 9;      // tile width in icons.png
constexpr int HEART_H   = 9;      // tile height
constexpr int HEARTS    = 10;     // 20 HP = 10 hearts
constexpr int GAP       = 1;      // 1px gap between hearts

// Source rectangles in icons.png (256×256), 9×9 tiles at y=0.
constexpr Rectangle SRC_EMPTY = { 16.0f, 0.0f, 9.0f, 9.0f };
constexpr Rectangle SRC_FULL  = { 52.0f, 0.0f, 9.0f, 9.0f };
constexpr Rectangle SRC_HALF  = { 61.0f, 0.0f, 9.0f, 9.0f };

} // namespace

bool HealthBar::load() {
    m_iconsTex = LoadTexture("assets/minecraft/textures/gui/icons.png");
    if (m_iconsTex.id == 0) {
        TraceLog(LOG_WARNING, "HealthBar: icons.png not found");
        m_loaded = false;
        return false;
    }
    SetTextureFilter(m_iconsTex, TEXTURE_FILTER_POINT);
    m_loaded = true;
    return true;
}

void HealthBar::unload() {
    if (m_iconsTex.id != 0) UnloadTexture(m_iconsTex);
    m_iconsTex = Texture2D{};
    m_loaded   = false;
}

void HealthBar::draw(float health, GameMode mode) const {
    // Hidden in Creative / Spectator (show full bars in survival / adventure).
    if (mode == GameMode::Creative || mode == GameMode::Spectator) return;
    if (!m_loaded) return;

    const int s  = guiScale();
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    // Hotbar position (mirrors HotbarRenderer).
    constexpr int HB_W          = 182;
    constexpr int BOTTOM_MARGIN = 4;
    const int barW = HB_W * s;
    const int barX = (sw - barW) / 2;
    const int barY = sh - 22 * s - BOTTOM_MARGIN * s;  // 22 = HB_H

    const int heartPx = HEART_W * s;
    const int gap     = GAP * s;
    const int hx      = barX;
    const int hy      = barY - heartPx - 4 * s;  // above hotbar

    for (int i = 0; i < HEARTS; ++i) {
        const float hpForHeart = health - 2.0f * (float)i;
        const int x = hx + i * (heartPx + gap);
        const Rectangle dst = { (float)x, (float)hy, (float)heartPx, (float)heartPx };

        // Full, half, or empty?
        // Half heart: draw empty background first, then the half-icon on top.
        if (hpForHeart >= 2.0f) {
            DrawTexturePro(m_iconsTex, SRC_FULL,  dst, { 0, 0 }, 0.0f, WHITE);
        } else if (hpForHeart >= 1.0f) {
            DrawTexturePro(m_iconsTex, SRC_EMPTY, dst, { 0, 0 }, 0.0f, WHITE);
            DrawTexturePro(m_iconsTex, SRC_HALF,  dst, { 0, 0 }, 0.0f, WHITE);
        } else {
            DrawTexturePro(m_iconsTex, SRC_EMPTY, dst, { 0, 0 }, 0.0f, WHITE);
        }
    }
}

} // namespace mc