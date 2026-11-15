#pragma once
#include "meshing/ChunkMeshData.hpp"
#include <memory>

namespace mc {
class Chunk;
class TextureAtlas;

class GreedyMeshBuilder {
public:
    ChunkMeshes build(const Chunk& chunk, const std::shared_ptr<Chunk> neighbors[6],
                      const TextureAtlas& atlas) const;
};

} // namespace mc