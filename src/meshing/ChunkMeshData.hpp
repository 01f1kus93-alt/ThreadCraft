#pragma once
#include "core/Config.hpp"
#include <vector>
#include <cstdint>

namespace mc {

struct ChunkMeshData {
    std::vector<float>          vertices;
    std::vector<float>          normals;
    std::vector<float>          texcoords;
    std::vector<float>          texcoords2; // 2 float на вершину: (leafDepth, 0)
    std::vector<unsigned char>  colors;    // 4 байта (RGBA) на вершину
    std::vector<unsigned short> indices;

    bool isEmpty() const noexcept { return vertices.empty(); }
};

struct ChunkMeshes {
    ChunkMeshData opaque;
    ChunkMeshData transparent;

    // Минимальная/максимальная высота (в локальных координатах чанка, 0..255)
    // Вычисляется в GreedyMeshBuilder::build() по вершинам меша.
    // Используется для «тугого» AABB при frustum culling.
    int yMinLocal = 0;
    int yMaxLocal = config::CHUNK_SIZE_Y - 1;
};

} // namespace mc