#pragma once
#include "ui/ItemStack.hpp"
#include <raylib.h>

namespace mc {

class Inventory;
class ItemEntityManager;
class TextureAtlas;

// Player inventory screen (inventory.png 176x166 GUI region).
// Armor / crafting / result / offhand slots are visual placeholders (kind >= 2).
class InventoryScreen {
public:
    bool load();
    void unload();

    bool isOpen() const noexcept { return m_open; }
    void open();
    void close(Inventory& inv);   // returns held stack via mergeInto
    void toggle(Inventory& inv);

    void update(Inventory& inv, ItemEntityManager& items, const Vector3& dropPos);
    void draw(const Inventory& inv, const TextureAtlas& atlas) const;

    bool consumeMouseSkip();

private:
    bool findSlot(int mx, int my, int invX, int invY, int s, int& kind, int& index) const;

    Texture2D m_inventoryTex{};
    bool      m_loaded = false;
    bool      m_open = false;
    bool      m_skipMouse = false;
    ItemStack m_held{};
};

} // namespace mc
