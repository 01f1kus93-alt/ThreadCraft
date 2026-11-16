#pragma once
#include "render/TextureAtlas.hpp"
#include "world/World.hpp"
#include "utils/Frustum.hpp"
#include "ui/ItemStack.hpp"
#include <raylib.h>
#include <array>

namespace mc {

// Owns the greedy shader/material, frustum culling and 2D HUD drawing.
class Renderer {
public:
    bool load(const TextureAtlas& atlas);
    void unload();

    void beginFrame(const Camera3D& cam);
    void drawWorld(const World& world) const;
    void drawSelection(const RaycastResult& hit) const;
    void drawCrosshair() const;
    void drawDebugInfo(const World& world, const Camera3D& cam, const ItemStack& selected) const;
    void drawFrameGraph() const;
    void drawPlayerArm(const Texture2D& tex, float swingAngle) const;

private:
    static constexpr int GRAPH_SIZE = 120;

    Shader   m_shader{};
    Material m_chunkMat{};
    Frustum  m_frustum;
    int      m_mvpLoc = -1;
    Vector3  m_camPos{};   // cached camera position for chunk culling

    std::array<float, GRAPH_SIZE> m_frameTimes{};
    int m_frameIndex = 0;

    bool isChunkVisible(Vector3 boxMin, Vector3 boxMax) const;
};

} // namespace mc
