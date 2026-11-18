#pragma once
#include "core/Config.hpp"
#include "core/Types.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
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
        const int sy = ly / config::SUBSECTION_SIZE_Y;
        const int iy = ly % config::SUBSECTION_SIZE_Y;
        const auto& sec = m_sections[sy];
        if (!sec) return BLOCK_AIR; // секция целиком из воздуха — память не выделена
        return (*sec)[(iy * config::CHUNK_SIZE_Z + lz) * config::CHUNK_SIZE_X + lx];
    }
    
    void set(int lx, int ly, int lz, BlockID id) noexcept {
        if (lx < 0 || lx >= config::CHUNK_SIZE_X || ly < 0 || ly >= config::CHUNK_SIZE_Y || lz < 0 || lz >= config::CHUNK_SIZE_Z) return;
        const int sy = ly / config::SUBSECTION_SIZE_Y;
        const int iy = ly % config::SUBSECTION_SIZE_Y;
        const int idx = (iy * config::CHUNK_SIZE_Z + lz) * config::CHUNK_SIZE_X + lx;

        if (id != BLOCK_AIR) {
            auto& sec = m_sections[sy];
            if (!sec) {
                sec = std::make_unique<Section>();
                sec->fill(BLOCK_AIR);
            }
            if ((*sec)[idx] == BLOCK_AIR) ++m_sectionCount[sy];
            (*sec)[idx] = id;

            hasBlocks = true;
            if (ly > maxY) maxY = ly;
            const int ci = lz * config::CHUNK_SIZE_X + lx;
            if ((int8_t)ly > columnTop[ci]) columnTop[ci] = (int8_t)ly;
        } else {
            auto& sec = m_sections[sy];
            if (sec && (*sec)[idx] != BLOCK_AIR) {
                (*sec)[idx] = BLOCK_AIR;
                if (m_sectionCount[sy] > 0) --m_sectionCount[sy];
                if (m_sectionCount[sy] == 0) sec.reset(); // секция снова пустая — освобождаем
            }
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
    using Section = std::array<BlockID, config::SUBSECTION_VOLUME>;

    ChunkCoord m_coord;
    std::array<std::unique_ptr<Section>, config::CHUNK_SUBSECTIONS> m_sections{};
    std::array<uint16_t, config::CHUNK_SUBSECTIONS> m_sectionCount{};
};

} // namespace mc