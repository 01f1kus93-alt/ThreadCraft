#pragma once
#include "ui/Inventory.hpp"
#include "render/TextureAtlas.hpp"
#include <raylib.h>

namespace mc {

class HotbarRenderer {
public:
    bool load();
    void unload();
    // Drawn every frame while the InventoryScreen is closed.
    void draw(const Inventory& inv, const TextureAtlas& atlas) const;

private:
    Texture2D m_widgetsTex{};
    bool      m_loaded = false;
};

} // namespace mc
