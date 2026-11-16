#pragma once
#include "ui/ItemStack.hpp"
#include <raylib.h>
#include <vector>

namespace mc {

class World;
class Inventory;
class TextureAtlas;

// Dropped item entities: gravity, simple ground snap, bobbing and pickup.
class ItemEntityManager {
public:
    void spawn(Vector3 pos, BlockID id, int count);
    void update(float dt, const Vector3& playerPos, World& world, Inventory& inv);
    void draw3D(const Camera3D& cam, const TextureAtlas& atlas) const;

    size_t count() const noexcept { return m_items.size(); }

private:
    struct ItemEntity {
        ItemStack stack;
        Vector3   pos{};
        Vector3   vel{};
        float     bobPhase = 0.0f;
        float     age      = 0.0f;
    };

    std::vector<ItemEntity> m_items;
};

} // namespace mc
