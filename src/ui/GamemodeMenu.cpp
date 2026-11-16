#include "ui/GamemodeMenu.hpp"
#include "ui/GuiScale.hpp"
#include "render/TextureAtlas.hpp"
#include "world/BlockRegistry.hpp"
#include <raylib.h>
#include <cstdio>

namespace mc {

namespace {

constexpr int SLOT_SIZE = 32;   // GUI-px per slot
constexpr int ICON_SIZE = 24;
constexpr int GAP       = 8;
constexpr int BG_PAD    = 12;
constexpr int SLOT_BAR_W = BG_PAD + 4 * SLOT_SIZE + 3 * GAP + BG_PAD;  // 188
constexpr int SLOT_BAR_H = BG_PAD + SLOT_SIZE + BG_PAD;                // 56
constexpr int TITLE_H    = 16;

const Color BG_COLOR      = {  0,   0,   0, 180 };
const Color SLOT_BG       = { 40,  40,  40, 200 };
const Color SLOT_SEL_BG   = { 90, 126, 148, 230 };  // blue-ish highlight
const Color SLOT_INACTIVE = { 30,  30,  30, 180 };
const Color ICON_DIM      = { 80,  80,  80, 255 };

constexpr int TILE_GRASS = 2;  // grass_block_top
constexpr int TILE_STONE = 0;  // stone

void drawAtlasIcon(const TextureAtlas& atlas, int tileIdx, int x, int y, int size, Color tint) {
    if (tileIdx < 0) return;
    const TileUV uv = atlas.tileUV(tileIdx);
    const float aw = (float)atlas.texture().width;
    const float ah = (float)atlas.texture().height;
    const Rectangle src = { uv.u0 * aw, uv.v0 * ah,
                            (uv.u1 - uv.u0) * aw, (uv.v1 - uv.v0) * ah };
    DrawTexturePro(atlas.texture(), src,
                   { (float)x, (float)y, (float)size, (float)size },
                   { 0.0f, 0.0f }, 0.0f, tint);
}

void drawItemTex(const Texture2D& tex, int x, int y, int size, Color tint) {
    if (tex.id == 0) return;
    DrawTexturePro(tex,
                   { 0.0f, 0.0f, (float)tex.width, (float)tex.height },
                   { (float)x, (float)y, (float)size, (float)size },
                   { 0.0f, 0.0f }, 0.0f, tint);
}

} // namespace

bool GamemodeMenu::load() {
    m_swordTex = LoadTexture("assets/minecraft/textures/item/diamond_sword.png");
    if (m_swordTex.id == 0) {
        TraceLog(LOG_WARNING, "GamemodeMenu: diamond_sword.png not found");
    } else {
        SetTextureFilter(m_swordTex, TEXTURE_FILTER_POINT);
    }
    m_eyeTex = LoadTexture("assets/minecraft/textures/item/ender_eye.png");
    if (m_eyeTex.id == 0) {
        TraceLog(LOG_WARNING, "GamemodeMenu: ender_eye.png not found");
    } else {
        SetTextureFilter(m_eyeTex, TEXTURE_FILTER_POINT);
    }
    m_loaded = true;
    return true;
}

void GamemodeMenu::unload() {
    if (m_swordTex.id != 0) UnloadTexture(m_swordTex);
    if (m_eyeTex.id != 0)   UnloadTexture(m_eyeTex);
    m_swordTex = Texture2D{};
    m_eyeTex   = Texture2D{};
    m_loaded   = false;
}

void GamemodeMenu::open() {
    m_open  = true;
    m_timer = 2.5f;
    (void)GetMouseDelta();  // consume any pending spike
}

void GamemodeMenu::close() {
    m_open  = false;
    m_pendingMode = (m_selected == 2) ? GameMode::Creative : GameMode::Survival;
    m_modeChanged = true;
}

void GamemodeMenu::update(float dt) {
    if (!m_open) return;
    m_timer -= dt;
    if (m_timer <= 0.0f) close();
}

void GamemodeMenu::cycle() {
    // Toggle between Survival (slot 0) and Creative (slot 2).
    m_selected = (m_selected == 0) ? 2 : 0;
    m_timer = 2.5f;
}

bool GamemodeMenu::takeModeChange(GameMode& outMode) {
    if (m_modeChanged) {
        m_modeChanged = false;
        outMode = m_pendingMode;
        return true;
    }
    return false;
}

void GamemodeMenu::draw(const TextureAtlas& atlas) const {
    if (!m_open) return;

    const int s = guiScale();
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    const int barW = SLOT_BAR_W * s;
    const int barH = SLOT_BAR_H * s;
    const int bx   = (sw - barW) / 2;
    const int by   = sh / 4;

    // ── Title ──
    const char* title = (m_selected == 2) ? "Creative Mode" : "Survival Mode";
    const int titleFs = TITLE_H * s;
    const int tw = MeasureText(title, titleFs);
    DrawText(title, (sw - tw) / 2, by - titleFs - 8 * s, titleFs, WHITE);

    // ── Background bar ──
    DrawRectangle(bx, by, barW, barH, BG_COLOR);

    // ── 4 slots ──
    const int slotSize = SLOT_SIZE * s;
    const int iconSize = ICON_SIZE * s;
    const int gap = GAP * s;
    const int pad = BG_PAD * s;

    for (int i = 0; i < 4; ++i) {
        const int sx = bx + pad + i * (slotSize + gap);
        const int sy = by + pad;
        const bool selected = (i == m_selected);
        const bool active   = (i == 0 || i == 2);  // Survival & Creative

        const Color slotBg = active ? (selected ? SLOT_SEL_BG : SLOT_BG)
                                    : SLOT_INACTIVE;
        DrawRectangle(sx, sy, slotSize, slotSize, slotBg);

        const int ix = sx + (slotSize - iconSize) / 2;
        const int iy = sy + (slotSize - iconSize) / 2;
        const Color tint = active ? WHITE : ICON_DIM;

        switch (i) {
        case 0: drawAtlasIcon(atlas, TILE_GRASS, ix, iy, iconSize, tint); break;
        case 1: drawItemTex(m_swordTex,        ix, iy, iconSize, tint); break;
        case 2: drawAtlasIcon(atlas, TILE_STONE, ix, iy, iconSize, tint); break;
        case 3: drawItemTex(m_eyeTex,          ix, iy, iconSize, tint); break;
        }
    }

    // ── Hint ──
    const char* hint = "[ F4 ] Next";
    const int hintFs = 12 * s;
    const int hintW  = MeasureText(hint, hintFs);
    DrawText(hint, (sw - hintW) / 2, by + barH + 6 * s, hintFs,
             Color{ 180, 180, 180, 255 });
}

} // namespace mc