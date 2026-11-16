#include "player/Player.hpp"
#include "world/ParticleSystem.hpp"
#include "world/ItemEntityManager.hpp"
#include "world/BlockRegistry.hpp"
#include "core/Config.hpp"
#include <raylib.h>
#include <raymath.h>
#include <cmath>

namespace mc {

bool Player::load() {
    return m_breaker.load();
}

void Player::update(float dt, World& world, bool gameplayEnabled) {
    if (!gameplayEnabled) return;

    for (int i = 0; i < Inventory::HOTBAR; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) m_inventory.select(i);
    }
    m_inventory.scrollSelect((int)GetMouseWheelMove());

    if (m_gameMode == GameMode::Creative) {
        m_camera.update(dt);
        m_feetPos = Vector3Subtract(m_camera.get().position, Vector3{0.0f, m_eyeHeight, 0.0f});
    } else {
        if (!m_physicsInit) {
            m_feetPos = Vector3Subtract(m_camera.get().position, Vector3{0.0f, m_eyeHeight, 0.0f});
            m_physicsInit = true;
        }
        m_camera.updateLookOnly(dt);
        applySurvivalPhysics(dt, world);
    }
}

void Player::applySurvivalPhysics(float dt, World& world) {
    // Gravity (always applied; terminal velocity cap).
    m_velocity.y -= 25.0f * dt;
    if (m_velocity.y < -78.4f) m_velocity.y = -78.4f;

    // Desired horizontal movement (camera yaw -> horizontal basis).
    const float yaw = m_camera.yaw();
    const Vector3 fwd   = { std::sin(yaw), 0.0f, -std::cos(yaw) };
    const Vector3 right = { std::cos(yaw), 0.0f,  std::sin(yaw) };

    Vector3 wish{};
    if (IsKeyDown(KEY_W)) wish = Vector3Add(wish, fwd);
    if (IsKeyDown(KEY_S)) wish = Vector3Subtract(wish, fwd);
    if (IsKeyDown(KEY_D)) wish = Vector3Add(wish, right);
    if (IsKeyDown(KEY_A)) wish = Vector3Subtract(wish, right);

    if (Vector3LengthSqr(wish) > 0.01f) {
        wish = Vector3Normalize(wish);
        m_sprinting = IsKeyDown(KEY_LEFT_SHIFT);
        const float speed = m_sprinting ? 5.6f : 4.3f;
        m_velocity.x = wish.x * speed;
        m_velocity.z = wish.z * speed;
    } else {
        m_velocity.x *= 0.8f;
        m_velocity.z *= 0.8f;
    }

    // Jump (only when grounded).
    if (IsKeyPressed(KEY_SPACE) && m_onGround) {
        m_velocity.y = 8.5f;
        m_onGround = false;
    }

    // Axis-separated movement with AABB collision (Minecraft-style).
    const Vector3 delta = Vector3Scale(m_velocity, dt);

    m_feetPos.y += delta.y;
    if (collidesWithWorld(world)) {
        m_feetPos.y -= delta.y;
        m_onGround = (delta.y < 0.0f);
        m_velocity.y = 0.0f;
    } else {
        m_onGround = false;
    }

    m_feetPos.x += delta.x;
    if (collidesWithWorld(world)) {
        m_feetPos.x -= delta.x;
        m_velocity.x = 0.0f;
    }

    m_feetPos.z += delta.z;
    if (collidesWithWorld(world)) {
        m_feetPos.z -= delta.z;
        m_velocity.z = 0.0f;
    }

    // Sync the camera to the feet position (eye height).
    m_camera.setPosition({ m_feetPos.x, m_feetPos.y + m_eyeHeight, m_feetPos.z });
}

bool Player::collidesWithWorld(const World& world) const {
    const float hw = m_playerWidth * 0.5f;
    const int x0 = (int)std::floor(m_feetPos.x - hw);
    const int x1 = (int)std::floor(m_feetPos.x + hw);
    const int y0 = (int)std::floor(m_feetPos.y);
    const int y1 = (int)std::floor(m_feetPos.y + m_playerHeight);
    const int z0 = (int)std::floor(m_feetPos.z - hw);
    const int z1 = (int)std::floor(m_feetPos.z + hw);

    for (int x = x0; x <= x1; ++x)
        for (int y = y0; y <= y1; ++y)
            for (int z = z0; z <= z1; ++z)
                if (BlockRegistry::get(world.getBlock(x, y, z)).isSolid) return true;
    return false;
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
