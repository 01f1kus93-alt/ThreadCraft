#include "ui/HealthBar.hpp"
#include "ui/GuiScale.hpp"
#include <raylib.h>
#include <cstdint>

namespace mc {

namespace {

constexpr int HEART_W = 9;
constexpr int HEART_H = 9;
constexpr int HEARTS  = 10;      // 20 HP = 10 hearts
constexpr int GAP     = 1;       // 1px gap between hearts

// Bitmask: 1 = solid, 0 = transparent (MSB-first for readability).
// Heart shape (9 columns wide, stored in uint16_t).
const uint16_t s_heartMask[HEART_H] = {
    0b011001100,  //  ##  ##
    0b111111111,  // #########
    0b111111111,  // #########
    0b111111111,  // #########
    0b011111110,  //  #######
    0b001111100,  //   #####
    0b000111000,  //    ###
    0b000010000,  //     #
    0b000000000,  //
};

Texture2D generateHeartTexture(Color fill, Color outline) {
    Image img = GenImageColor(HEART_W, HEART_H, Color{ 0, 0, 0, 0 });
    Color* px = (Color*)img.data;

    for (int y = 0; y < HEART_H; ++y) {
        for (int x = 0; x < HEART_W; ++x) {
            if (!((s_heartMask[y] >> (8 - x)) & 1)) continue;

            // Determine if this pixel is on the edge (adjacent to transparent)
            bool edge = false;
            const int dx[] = { -1, 0, 1, 0, -1, -1, 1, 1 };
            const int dy[] = { 0, -1, 0, 1, -1, 1, -1, 1 };
            for (int d = 0; d < 8 && !edge; ++d) {
                const int nx = x + dx[d], ny = y + dy[d];
                if (nx < 0 || nx >= HEART_W || ny < 0 || ny >= HEART_H) {
                    edge = true;
                } else if (!((s_heartMask[ny] >> (8 - nx)) & 1)) {
                    edge = true;
                }
            }
            px[y * HEART_W + x] = edge ? outline : fill;
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

} // namespace

bool HealthBar::load() {
    m_fullHeart  = generateHeartTexture(Color{ 200, 0, 0, 255 },
                                        Color{ 120, 0, 0, 255 });
    m_emptyHeart = generateHeartTexture(Color{ 60, 60, 60, 255 },
                                        Color{ 30, 30, 30, 255 });
    m_loaded = (m_fullHeart.id != 0 && m_emptyHeart.id != 0);
    return m_loaded;
}

void HealthBar::unload() {
    if (m_fullHeart.id  != 0) UnloadTexture(m_fullHeart);
    if (m_emptyHeart.id != 0) UnloadTexture(m_emptyHeart);
    m_fullHeart  = Texture2D{};
    m_emptyHeart = Texture2D{};
    m_loaded = false;
}

void HealthBar::draw(float health) const {
    if (!m_loaded) return;

    const int s = guiScale();
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    // Hotbar position reference (same as HotbarRenderer).
    constexpr int HB_W          = 182;  // widgets.png hotbar width in GUI-px
    constexpr int BOTTOM_MARGIN = 4;    // GUI-px below hotbar
    const int barW = HB_W * s;
    const int barX = (sw - barW) / 2;
    const int barY = sh - 22 * s - BOTTOM_MARGIN * s;  // 22 = HB_H

    // Hearts sit above the hotbar, aligned to its left edge.
    const int heartSize = 9 * s;
    const int gap = GAP * s;
    const int hx = barX;
    const int hy = barY - heartSize - 4 * s;

    for (int i = 0; i < HEARTS; ++i) {
        const float hpForThisHeart = health - 2.0f * (float)i; // 2 HP per heart
        const int x = hx + i * (heartSize + gap);

        if (hpForThisHeart >= 2.0f) {
            // Full heart
            DrawTexturePro(m_fullHeart,
                { 0, 0, (float)HEART_W, (float)HEART_H },
                { (float)x, (float)hy, (float)heartSize, (float)heartSize },
                { 0, 0 }, 0.0f, WHITE);
        } else if (hpForThisHeart >= 1.0f) {
            // Half heart: empty + left half of full
            DrawTexturePro(m_emptyHeart,
                { 0, 0, (float)HEART_W, (float)HEART_H },
                { (float)x, (float)hy, (float)heartSize, (float)heartSize },
                { 0, 0 }, 0.0f, WHITE);
            // Left half of full heart
            DrawTexturePro(m_fullHeart,
                { 0, 0, (float)HEART_W * 0.5f, (float)HEART_H },
                { (float)x, (float)hy, (float)heartSize * 0.5f, (float)heartSize },
                { 0, 0 }, 0.0f, WHITE);
        } else {
            // Empty heart
            DrawTexturePro(m_emptyHeart,
                { 0, 0, (float)HEART_W, (float)HEART_H },
                { (float)x, (float)hy, (float)heartSize, (float)heartSize },
                { 0, 0 }, 0.0f, WHITE);
        }
    }
}

} // namespace mc