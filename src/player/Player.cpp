#include "player/Player.hpp"
#include "world/ParticleSystem.hpp"
#include "world/ItemEntityManager.hpp"
#include "world/BlockRegistry.hpp"
#include "core/Config.hpp"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>

namespace mc {

bool Player::load() {
    return m_breaker.load();
}
void Player::setGameMode(GameMode m) noexcept {
    m_gameMode = m;
    m_velocity    = {0.0f, 0.0f, 0.0f};
    m_fallDistance = 0.0f;
    m_flying      = (m == GameMode::Spectator);
}

void Player::update(float dt, World& world, bool gameplayEnabled) {
    if (!gameplayEnabled) return;

    // One-time feet position sync from camera
    if (!m_physicsInit) {
        m_feetPos = Vector3Subtract(m_camera.get().position, Vector3{0.0f, m_eyeHeight, 0.0f});
        m_physicsInit = true;
    }

    // Hotbar selection
    for (int i = 0; i < Inventory::HOTBAR; ++i) {
        if (IsKeyPressed(KEY_ONE + i)) m_inventory.select(i);
    }
    m_inventory.scrollSelect((int)GetMouseWheelMove());

    // Double-space flight toggle (works in all modes)
    if (m_spaceTapTimer > 0.0f) m_spaceTapTimer -= dt;
    if (IsKeyPressed(KEY_SPACE)) {
        if (m_spaceTapTimer > 0.0f) {
            m_flying = !m_flying;
            m_spaceTapTimer = 0.0f;
            if (m_flying) m_velocity.y = 0.0f;  // cancel fall
        } else {
            m_spaceTapTimer = DOUBLE_TAP_WINDOW;
        }
    }

    // Look only (movement is handled by applyPhysics)
    m_camera.updateLookOnly(dt);

    // Unified physics for all modes
    applyPhysics(dt, world);

    // Death → respawn
    if (m_health <= 0.0f) respawn();
}

void Player::applyPhysics(float dt, World& world) {
    const bool spectator = (m_gameMode == GameMode::Spectator);
    const bool flying    = m_flying;
    const bool noclip    = spectator; // Spectator always ignores collisions

    // Gravity (only when not flying).
    if (!flying) {
        m_velocity.y -= 25.0f * dt;
        if (m_velocity.y < -78.4f) m_velocity.y = -78.4f;
    }

    // Horizontal wish (camera yaw → horizontal basis).
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
        const float speed = flying ? 15.0f : (m_sprinting ? 5.6f : 4.3f);
        m_velocity.x = wish.x * speed;
        m_velocity.z = wish.z * speed;
    } else {
        m_velocity.x *= (flying ? 0.6f : 0.8f);
        m_velocity.z *= (flying ? 0.6f : 0.8f);
    }

    // Vertical control
    if (flying) {
        float vy = 0.0f;
        if (IsKeyDown(KEY_SPACE))      vy += 1.0f;
        if (IsKeyDown(KEY_LEFT_SHIFT)) vy -= 1.0f;
        m_velocity.y = vy * 15.0f;
        m_sprinting = false;
    } else {
        m_sprinting = IsKeyDown(KEY_LEFT_SHIFT);
        if (IsKeyPressed(KEY_SPACE) && m_onGround) {
            m_velocity.y = 8.5f;
            m_onGround = false;
        }
    }

    // Movement (with or without collision)
    const Vector3 delta = Vector3Scale(m_velocity, dt);

    if (noclip) {
        m_feetPos = Vector3Add(m_feetPos, delta);
        m_onGround = false;
        m_fallDistance = 0.0f;
    } else {
        // Y-axis
        m_feetPos.y += delta.y;
        if (collidesWithWorld(world)) {
            m_feetPos.y -= delta.y;
            if (flying) {
                // Do NOT zero velocity while flying (req #5)
            } else {
                const bool landed = (delta.y < 0.0f);
                if (landed) {
                    if (m_gameMode == GameMode::Survival && m_fallDistance > 3.0f)
                        damage(m_fallDistance - 3.0f);
                    m_fallDistance = 0.0f;
                }
                m_onGround = landed;
                m_velocity.y = 0.0f;
            }
        } else {
            m_onGround = false;
            if (!flying && m_velocity.y < 0.0f)
                m_fallDistance += -m_velocity.y * dt;
        }

        // X-axis
        m_feetPos.x += delta.x;
        if (collidesWithWorld(world)) {
            m_feetPos.x -= delta.x;
            m_velocity.x = 0.0f;
        }

        // Z-axis
        m_feetPos.z += delta.z;
        if (collidesWithWorld(world)) {
            m_feetPos.z -= delta.z;
            m_velocity.z = 0.0f;
        }
    }

    // Sync camera to feet position
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

void Player::damage(float amount) noexcept {
    m_health -= amount;
    if (m_health < 0.0f) m_health = 0.0f;
}

void Player::heal(float amount) noexcept {
    m_health = std::min(m_health + amount, 20.0f);
}

void Player::respawn() noexcept {
    m_health = 20.0f;
    m_feetPos.y = 100.0f;
    m_velocity = {0.0f, 0.0f, 0.0f};
    m_fallDistance = 0.0f;
    m_camera.setPosition({ m_feetPos.x, m_feetPos.y + m_eyeHeight, m_feetPos.z });
}

bool Player::blockIntersectsPlayer(int wx, int wy, int wz) const noexcept {
    const float hw = m_playerWidth * 0.5f;
    const float minX = m_feetPos.x - hw;
    const float maxX = m_feetPos.x + hw;
    const float minY = m_feetPos.y;
    const float maxY = m_feetPos.y + m_playerHeight;
    const float minZ = m_feetPos.z - hw;
    const float maxZ = m_feetPos.z + hw;
    // Block occupies [wx, wx+1] × [wy, wy+1] × [wz, wz+1]
    return (wx + 1.0f > minX && wx < maxX) &&
           (wy + 1.0f > minY && wy < maxY) &&
           (wz + 1.0f > minZ && wz < maxZ);
}

bool Player::handleInteraction(float dt, World& world, ParticleSystem& particles,
                               ItemEntityManager& items, bool gameplayEnabled) {
    // Crosshair ray, computed once per frame.
    const int cx = GetScreenWidth() / 2;
    const int cy = GetScreenHeight() / 2;
    const Ray ray = GetScreenToWorldRay({ (float)cx, (float)cy }, m_camera.get());
    m_lastHit = world.raycast(ray.position, ray.direction, 8.0f);

    const RaycastResult& hit = m_lastHit;

    if (!gameplayEnabled) {
        m_breaker.reset();
        return false;
    }

    const bool creative = (m_gameMode == GameMode::Creative);
    bool broke = false;

    // LMB: break. Creative = instant (no drop); Survival = hold-to-break + drop.
    if (creative) {
        m_breaker.reset();
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hit.hit) {
            const BlockID id = world.getBlock(hit.wx, hit.wy, hit.wz);
            if (id != BLOCK_AIR && BlockRegistry::get(id).hardness >= 0.0f) {
                world.setBlock(hit.wx, hit.wy, hit.wz, BLOCK_AIR);
                particles.spawnBlockBreak({ hit.wx + 0.5f, hit.wy + 0.5f, hit.wz + 0.5f }, id);
            }
        }
    } else {
        broke = m_breaker.update(dt, IsMouseButtonDown(MOUSE_BUTTON_LEFT), hit, world, particles);
        if (broke) {
            const BlockID brokenId = world.getBlock(hit.wx, hit.wy, hit.wz);
            const Vector3 center = { hit.wx + 0.5f, hit.wy + 0.5f, hit.wz + 0.5f };
            world.setBlock(hit.wx, hit.wy, hit.wz, BLOCK_AIR);
            particles.spawnBlockBreak(center, brokenId);
            items.spawn(center, brokenId, 1);
        }
    }

    // RMB: place the selected block (press only). Creative doesn't consume the stack.
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && hit.hit) {
        const ItemStack sel = m_inventory.hotbar(m_inventory.selected());
        if (!sel.isEmpty()) {
            const int px = hit.wx + hit.nx;
            const int py = hit.wy + hit.ny;
            const int pz = hit.wz + hit.nz;
            const BlockID target = world.getBlock(px, py, pz);
            const bool replaceable = (target == BLOCK_AIR) || !BlockRegistry::get(target).isSolid;
            if (replaceable && !blockIntersectsPlayer(px, py, pz)) {
                world.setBlock(px, py, pz, sel.id);
                if (!creative) {
                    ItemStack& s = m_inventory.hotbar(m_inventory.selected());
                    if (--s.count == 0) s.clear();
                }
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

    return broke;
}

} // namespace mc
