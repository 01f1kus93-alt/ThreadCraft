#include "ui/InventoryScreen.hpp"
#include "ui/Inventory.hpp"
#include "ui/GuiScale.hpp"
#include "world/BlockRegistry.hpp"
#include "world/ItemEntityManager.hpp"
#include "render/TextureAtlas.hpp"
#include "core/Config.hpp"
#include <algorithm>
#include <utility>

namespace mc {

namespace {
    constexpr int GUI_W = 176;
    constexpr int GUI_H = 166;
    constexpr int PITCH = 18;

    int slotCount(int kind) {
        switch (kind) {
        case 0: return 9;
        case 1: return 27;
        case 2: return 4;
        case 3: return 4;
        case 4: return 1;
        case 5: return 1;
        }
        return 0;
    }

    // Content coords (GUI-px) of a slot by (kind, index).
    bool slotContent(int kind, int index, int& outX, int& outY) {
        switch (kind) {
        case 0:  // hotbar 9
            if (index < 0 || index >= 9) return false;
            outX = 8 + index * PITCH; outY = 142; return true;
        case 1:  // main 27
            if (index < 0 || index >= 27) return false;
            outX = 8 + (index % 9) * PITCH; outY = 84 + (index / 9) * PITCH; return true;
        case 2:  // armor 4 (placeholder)
            if (index < 0 || index >= 4) return false;
            outX = 8; outY = 8 + index * PITCH; return true;
        case 3:  // crafting 2x2 (placeholder)
            if (index < 0 || index >= 4) return false;
            outX = 98 + (index % 2) * PITCH; outY = 18 + (index / 2) * PITCH; return true;
        case 4:  // result (placeholder)
            if (index != 0) return false;
            outX = 154; outY = 28; return true;
        case 5:  // offhand (placeholder)
            if (index != 0) return false;
            outX = 77; outY = 62; return true;
        }
        return false;
    }

    void drawIcon(const TextureAtlas& atlas, BlockID id, int x, int y, int size) {
        const BlockType& bt = BlockRegistry::get(id);
        const int tile = bt.faceTiles[(int)Face::PosY];
        if (tile < 0) return;
        const TileUV uv = atlas.tileUV(tile);
        const float aw = (float)atlas.texture().width;
        const float ah = (float)atlas.texture().height;
        const Rectangle src = { uv.u0 * aw, uv.v0 * ah,
                                (uv.u1 - uv.u0) * aw, (uv.v1 - uv.v0) * ah };
        DrawTexturePro(atlas.texture(), src,
            { (float)x, (float)y, (float)size, (float)size },
            { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    void drawCount(int count, int x, int y, int s) {
        if (count <= 1) return;
        const char* txt = TextFormat("%d", count);
        const int fs = 8 * s;
        const int tw = MeasureText(txt, fs);
        const int tx = x + 16 * s - tw - s;
        const int ty = y + 16 * s - fs - s;
        DrawText(txt, tx + s, ty + s, fs, Color{ 63, 63, 63, 255 });
        DrawText(txt, tx,     ty,     fs, WHITE);
    }
}

bool InventoryScreen::load() {
    m_inventoryTex = LoadTexture("assets/minecraft/textures/gui/container/inventory.png");
    if (m_inventoryTex.id == 0) {
        TraceLog(LOG_WARNING, "InventoryScreen: inventory.png missing, procedural fallback");
        m_loaded = false;
        return false;
    }
    SetTextureFilter(m_inventoryTex, TEXTURE_FILTER_POINT);
    m_loaded = true;
    return true;
}

void InventoryScreen::unload() {
    if (m_inventoryTex.id != 0) UnloadTexture(m_inventoryTex);
    m_inventoryTex = Texture2D{};
    m_loaded = false;
    m_open = false;
    m_skipMouse = false;
    m_held.clear();
}

void InventoryScreen::open() {
    m_open = true;
    EnableCursor();
    (void)GetMouseDelta();
}

void InventoryScreen::close(Inventory& inv) {
    if (!m_held.isEmpty()) {
        inv.mergeInto(m_held);
        m_held.clear();
    }
    m_open = false;
    DisableCursor();
    m_skipMouse = true;
}

void InventoryScreen::toggle(Inventory& inv) {
    if (m_open) close(inv);
    else        open();
}

bool InventoryScreen::consumeMouseSkip() {
    const bool v = m_skipMouse;
    m_skipMouse = false;
    return v;
}

bool InventoryScreen::findSlot(int mx, int my, int invX, int invY, int s,
                               int& kind, int& index) const {
    // Hit-test is 18x18 (1 px outward from the 16x16 content).
    for (int k = 0; k <= 5; ++k) {
        for (int i = 0; i < slotCount(k); ++i) {
            int cx, cy;
            if (!slotContent(k, i, cx, cy)) continue;
            const int sx = invX + (cx - 1) * s;
            const int sy = invY + (cy - 1) * s;
            const int sz = 18 * s;
            if (mx >= sx && mx < sx + sz && my >= sy && my < sy + sz) {
                kind = k;
                index = i;
                return true;
            }
        }
    }
    return false;
}

void InventoryScreen::update(Inventory& inv, ItemEntityManager& items, const Vector3& dropPos) {
    if (!m_open) return;

    const int s = guiScale();
    const int invX = (GetScreenWidth() - GUI_W * s) / 2;
    const int invY = (GetScreenHeight() - GUI_H * s) / 2;
    const int mx = GetMouseX();
    const int my = GetMouseY();

    int kind = -1, index = -1;
    if (findSlot(mx, my, invX, invY, s, kind, index)) {
        if (kind > 1) return;   // placeholder slots: ignore clicks

        ItemStack& slot = (kind == 0) ? inv.hotbar(index) : inv.main(index);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (m_held.isEmpty()) {
                if (!slot.isEmpty()) { m_held = slot; slot.clear(); }
            } else if (slot.isEmpty()) {
                slot = m_held; m_held.clear();
            } else if (slot.id == m_held.id) {
                const int room = config::STACK_MAX - (int)slot.count;
                const int put  = std::min((int)m_held.count, room);
                slot.count = (uint8_t)(slot.count + put);
                m_held.count = (uint8_t)(m_held.count - put);
                if (m_held.count == 0) m_held.clear();
            } else {
                std::swap(slot, m_held);
            }
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (m_held.isEmpty()) {
                if (!slot.isEmpty()) {
                    const int half = ((int)slot.count + 1) / 2;
                    m_held = ItemStack{ slot.id, (uint8_t)half };
                    slot.count = (uint8_t)(slot.count - half);
                    if (slot.count == 0) slot.clear();
                }
            } else if (slot.isEmpty()) {
                slot = ItemStack{ m_held.id, 1 };
                if (--m_held.count == 0) m_held.clear();
            } else if (slot.id == m_held.id && slot.count < config::STACK_MAX) {
                ++slot.count;
                if (--m_held.count == 0) m_held.clear();
            }
        }
    } else {
        // Click outside any slot -> drop the held stack into the world.
        if (!m_held.isEmpty() &&
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
             IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {
            items.spawn(dropPos, m_held.id, m_held.count);
            m_held.clear();
        }
    }
}

void InventoryScreen::draw(const Inventory& inv, const TextureAtlas& atlas) const {
    if (!m_open) return;

    const int s = guiScale();
    const int invX = (GetScreenWidth() - GUI_W * s) / 2;
    const int invY = (GetScreenHeight() - GUI_H * s) / 2;

    if (m_loaded) {
        DrawTexturePro(m_inventoryTex,
            { 0.0f, 0.0f, (float)GUI_W, (float)GUI_H },
            { (float)invX, (float)invY, (float)(GUI_W * s), (float)(GUI_H * s) },
            { 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        DrawRectangle(invX, invY, GUI_W * s, GUI_H * s, Color{ 198, 198, 198, 255 });
        for (int k = 0; k <= 5; ++k) {
            for (int i = 0; i < slotCount(k); ++i) {
                int cx, cy;
                if (!slotContent(k, i, cx, cy)) continue;
                DrawRectangleLines(invX + cx * s, invY + cy * s, 16 * s, 16 * s,
                                   Color{ 60, 60, 60, 255 });
            }
        }
    }

    // Real slots: hotbar + main
    for (int i = 0; i < 9; ++i) {
        const ItemStack& st = inv.hotbar(i);
        if (st.isEmpty()) continue;
        int cx, cy; slotContent(0, i, cx, cy);
        drawIcon(atlas, st.id, invX + cx * s, invY + cy * s, 16 * s);
        drawCount((int)st.count, invX + cx * s, invY + cy * s, s);
    }
    for (int i = 0; i < 27; ++i) {
        const ItemStack& st = inv.main(i);
        if (st.isEmpty()) continue;
        int cx, cy; slotContent(1, i, cx, cy);
        drawIcon(atlas, st.id, invX + cx * s, invY + cy * s, 16 * s);
        drawCount((int)st.count, invX + cx * s, invY + cy * s, s);
    }

    // Held item follows the cursor
    if (!m_held.isEmpty()) {
        const int hx = GetMouseX() - 8 * s;
        const int hy = GetMouseY() - 8 * s;
        drawIcon(atlas, m_held.id, hx, hy, 16 * s);
        drawCount((int)m_held.count, hx, hy, s);
    }
}

} // namespace mc
