#include "player/BlockBreaker.hpp"
#include "world/World.hpp"
#include "world/ParticleSystem.hpp"
#include "world/BlockRegistry.hpp"
#include "core/Config.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <rlgl.h>

namespace mc {

namespace {
    // Draw a full-texture quad on all 6 cube faces via rlgl primitives, with
    // CCW-outward winding (verified by cross-product normal check).
    void drawCrackCube(const Texture2D& tex, Vector3 c, float size) {
        const float h = size * 0.5f;
        rlDisableShader();               // ensure default shader after greedy pass
        rlSetTexture(tex.id);
        rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);

        // +X (normal → +X)
        rlTexCoord2f(0, 0); rlVertex3f(c.x + h, c.y - h, c.z + h);
        rlTexCoord2f(1, 0); rlVertex3f(c.x + h, c.y - h, c.z - h);
        rlTexCoord2f(1, 1); rlVertex3f(c.x + h, c.y + h, c.z - h);
        rlTexCoord2f(0, 1); rlVertex3f(c.x + h, c.y + h, c.z + h);
        // -X (normal → -X)
        rlTexCoord2f(0, 0); rlVertex3f(c.x - h, c.y - h, c.z - h);
        rlTexCoord2f(1, 0); rlVertex3f(c.x - h, c.y - h, c.z + h);
        rlTexCoord2f(1, 1); rlVertex3f(c.x - h, c.y + h, c.z + h);
        rlTexCoord2f(0, 1); rlVertex3f(c.x - h, c.y + h, c.z - h);
        // +Y (normal → +Y)
        rlTexCoord2f(0, 0); rlVertex3f(c.x - h, c.y + h, c.z + h);
        rlTexCoord2f(1, 0); rlVertex3f(c.x + h, c.y + h, c.z + h);
        rlTexCoord2f(1, 1); rlVertex3f(c.x + h, c.y + h, c.z - h);
        rlTexCoord2f(0, 1); rlVertex3f(c.x - h, c.y + h, c.z - h);
        // -Y (normal → -Y)
        rlTexCoord2f(0, 0); rlVertex3f(c.x - h, c.y - h, c.z - h);
        rlTexCoord2f(1, 0); rlVertex3f(c.x + h, c.y - h, c.z - h);
        rlTexCoord2f(1, 1); rlVertex3f(c.x + h, c.y - h, c.z + h);
        rlTexCoord2f(0, 1); rlVertex3f(c.x - h, c.y - h, c.z + h);
        // +Z (normal → +Z)
        rlTexCoord2f(0, 0); rlVertex3f(c.x - h, c.y - h, c.z + h);
        rlTexCoord2f(1, 0); rlVertex3f(c.x + h, c.y - h, c.z + h);
        rlTexCoord2f(1, 1); rlVertex3f(c.x + h, c.y + h, c.z + h);
        rlTexCoord2f(0, 1); rlVertex3f(c.x - h, c.y + h, c.z + h);
        // -Z
        rlTexCoord2f(0, 0); rlVertex3f(c.x + h, c.y - h, c.z - h);
        rlTexCoord2f(1, 0); rlVertex3f(c.x - h, c.y - h, c.z - h);
        rlTexCoord2f(1, 1); rlVertex3f(c.x - h, c.y + h, c.z - h);
        rlTexCoord2f(0, 1); rlVertex3f(c.x + h, c.y + h, c.z - h);

        rlEnd();
        rlSetTexture(0);
    }
}

bool BlockBreaker::load() {
    bool ok = true;
    for (int i = 0; i < 10; ++i) {
        char path[96];
        snprintf(path, sizeof(path), "assets/minecraft/textures/block/destroy_stage_%d.png", i);
        m_crackTex[i] = LoadTexture(path);
        if (m_crackTex[i].id == 0) {
            ok = false;
        } else {
            SetTextureFilter(m_crackTex[i], TEXTURE_FILTER_POINT);
        }
    }
    m_crackLoaded = ok;
    if (!ok) {
        TraceLog(LOG_WARNING, "BlockBreaker: destroy_stage_*.png missing, procedural crack fallback");
        unload();
    }
    return m_crackLoaded;
}

void BlockBreaker::unload() {
    for (Texture2D& t : m_crackTex) {
        if (t.id != 0) UnloadTexture(t);
        t = Texture2D{};
    }
    m_crackLoaded = false;
}

void BlockBreaker::reset() {
    m_active    = false;
    m_progress  = 0.0f;
    m_breakTime = 0.0f;
    m_hitTimer  = 0.0f;
    m_stage     = 0;
    m_blockId   = BLOCK_AIR;
}

bool BlockBreaker::update(float dt, bool lmbHeld, const RaycastResult& hit,
                          World& world, ParticleSystem& particles) {
    if (!lmbHeld || !hit.hit) { reset(); return false; }

    const BlockID currentId = world.getBlock(hit.wx, hit.wy, hit.wz);
    if (currentId == BLOCK_AIR) { reset(); return false; }

    const BlockType& bt = BlockRegistry::get(currentId);
    if (bt.hardness < 0.0f) { reset(); return false; }   // bedrock / water

    if (!m_active || m_tx != hit.wx || m_ty != hit.wy || m_tz != hit.wz) {
        m_active    = true;
        m_tx        = hit.wx;
        m_ty        = hit.wy;
        m_tz        = hit.wz;
        m_blockId   = currentId;
        m_progress  = 0.0f;
        m_breakTime = std::max(0.05f, bt.hardness * config::BREAK_TIME_MULT);
        m_hitTimer  = 0.0f;
    }

    m_progress += dt / m_breakTime;
    m_hitTimer += dt;
    m_stage     = std::min(9, (int)(m_progress * 10.0f));

    // Mining-hit particles every ~0.15 s
    if (m_hitTimer >= 0.15f) {
        m_hitTimer = 0.0f;
        particles.spawnMiningHit({ m_tx + 0.5f, m_ty + 0.5f, m_tz + 0.5f }, m_blockId);
    }

    if (m_progress >= 1.0f) {
        m_active   = false;
        m_progress = 0.0f;
        m_stage    = 0;
        return true;
    }
    return false;
}

void BlockBreaker::drawCrack() const {
    if (!m_active) return;
    const Vector3 center = { m_tx + 0.5f, m_ty + 0.5f, m_tz + 0.5f };

    rlDisableDepthMask();
    rlDisableBackfaceCulling();  // crack overlay must be visible from all 6 sides
    if (m_crackLoaded) {
        const int stage = std::clamp(m_stage, 0, 9);
        drawCrackCube(m_crackTex[stage], center, 1.004f);
    } else {
        DrawCube(center, 1.004f, 1.004f, 1.004f, Color{ 0, 0, 0, (uint8_t)(m_stage * 22) });
    }
    rlEnableBackfaceCulling();
    rlEnableDepthMask();
}

} // namespace mc
