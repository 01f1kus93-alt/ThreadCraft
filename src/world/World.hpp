#pragma once
#include "core/Types.hpp"
#include "core/ThreadPool.hpp"
#include "world/Chunk.hpp"
#include "world/WorldGenerator.hpp"
#include "meshing/ChunkMeshData.hpp"
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <queue>
#include <mutex>
#include <vector>

namespace mc {

class TextureAtlas;
class GreedyMeshBuilder;

struct ChunkUploadTask {
    ChunkCoord coord;
    std::unique_ptr<ChunkMeshes> data;
    int priority;

    bool operator<(const ChunkUploadTask& other) const {
        return priority > other.priority;
    }
};

using ChunkPtr = std::shared_ptr<Chunk>;

struct RaycastResult {
    bool hit = false;
    int wx = 0, wy = 0, wz = 0;
    int nx = 0, ny = 0, nz = 0;
};

class World {
public:
    World(uint32_t seed, int numThreads);
    ~World();

    BlockID getBlock(int wx, int wy, int wz) const;
    RaycastResult raycast(Vector3 start, Vector3 dir, float maxDist) const;
    void setBlock(int wx, int wy, int wz, BlockID id);
    void update(Vector3 playerPos);
    void processUploads(const TextureAtlas& atlas);

    const auto& chunks() const noexcept { return m_chunks; }
    size_t chunkCount() const;

private:
    void requestChunksAround(ChunkCoord playerChunk, const Vector3* velocityDir);
    void requestMesh(ChunkCoord coord);
    void unloadDistantChunks(ChunkCoord playerChunk);

    std::unordered_map<int64_t, ChunkPtr> m_chunks;
    mutable std::shared_mutex m_chunksMutex;

    WorldGenerator m_generator;
    ThreadPool     m_pool;

    std::priority_queue<ChunkUploadTask> m_uploadQueue;
    std::mutex m_uploadMutex;

    std::unique_ptr<GreedyMeshBuilder> m_mesher;

    ChunkCoord m_lastPlayerChunk{0, 0, 0};
    Vector3    m_lastPlayerPos{0, 0, 0};
    bool       m_firstUpdate  = true;
    float      m_updateTimer  = 0.0f;
};

} // namespace mc