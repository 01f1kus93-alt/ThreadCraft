#pragma once
#include "core/Types.hpp"
#include <raylib.h>

namespace mc {

struct RaycastResult;
class World;
class ParticleSystem;

class BlockBreaker {
public:
    bool load();
    void unload();

    // Returns true on the frame the block finishes breaking.
    bool update(float dt, bool lmbHeld, const RaycastResult& hit,
                World& world, ParticleSystem& particles);
    void reset();
    void drawCrack() const;

private:
    bool    m_active = false;
    int     m_tx = 0, m_ty = 0, m_tz = 0;
    BlockID m_blockId = BLOCK_AIR;
    float   m_progress  = 0.0f;
    float   m_breakTime = 0.0f;
    float   m_hitTimer  = 0.0f;
    int     m_stage     = 0;

    Texture2D m_crackTex[10]{};
    bool      m_crackLoaded = false;
};

} // namespace mc
