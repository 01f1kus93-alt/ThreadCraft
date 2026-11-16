#include "world/ItemEntityManager.hpp"
#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "ui/Inventory.hpp"
#include "render/TextureAtlas.hpp"
#include "core/Config.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

namespace {
    float randf(float a, float b) {
        return a + (float)GetRandomValue(0, 100000) / 100000.0f * (b - a);
    }
}

void ItemEntityManager::spawn(Vector3 pos, BlockID id, int count) {
    if (id == BLOCK_AIR || count <= 0) return;

    // Drop the oldest item when at capacity.
    while ((int)m_items.size() >= config::MAX_ITEM_ENTITIES) {
        m_items.erase(m_items.begin());
    }

    ItemEntity e;
    e.stack = ItemStack{ id, (uint8_t)std::min(count, config::STACK_MAX) };
    e.pos   = pos;
    e.vel   = { randf(-1.0f, 1.0f), randf(1.5f, 3.0f), randf(-1.0f, 1.0f) };
    e.bobPhase = randf(0.0f, 6.283f);
    m_items.push_back(e);
}

void ItemEntityManager::update(float dt, const Vector3& playerPos, World& world, Inventory& inv) {
    for (ItemEntity& e : m_items) {
        e.age      += dt;
        e.bobPhase += dt * 3.0f;

        e.vel.y += config::ITEM_GRAVITY * dt;
        e.pos.x += e.vel.x * dt;
        e.pos.y += e.vel.y * dt;
        e.pos.z += e.vel.z * dt;

        // Ground collision: rest on top of a solid block below.
        const int fx = (int)std::floor(e.pos.x);
        const int fy = (int)std::floor(e.pos.y - 0.05f);
        const int fz = (int)std::floor(e.pos.z);
        if (fy >= 0) {
            const BlockType& below = BlockRegistry::get(world.getBlock(fx, fy, fz));
            if (below.isSolid && e.vel.y <= 0.0f) {
                e.pos.y = (float)(fy + 1);
                e.vel   = Vector3{ 0.0f, 0.0f, 0.0f };
            }
        }

        // Pickup
        if (e.age >= config::ITEM_PICKUP_DELAY) {
            const float dx = playerPos.x - e.pos.x;
            const float dy = playerPos.y - e.pos.y;
            const float dz = playerPos.z - e.pos.z;
            const float r2 = config::ITEM_PICKUP_R * config::ITEM_PICKUP_R;
            if (dx*dx + dy*dy + dz*dz <= r2) {
                const int leftover = inv.mergeInto(e.stack);
                if (leftover <= 0) e.stack.clear();
                else               e.stack.count = (uint8_t)leftover;
            }
        }
    }

    // Remove picked-up and despawned items (age > 300 s).
    m_items.erase(
        std::remove_if(m_items.begin(), m_items.end(),
                       [](const ItemEntity& e) { return e.stack.isEmpty() || e.age > 300.0f; }),
        m_items.end());
}

void ItemEntityManager::draw3D(const Camera3D& /*cam*/, const TextureAtlas& atlas) const {
    for (const ItemEntity& e : m_items) {
        const BlockType& bt = BlockRegistry::get(e.stack.id);
        const int tile = bt.faceTiles[(int)Face::PosY];
        const Color c = atlas.centerColor(tile);

        const float yOff = std::sin(e.bobPhase) * 0.05f;
        const Vector3 pos = { e.pos.x, e.pos.y + 0.15f + yOff, e.pos.z };
        DrawCube(pos, 0.25f, 0.25f, 0.25f, c);
        DrawCubeWires(pos, 0.251f, 0.251f, 0.251f, Color{ 40, 40, 40, 200 });
    }
}

} // namespace mc
