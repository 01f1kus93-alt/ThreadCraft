#pragma once
#include "core/Types.hpp"
#include <raylib.h>
#include <vector>

namespace mc {

// Block-break / mining-hit particles. Rendered as small cubes (fallback
// since vanilla 1.20.1 has no combined particles.png).
class ParticleSystem {
public:
    void spawnBlockBreak(Vector3 center, BlockID id);
    void spawnMiningHit(Vector3 center, BlockID id);
    void update(float dt);
    void draw3D(const Camera3D& cam) const;

private:
    struct Particle {
        Vector3 pos{};
        Vector3 vel{};
        Color   color{};
        float   life    = 0.0f;
        float   maxLife = 0.0f;
        float   size    = 0.08f;
    };

    void capParticles();

    std::vector<Particle> m_particles;
};

} // namespace mc
