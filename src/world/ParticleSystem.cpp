#include "world/ParticleSystem.hpp"
#include "world/BlockRegistry.hpp"
#include "core/Config.hpp"
#include "render/TextureAtlas.hpp"
#include <algorithm>
#include <raymath.h>
#include <rlgl.h>

namespace mc {

namespace {
    float randf(float a, float b) {
        return a + (float)GetRandomValue(0, 100000) / 100000.0f * (b - a);
    }

    Color jitterColor(Color base, int amount) {
        const int dr = GetRandomValue(-amount, amount);
        const int dg = GetRandomValue(-amount, amount);
        const int db = GetRandomValue(-amount, amount);
        return {
            (uint8_t)std::clamp((int)base.r + dr, 0, 255),
            (uint8_t)std::clamp((int)base.g + dg, 0, 255),
            (uint8_t)std::clamp((int)base.b + db, 0, 255),
            255
        };
    }
}

void ParticleSystem::spawnBlockBreak(Vector3 center, BlockID id) {
    const BlockType& bt = BlockRegistry::get(id);
    const int tile = bt.faceTiles[(int)Face::PosY];
    const Color base = g_textureAtlas ? g_textureAtlas->centerColor(tile)
                                      : Color{ 180, 180, 180, 255 };

    for (int i = 0; i < 10; ++i) {
        Particle p;
        p.pos = { center.x + randf(-0.4f, 0.4f),
                  center.y + randf(-0.4f, 0.4f),
                  center.z + randf(-0.4f, 0.4f) };
        p.vel = { randf(-2.0f, 2.0f), randf(1.0f, 4.0f), randf(-2.0f, 2.0f) };
        p.life = p.maxLife = randf(0.5f, 1.0f);
        p.color = jitterColor(base, 5);
        p.size  = 0.08f;
        m_particles.push_back(p);
    }
    capParticles();
}

void ParticleSystem::spawnMiningHit(Vector3 center, BlockID id) {
    const BlockType& bt = BlockRegistry::get(id);
    const int tile = bt.faceTiles[(int)Face::PosY];
    const Color base = g_textureAtlas ? g_textureAtlas->centerColor(tile)
                                      : Color{ 180, 180, 180, 255 };

    const int n = GetRandomValue(1, 2);
    for (int i = 0; i < n; ++i) {
        Particle p;
        p.pos = { center.x + randf(-0.2f, 0.2f),
                  center.y + randf(-0.2f, 0.2f),
                  center.z + randf(-0.2f, 0.2f) };
        p.vel = { randf(-0.8f, 0.8f), randf(0.5f, 1.5f), randf(-0.8f, 0.8f) };
        p.life = p.maxLife = randf(0.4f, 0.7f);
        p.color = jitterColor(base, 5);
        p.size  = 0.05f;
        m_particles.push_back(p);
    }
    capParticles();
}

void ParticleSystem::update(float dt) {
    for (Particle& p : m_particles) {
        p.vel.y += config::PARTICLE_GRAVITY * dt;
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        p.life -= dt;
    }

    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& p) { return p.life <= 0.0f; }),
        m_particles.end());
}

void ParticleSystem::draw3D(const Camera3D& cam) const {
    (void)cam;
    rlDisableDepthMask();
    for (const Particle& p : m_particles) {
        const float t = p.maxLife > 0.0f ? (p.life / p.maxLife) : 0.0f;
        Color c = p.color;
        c.a = (uint8_t)(t * 255.0f);
        DrawCube(p.pos, p.size, p.size, p.size, c);
    }
    rlEnableDepthMask();
}

void ParticleSystem::capParticles() {
    const int excess = (int)m_particles.size() - config::MAX_PARTICLES;
    if (excess > 0) {
        m_particles.erase(m_particles.begin(), m_particles.begin() + excess);
    }
}

} // namespace mc
