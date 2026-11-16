#pragma once
#include "render/Camera.hpp"
#include "ui/Inventory.hpp"
#include "player/BlockBreaker.hpp"
#include "world/World.hpp"   // RaycastResult
#include "core/Types.hpp"    // GameMode

namespace mc {

class ParticleSystem;
class ItemEntityManager;

class Player {
public:
    bool load();

    FlyCamera&       camera()    noexcept { return m_camera; }
    const FlyCamera& camera() const noexcept { return m_camera; }
    Inventory&       inventory() noexcept { return m_inventory; }
    const Inventory& inventory() const noexcept { return m_inventory; }
    BlockBreaker&    breaker()   noexcept { return m_breaker; }
    const BlockBreaker& breaker() const noexcept { return m_breaker; }

    const RaycastResult& lastHit() const noexcept { return m_lastHit; }

    GameMode gameMode() const noexcept { return m_gameMode; }
    void setGameMode(GameMode m) noexcept;
    bool isFlying() const noexcept { return m_flying; }

    // skinTexture() and swingAngle() removed — first-person arm no longer rendered

    float health() const noexcept { return m_health; }
    void  damage(float amount) noexcept;
    void  heal(float amount) noexcept;
    void  respawn() noexcept;

    void update(float dt, World& world, bool gameplayEnabled);
    bool handleInteraction(float dt, World& world, ParticleSystem& particles,
                           ItemEntityManager& items, bool gameplayEnabled);

private:
    void applyPhysics(float dt, World& world);
    bool collidesWithWorld(const World& world) const;
    bool blockIntersectsPlayer(int wx, int wy, int wz) const noexcept;

    FlyCamera    m_camera;
    Inventory    m_inventory;
    BlockBreaker m_breaker;
    RaycastResult m_lastHit{};

    GameMode m_gameMode = GameMode::Survival;
    Vector3  m_feetPos{0.0f, 40.0f, 0.0f};
    Vector3  m_velocity{0.0f, 0.0f, 0.0f};
    bool     m_onGround = false;
    bool     m_sprinting = false;
    bool     m_physicsInit = false;
    float    m_eyeHeight = 1.62f;
    float    m_playerHeight = 1.8f;
    float    m_playerWidth = 0.6f;

    // m_skinTex, m_swingAngle, m_swingProgress, m_swingCooldown removed

    float m_health = 20.0f;
    float m_fallDistance = 0.0f;

    // ── Flight / mode mechanics ──
    bool  m_flying = false;
    float m_spaceTapTimer = 0.0f;
    static constexpr float DOUBLE_TAP_WINDOW = 0.3f;
};

} // namespace mc
