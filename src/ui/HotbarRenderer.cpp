#include "ui/HotbarRenderer.hpp"
#include "ui/GuiScale.hpp"
#include "world/BlockRegistry.hpp"
#include "core/Types.hpp"
#include <raylib.h>

namespace mc {

namespace {
    // widgets.png (vanilla 1.20.1)
    constexpr int HB_W          = 182;
    constexpr int HB_H          = 22;
    constexpr int SEL_W         = 24;
    constexpr int SEL_H         = 24;
    constexpr int SLOT_PITCH    = 20;
    constexpr int SLOT_INSET    = 3;    // 1 (outer border) + 2 (padding)
    constexpr int SLOT_ITEM     = 16;
    constexpr int BOTTOM_MARGIN = 4;    // GUI-px

    // Block icon: top face (Face::PosY). Items would use a different lookup.
    void drawBlockIcon(const TextureAtlas& atlas, BlockID id, int x, int y, int size) {
        const BlockType& bt = BlockRegistry::get(id);
        const int tile = bt.faceTiles[(int)Face::PosY];
        if (tile < 0) return;

        const TileUV uv = atlas.tileUV(tile);
        const float aw = (float)atlas.texture().width;
        const float ah = (float)atlas.texture().height;
        const Rectangle src = {
            uv.u0 * aw, uv.v0 * ah,
            (uv.u1 - uv.u0) * aw,
            (uv.v1 - uv.v0) * ah
        };
        DrawTexturePro(atlas.texture(), src,
            { (float)x, (float)y, (float)size, (float)size },
            { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    void drawCount(int count, int slotX, int slotY, int s) {
        if (count <= 1) return;
        const char* txt = TextFormat("%d", count);
        const int fs = 8 * s;
        const int tw = MeasureText(txt, fs);
        const int tx = slotX + (SLOT_ITEM * s) - tw - s;
        const int ty = slotY + (SLOT_ITEM * s) - fs - s;
        DrawText(txt, tx + s, ty + s, fs, Color{ 63, 63, 63, 255 }); // shadow
        DrawText(txt, tx,     ty,     fs, WHITE);                    // number
    }
}

bool HotbarRenderer::load() {
    m_widgetsTex = LoadTexture("assets/minecraft/textures/gui/widgets.png");
    if (m_widgetsTex.id == 0) {
        TraceLog(LOG_WARNING, "HotbarRenderer: widgets.png missing, procedural fallback");
        m_loaded = false;
        return false;
    }
    SetTextureFilter(m_widgetsTex, TEXTURE_FILTER_POINT);
    m_loaded = true;
    return true;
}

void HotbarRenderer::unload() {
    if (m_widgetsTex.id != 0) UnloadTexture(m_widgetsTex);
    m_widgetsTex = Texture2D{};
    m_loaded = false;
}

void HotbarRenderer::draw(const Inventory& inv, const TextureAtlas& atlas) const {
    const int s  = guiScale();
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    const int barW = HB_W * s;
    const int barH = HB_H * s;
    const int barX = (sw - barW) / 2;
    const int barY = sh - barH - BOTTOM_MARGIN * s;

    // 1) Hotbar background
    if (m_loaded) {
        DrawTexturePro(m_widgetsTex,
            { 0.0f, 0.0f, (float)HB_W, (float)HB_H },
            { (float)barX, (float)barY, (float)barW, (float)barH },
            { 0.0f, 0.0f }, 0.0f, WHITE);

        // 2) Selected-slot frame (MC coordinates: x-1, y-2 relative to bar)
        const int sel = inv.selected();
        const float selX = (float)(barX + (sel * SLOT_PITCH - 1) * s);
        const float selY = (float)(barY - 2 * s);
        DrawTexturePro(m_widgetsTex,
            { 0.0f, 22.0f, (float)SEL_W, (float)SEL_H },
            { selX, selY, (float)(SEL_W * s), (float)(SEL_H * s) },
            { 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        // Procedural fallback: gray bar + slot frames
        DrawRectangle(barX, barY, barW, barH, Color{ 40, 40, 40, 200 });
        for (int i = 0; i < Inventory::HOTBAR; ++i) {
            const int sx = barX + (1 + i * SLOT_PITCH) * s;
            const int sy = barY + 1 * s;
            const int sz = 20 * s;
            const Color c = (i == inv.selected()) ? Color{ 220, 220, 220, 255 }
                                                  : Color{  90,  90,  90, 255 };
            DrawRectangleLines(sx, sy, sz, sz, c);
        }
    }

    // 3) Icons + counts
    for (int i = 0; i < Inventory::HOTBAR; ++i) {
        const ItemStack& st = inv.hotbar(i);
        if (st.isEmpty()) continue;

        const int ix = barX + (SLOT_INSET + i * SLOT_PITCH) * s;
        const int iy = barY + SLOT_INSET * s;
        drawBlockIcon(atlas, st.id, ix, iy, SLOT_ITEM * s);
        drawCount((int)st.count,
                  barX + (i * SLOT_PITCH + 1) * s,   // slot left corner
                  barY + 1 * s,
                  s);
    }
}

} // namespace mc
