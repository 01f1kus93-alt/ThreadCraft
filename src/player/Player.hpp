#pragma once
#include "render/Camera.hpp"
#include "ui/Inventory.hpp"
#include "player/BlockBreaker.hpp"
#include "world/World.hpp"   // RaycastResult

namespace mc {

class ParticleSystem;
class ItemEntityManager;

class Player {
public:
    bool load();   // loads breaker crack textures

    FlyCamera&       camera()    noexcept { return m_camera; }
    const FlyCamera& camera() const noexcept { return m_camera; }
    Inventory&       inventory() noexcept { return m_inventory; }
    const Inventory& inventory() const noexcept { return m_inventory; }
    BlockBreaker&    breaker()   noexcept { return m_breaker; }
    const BlockBreaker& breaker() const noexcept { return m_breaker; }

    const RaycastResult& lastHit() const noexcept { return m_lastHit; }

    void update(float dt, bool gameplay);
    void handleInteraction(float dt, World& world, ParticleSystem& particles,
                           ItemEntityManager& items);

private:
    FlyCamera    m_camera;
    Inventory    m_inventory;
    BlockBreaker m_breaker;
    RaycastResult m_lastHit{};
};

} // namespace mc
