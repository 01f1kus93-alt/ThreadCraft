#pragma once
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
};

} // namespace mc