#pragma once
#include "core/Config.hpp"
#include "core/Types.hpp"
#include <array>
#include <atomic>
#include <raylib.h>

namespace mc {

enum class ChunkState : uint8_t {
    Empty, Generating, Generated, Meshing, MeshReady, Uploaded
};

class Chunk {
public:
    explicit Chunk(ChunkCoord coord);

    ChunkCoord coord() const noexcept { return m_coord; }

    BlockID getFast(int lx, int ly, int lz) const noexcept {
        return m_blocks[(ly * config::CHUNK_SIZE_Z + lz) * config::CHUNK_SIZE_X + lx];
    }
    
    void set(int lx, int ly, int lz, BlockID id) noexcept {
        if (lx < 0 || lx >= config::CHUNK_SIZE_X || ly < 0 || ly >= config::CHUNK_SIZE_Y || lz < 0 || lz >= config::CHUNK_SIZE_Z) return;
        m_blocks[(ly * config::CHUNK_SIZE_Z + lz) * config::CHUNK_SIZE_X + lx] = id;
        if (id != BLOCK_AIR) {
            hasBlocks = true;
            if (ly > maxY) maxY = ly;
            const int ci = lz * config::CHUNK_SIZE_X + lx;
            if ((int8_t)ly > columnTop[ci]) columnTop[ci] = (int8_t)ly;
        }
    }

    std::atomic<ChunkState> state{ChunkState::Empty};
    std::atomic<bool> neighborDirty{false};

    int     maxY      = 0;
    bool    hasBlocks = false;
    std::array<int8_t, config::CHUNK_SIZE_X * config::CHUNK_SIZE_Z> columnTop{};

    // Для Frustum Culling
    Vector3 boxMin{};
    Vector3 boxMax{};

    Mesh opaqueMesh{};
    bool hasOpaqueMesh = false;
    Mesh transparentMesh{};
    bool hasTransparentMesh = false;

private:
    ChunkCoord m_coord;
    std::array<BlockID, config::CHUNK_VOLUME> m_blocks{};
};

} // namespace mc