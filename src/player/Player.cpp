#include "player/Player.hpp"
#include "world/ParticleSystem.hpp"
#include "world/ItemEntityManager.hpp"
#include "world/BlockRegistry.hpp"
#include "core/Config.hpp"
#include <raylib.h>

namespace mc {

bool Player::load() {
    return m_breaker.load();
}

void Player::update(float dt, bool gameplay) {
    if (!gameplay) return;

    m_camera.update(dt);

    for (int i = 0; i < Inventory::HOTBAR; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) m_inventory.select(i);
    }
    m_inventory.scrollSelect((int)GetMouseWheelMove());
}

void Player::handleInteraction(float dt, World& world, ParticleSystem& particles,
                               ItemEntityManager& items) {
    // Crosshair ray, computed once per frame.
    const int cx = GetScreenWidth() / 2;
    const int cy = GetScreenHeight() / 2;
    const Ray ray = GetScreenToWorldRay({ (float)cx, (float)cy }, m_camera.get());
    m_lastHit = world.raycast(ray.position, ray.direction, 8.0f);

    const RaycastResult& hit = m_lastHit;

    // LMB: hold to break (progress-based).
    const bool lmb = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool broke = m_breaker.update(dt, lmb, hit, world, particles);
    if (broke) {
        const BlockID brokenId = world.getBlock(hit.wx, hit.wy, hit.wz);
        const Vector3 center = { hit.wx + 0.5f, hit.wy + 0.5f, hit.wz + 0.5f };
        world.setBlock(hit.wx, hit.wy, hit.wz, BLOCK_AIR);
        particles.spawnBlockBreak(center, brokenId);
        items.spawn(center, brokenId, 1);
    }

    // RMB: place the selected block (press only).
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hit.hit) {
        const ItemStack sel = m_inventory.hotbar(m_inventory.selected());
        if (!sel.isEmpty()) {
            const int px = hit.wx + hit.nx;
            const int py = hit.wy + hit.ny;
            const int pz = hit.wz + hit.nz;
            const BlockID target = world.getBlock(px, py, pz);
            const bool replaceable = (target == BLOCK_AIR) || !BlockRegistry::get(target).isSolid;
            if (replaceable) {
                world.setBlock(px, py, pz, sel.id);
                ItemStack& s = m_inventory.hotbar(m_inventory.selected());
                if (--s.count == 0) s.clear();
            }
        }
    }

    // MMB: pick targeted block into the selected hotbar slot (full stack).
    if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) && hit.hit) {
        const BlockID id = world.getBlock(hit.wx, hit.wy, hit.wz);
        if (id != BLOCK_AIR) {
            m_inventory.hotbar(m_inventory.selected()) =
                ItemStack{ id, (uint8_t)config::STACK_MAX };
        }
    }
}

} // namespace mc
