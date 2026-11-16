#include "ui/GamemodeMenu.hpp"
#include "ui/GuiScale.hpp"
#include "render/TextureAtlas.hpp"
#include <raylib.h>
#include <cstdio>
#include <algorithm>

namespace mc {

namespace {

// ── Layout from gamemode_switcher.png (128x128) ──
// Background: x=0, y=0, w=125, h=75
// Slot:       x=0, y=75, w=26, h=26
// Selection:  x=26, y=75, w=26, h=26
constexpr int BG_W = 125;
constexpr int BG_H = 75;
constexpr int SLOT_SIZE = 26;
constexpr int ICON_SIZE = 16;
constexpr int SLOT_Y   = 20;           // y-offset of slots within background
constexpr int SLOT_X[4] = { 10, 41, 72, 103 };  // x offsets of 4 slots within background
constexpr int TITLE_H    = 16;
constexpr int HINT_H     = 12;
constexpr float FADE_TIME = 0.15f;

// ── Atlas tile indices ──
constexpr int TILE_GRASS_BLOCK_SIDE = 3;   // Creative icon
constexpr int TILE_GRASS_BLOCK_TOP  = 2;   // Adventure icon (dimmed, inactive)

// ── Slot → mode mapping (Creative, Survival, Adventure*, Spectator) ──
constexpr const char* MODE_NAMES[4] = {
    "Creative Mode",
    "Survival Mode",
    "Adventure Mode",
    "Spectator Mode"
};

// ── Helpers ──

void drawAtlasIcon(const TextureAtlas& atlas, int tileIdx,
                   float x, float y, float size, Color tint) {
    if (tileIdx < 0) return;
    const TileUV uv = atlas.tileUV(tileIdx);
    const float aw = (float)atlas.texture().width;
    const float ah = (float)atlas.texture().height;
    const Rectangle src = {
        uv.u0 * aw, uv.v0 * ah,
        (uv.u1 - uv.u0) * aw,
        (uv.v1 - uv.v0) * ah
    };
    DrawTexturePro(atlas.texture(), src,
                   { x, y, size, size }, { 0, 0 }, 0.0f, tint);
}

void drawItemTex(const Texture2D& tex, float x, float y, float size, Color tint) {
    if (tex.id == 0) return;
    const float tw = (float)tex.width;
    const float th = (float)tex.height;
    DrawTexturePro(tex, { 0, 0, tw, th },
                   { x, y, size, size }, { 0, 0 }, 0.0f, tint);
}

void drawSwitchPart(const Texture2D& tex,
                    int srcX, int srcY, int srcW, int srcH,
                    float dstX, float dstY, float dstW, float dstH,
                    Color tint) {
    if (tex.id == 0) return;
    DrawTexturePro(tex,
                   { (float)srcX, (float)srcY, (float)srcW, (float)srcH },
                   { dstX, dstY, dstW, dstH },
                   { 0, 0 }, 0.0f, tint);
}

} // namespace

// ──────────────────────────────────────────────────────────
//  Static mapping helpers
// ──────────────────────────────────────────────────────────
GameMode GamemodeMenu::slotToMode(int slot) noexcept {
    switch (slot) {
    case 0:  return GameMode::Creative;
    case 1:  return GameMode::Survival;
    case 2:  return GameMode::Adventure;
    default: return GameMode::Spectator;
    }
}
int GamemodeMenu::modeToSlot(GameMode mode) noexcept {
    switch (mode) {
    case GameMode::Creative:  return 0;
    case GameMode::Survival:  return 1;
    case GameMode::Adventure: return 2;
    case GameMode::Spectator: return 3;
    }
    return 0;
}

// ──────────────────────────────────────────────────────────
//  Lifecycle
// ──────────────────────────────────────────────────────────
bool GamemodeMenu::load() {
    m_bgTex = LoadTexture("assets/minecraft/textures/gui/container/gamemode_switcher.png");
    if (m_bgTex.id != 0) SetTextureFilter(m_bgTex, TEXTURE_FILTER_POINT);
    else TraceLog(LOG_WARNING, "GamemodeMenu: gamemode_switcher.png not found");

    m_swordTex = LoadTexture("assets/minecraft/textures/item/diamond_sword.png");
    if (m_swordTex.id != 0) SetTextureFilter(m_swordTex, TEXTURE_FILTER_POINT);
    else TraceLog(LOG_WARNING, "GamemodeMenu: diamond_sword.png not found");

    m_eyeTex = LoadTexture("assets/minecraft/textures/item/ender_eye.png");
    if (m_eyeTex.id != 0) SetTextureFilter(m_eyeTex, TEXTURE_FILTER_POINT);
    else TraceLog(LOG_WARNING, "GamemodeMenu: ender_eye.png not found");

    m_loaded = true;
    return true;
}

void GamemodeMenu::unload() {
    if (m_bgTex.id    != 0) UnloadTexture(m_bgTex);
    if (m_swordTex.id != 0) UnloadTexture(m_swordTex);
    if (m_eyeTex.id   != 0) UnloadTexture(m_eyeTex);
    m_bgTex    = Texture2D{};
    m_swordTex = Texture2D{};
    m_eyeTex   = Texture2D{};
    m_loaded   = false;
}
// ──────────────────────────────────────────────────────────
//  State machine (update)
// ──────────────────────────────────────────────────────────
void GamemodeMenu::update(float dt, GameMode currentMode) {
    const bool f3 = IsKeyDown(KEY_F3);

    switch (m_state) {
    case State::Closed:
        if (f3) {
            m_state    = State::Open;
            m_selected = modeToSlot(currentMode);
            m_fade     = 0.0f;
        }
        break;

    case State::Open:
        // fade in
        m_fade = std::min(m_fade + dt / FADE_TIME, 1.0f);

        if (IsKeyPressed(KEY_F4)) cycle();

        if (!f3) {
            m_state = State::Closing;
        }
        break;

    case State::Closing:
        // re-open if F3 pressed again while fading
        if (f3) {
            m_state = State::Open;
            m_fade  = 1.0f - m_fade;
            break;
        }
        // fade out
        m_fade = std::max(m_fade - dt / FADE_TIME, 0.0f);
        if (m_fade <= 0.0f) {
            m_state       = State::Closed;
            m_pendingMode = slotToMode(m_selected);
            m_modeChanged = true;
        }
        break;
    }
}

void GamemodeMenu::cycle() noexcept {
    do { m_selected = (m_selected + 1) % 4; } while (!isActiveSlot(m_selected));
}

bool GamemodeMenu::takeModeChange(GameMode& outMode) {
    if (m_modeChanged) {
        m_modeChanged = false;
        outMode = m_pendingMode;
        return true;
    }
    return false;
}

// ──────────────────────────────────────────────────────────
//  Draw
// ──────────────────────────────────────────────────────────
void GamemodeMenu::draw(const TextureAtlas& atlas) const {
    if (m_fade <= 0.0f) return;

    const int s   = guiScale();
    const int sw  = GetScreenWidth();
    const int sh  = GetScreenHeight();

    // Background size in screen pixels
    const int bgPxW = BG_W * s;
    const int bgPxH = BG_H * s;
    const int slotPx = SLOT_SIZE * s;
    const int iconPx = ICON_SIZE * s;

    // Centre the background on screen
    const int bgX = (sw - bgPxW) / 2;
    const int bgY = (sh - bgPxH) / 2;

    // Fade alpha applied to all UI elements
    const unsigned char a = (unsigned char)(m_fade * 255.0f);
    const Color tintAll = { 255, 255, 255, a };

    // ── Title above background ──
    const char* title = MODE_NAMES[m_selected];
    const int titleFs = TITLE_H * s;
    const int titleW = MeasureText(title, titleFs);
    DrawText(title, (sw - titleW) / 2, bgY - titleFs - 8 * s, titleFs,
             Color{ 255, 255, 255, a });

    // ── Background from gamemode_switcher.png (src 0,0,125,75) ──
    drawSwitchPart(m_bgTex, 0, 0, BG_W, BG_H,
                   (float)bgX, (float)bgY, (float)bgPxW, (float)bgPxH, tintAll);

    // ── Draw slots and icons ──
    for (int i = 0; i < 4; ++i) {
        const int sx  = bgX + SLOT_X[i] * s;
        const int sy  = bgY + SLOT_Y * s;
        const bool selected = (i == m_selected);
        const bool active   = isActiveSlot(i);

        // Slot background (src 0,75,26,26)
        const Color slotTint = active ? tintAll : Color{ 100, 100, 100, a };
        drawSwitchPart(m_bgTex, 0, 75, SLOT_SIZE, SLOT_SIZE,
                       (float)sx, (float)sy, (float)slotPx, (float)slotPx, slotTint);

        // Icon centred in slot (26-16)/2 = 5 px padding
        const int ix = sx + 5 * s;
        const int iy = sy + 5 * s;
        const Color iconTint = active ? Color{ 255, 255, 255, a }
                                      : Color{ 80, 80, 80, a };

        switch (i) {
        case 0: drawAtlasIcon(atlas, TILE_GRASS_BLOCK_SIDE, (float)ix, (float)iy, (float)iconPx, iconTint); break;
        case 1: drawItemTex(m_swordTex,                     (float)ix, (float)iy, (float)iconPx, iconTint); break;
        case 2: drawAtlasIcon(atlas, TILE_GRASS_BLOCK_TOP,  (float)ix, (float)iy, (float)iconPx, iconTint); break;
        case 3: drawItemTex(m_eyeTex,                       (float)ix, (float)iy, (float)iconPx, iconTint); break;
        }

        // Selection highlight (src 26,75,26,26) drawn OVER the selected slot
        if (selected) {
            drawSwitchPart(m_bgTex, 26, 75, SLOT_SIZE, SLOT_SIZE,
                           (float)sx, (float)sy, (float)slotPx, (float)slotPx, tintAll);
        }
    }

    // ── Hint below background ──
    const char* hint = "[ F4 ] Switch";
    const int hintFs = HINT_H * s;
    const int hintW  = MeasureText(hint, hintFs);
    DrawText(hint, (sw - hintW) / 2, bgY + bgPxH + 6 * s, hintFs,
             Color{ 180, 180, 180, a });
}

} // namespace mc