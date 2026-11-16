#include "ui/GamemodeMenu.hpp"
#include "ui/GuiScale.hpp"
#include "render/TextureAtlas.hpp"
#include <raylib.h>
#include <cstdio>
#include <algorithm>

namespace mc {

namespace {

// ── Layout constants (GUI-px) ──
constexpr int SLOT_SIZE  = 32;
constexpr int ICON_SIZE  = 16;      // native item/block icon size
constexpr int GAP        = 8;
constexpr int BG_PAD     = 12;
constexpr int BAR_W = BG_PAD + 4 * SLOT_SIZE + 3 * GAP + BG_PAD;  // 176
constexpr int BAR_H = BG_PAD + SLOT_SIZE + BG_PAD;                // 56
constexpr int TITLE_H    = 16;
constexpr int HINT_H     = 12;
constexpr float FADE_TIME = 0.15f;

// ── Atlas tile indices ──
constexpr int TILE_GRASS_BLOCK_SIDE = 3;   // Creative icon
constexpr int TILE_GRASS_BLOCK_TOP  = 2;   // Adventure icon (dimmed, inactive)

// ── Slot → mode mapping ──
constexpr const char* MODE_NAMES[4] = {
    "Survival Mode",
    "Creative Mode",
    "Spectator Mode",
    "Adventure Mode"
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

} // namespace

// ──────────────────────────────────────────────────────────
//  Static mapping helpers
// ──────────────────────────────────────────────────────────
GameMode GamemodeMenu::slotToMode(int slot) noexcept {
    switch (slot) {
    case 0:  return GameMode::Survival;
    case 1:  return GameMode::Creative;
    case 2:  return GameMode::Spectator;
    default: return GameMode::Adventure;
    }
}
int GamemodeMenu::modeToSlot(GameMode mode) noexcept {
    switch (mode) {
    case GameMode::Survival:  return 0;
    case GameMode::Creative:  return 1;
    case GameMode::Spectator: return 2;
    case GameMode::Adventure: return 3;
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

    const int barW = BAR_W * s;
    const int barH = BAR_H * s;
    const int bx   = (sw - barW) / 2;
    const int by   = sh / 4;

    // Fade alpha applied to all UI elements
    const unsigned char a = (unsigned char)(m_fade * 255.0f);
    const Color tintAll = { 255, 255, 255, a };

    // ── Title ──
    const char* title = MODE_NAMES[m_selected];
    const int titleFs = TITLE_H * s;
    const int tw = MeasureText(title, titleFs);
    DrawText(title, (sw - tw) / 2, by - titleFs - 8 * s, titleFs,
             Color{ 255, 255, 255, a });

    // ── Slot backgrounds from gamemode_switcher.png ──
    const int slotPx = SLOT_SIZE * s;
    const int iconPx = ICON_SIZE * s;
    const int gap    = GAP * s;
    const int pad    = BG_PAD * s;

    for (int i = 0; i < 4; ++i) {
        const int sx  = bx + pad + i * (slotPx + gap);
        const int sy  = by + pad;
        const bool selected = (i == m_selected);
        const bool active   = isActiveSlot(i);

        // Source: selected slot → row 0, normal slot → row 1
        const float srcY = selected ? 0.0f : 32.0f;
        const Rectangle src = { 0.0f, srcY, 32.0f, 32.0f };
        const Rectangle dst = { (float)sx, (float)sy, (float)slotPx, (float)slotPx };

        if (m_bgTex.id != 0) {
            DrawTexturePro(m_bgTex, src, dst, { 0, 0 }, 0.0f, tintAll);
        } else {
            // Fallback if texture failed to load
            const Color slotCol = active
                ? (selected ? Color{ 90, 126, 148, a } : Color{ 60, 60, 60, a })
                : Color{ 30, 30, 30, a };
            DrawRectangle(sx, sy, slotPx, slotPx, slotCol);
        }

        // Icon
        const int ix = sx + (slotPx - iconPx) / 2;
        const int iy = sy + (slotPx - iconPx) / 2;
        const Color iconTint = active ? Color{ 255, 255, 255, a }
                                      : Color{ 80, 80, 80, a };

        switch (i) {
        case 0: drawItemTex(m_swordTex,     (float)ix, (float)iy, (float)iconPx, iconTint); break;
        case 1: drawAtlasIcon(atlas, TILE_GRASS_BLOCK_SIDE, (float)ix, (float)iy, (float)iconPx, iconTint); break;
        case 2: drawItemTex(m_eyeTex,       (float)ix, (float)iy, (float)iconPx, iconTint); break;
        case 3: drawAtlasIcon(atlas, TILE_GRASS_BLOCK_TOP,  (float)ix, (float)iy, (float)iconPx, iconTint); break;
        }
    }

    // ── Hint ──
    const char* hint = "[ F4 ] Switch";
    const int hintFs = HINT_H * s;
    const int hintW  = MeasureText(hint, hintFs);
    DrawText(hint, (sw - hintW) / 2, by + barH + 6 * s, hintFs,
             Color{ 180, 180, 180, a });
}

} // namespace mc