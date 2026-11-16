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
    void setGameMode(GameMode m) noexcept { m_gameMode = m; m_velocity = {0.0f, 0.0f, 0.0f}; }

    const Texture2D& skinTexture() const noexcept { return m_skinTex; }
    float swingAngle() const noexcept { return m_swingAngle; }

    void update(float dt, World& world, bool gameplayEnabled);
    bool handleInteraction(float dt, World& world, ParticleSystem& particles,
                           ItemEntityManager& items, bool gameplayEnabled);

private:
    void applySurvivalPhysics(float dt, World& world);
    bool collidesWithWorld(const World& world) const;
    void updateSwing(float dt);

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

    Texture2D m_skinTex{};
    float     m_swingAngle = 0.0f;
    float     m_swingProgress = 1.0f;
    float     m_swingCooldown = 0.0f;
};

} // namespace mc
