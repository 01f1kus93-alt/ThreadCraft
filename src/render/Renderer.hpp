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

    void setVisibleLeafLayers(int layers);
    void beginFrame(const Camera3D& cam, int renderDistanceChunks);
    void drawWorld(const World& world) const;
    void drawSelection(const RaycastResult& hit) const;
    void drawCrosshair() const;
    void drawDebugInfo(const World& world, const Camera3D& cam, const ItemStack& selected) const;
    void drawFrameGraph() const;
    private:
    static constexpr int GRAPH_SIZE = 120;

    Shader   m_shader{};
    Material m_chunkMat{};
    Frustum  m_frustum;
    int      m_mvpLoc = -1;
    int      m_fogStartLoc = -1;
    int      m_fogEndLoc   = -1;
    int      m_fogColorLoc = -1;
    int      m_camPosLoc   = -1;
    int      m_leavesLodLoc = -1;
    int      m_visibleLeafLayersLoc = -1;
    int      m_visibleLeafLayers = config::LEAVES_VISIBLE_LAYERS_DEFAULT;
    float    m_fogEnd = 384.0f;   // cached fog end (world blocks) for distance culling
    Vector3  m_camPos{};   // cached camera position for chunk culling

    std::array<float, GRAPH_SIZE> m_frameTimes{};
    int m_frameIndex = 0;

    bool isChunkVisible(Vector3 boxMin, Vector3 boxMax) const;
};

} // namespace mc
