#include "world/World.hpp"
#include "world/BlockRegistry.hpp"
#include "meshing/GreedyMeshBuilder.hpp"
#include "utils/Math.hpp"
#include "core/Config.hpp"
#include "render/TextureAtlas.hpp"

#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>

namespace mc {

namespace {
    constexpr int DX[6] = {-1, 1, 0, 0, 0, 0};
    constexpr int DY[6] = { 0, 0,-1, 1, 0, 0};
    constexpr int DZ[6] = { 0, 0, 0, 0,-1, 1};
}

World::World(uint32_t seed, int numThreads)
    : m_generator(seed)
    , m_pool(numThreads)
    , m_mesher(std::make_unique<GreedyMeshBuilder>())
{
}

World::~World() {
    {
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        m_uploadStopping = true;
    }
    m_uploadCv.notify_all();
    m_pool.stop();
    for (auto& [key, chunk] : m_chunks) {
        if (chunk->hasOpaqueMesh && chunk->opaqueMesh.vertexCount > 0) UnloadMesh(chunk->opaqueMesh);
        if (chunk->hasTransparentMesh && chunk->transparentMesh.vertexCount > 0) UnloadMesh(chunk->transparentMesh);
    }
}

BlockID World::getBlock(int wx, int wy, int wz) const {
    if (wy < 0) return BLOCK_AIR;
    const ChunkCoord coord = math::worldToChunk(wx, wy, wz);
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(math::chunkKey(coord));
    if (it == m_chunks.end()) return BLOCK_AIR;
    const BlockCoord local = math::worldToLocal(wx, wy, wz);
    return it->second->getFast(local.x, local.y, local.z);
}

RaycastResult World::raycast(Vector3 start, Vector3 dir, float maxDist) const {
    RaycastResult result;

    int x = (int)std::floor(start.x);
    int y = (int)std::floor(start.y);
    int z = (int)std::floor(start.z);

    const int stepX = (dir.x > 0.0f) ? 1 : (dir.x < 0.0f ? -1 : 0);
    const int stepY = (dir.y > 0.0f) ? 1 : (dir.y < 0.0f ? -1 : 0);
    const int stepZ = (dir.z > 0.0f) ? 1 : (dir.z < 0.0f ? -1 : 0);

    constexpr float INF = 1e30f;

    const float tDeltaX = (stepX != 0) ? std::fabs(1.0f / dir.x) : INF;
    const float tDeltaY = (stepY != 0) ? std::fabs(1.0f / dir.y) : INF;
    const float tDeltaZ = (stepZ != 0) ? std::fabs(1.0f / dir.z) : INF;

    float tMaxX = (stepX > 0) ? ((float)(x + 1) - start.x) * tDeltaX
                              : ((stepX < 0) ? (start.x - (float)x) * tDeltaX : INF);
    float tMaxY = (stepY > 0) ? ((float)(y + 1) - start.y) * tDeltaY
                              : ((stepY < 0) ? (start.y - (float)y) * tDeltaY : INF);
    float tMaxZ = (stepZ > 0) ? ((float)(z + 1) - start.z) * tDeltaZ
                              : ((stepZ < 0) ? (start.z - (float)z) * tDeltaZ : INF);

    int lastStepX = 0, lastStepY = 0, lastStepZ = 0;
    float t = 0.0f;

    while (t <= maxDist) {
        const BlockID id = getBlock(x, y, z);
        if (id != BLOCK_AIR) {
            const BlockType& type = BlockRegistry::get(id);
            if (type.isSolid || type.isOpaque) {
                result.hit = true;
                result.wx = x;
                result.wy = y;
                result.wz = z;
                result.nx = -lastStepX;
                result.ny = -lastStepY;
                result.nz = -lastStepZ;
                return result;
            }
        }

        if (tMaxX < tMaxY && tMaxX < tMaxZ) {
            x += stepX;
            t = tMaxX;
            tMaxX += tDeltaX;
            lastStepX = stepX; lastStepY = 0; lastStepZ = 0;
        } else if (tMaxY < tMaxZ) {
            y += stepY;
            t = tMaxY;
            tMaxY += tDeltaY;
            lastStepX = 0; lastStepY = stepY; lastStepZ = 0;
        } else {
            z += stepZ;
            t = tMaxZ;
            tMaxZ += tDeltaZ;
            lastStepX = 0; lastStepY = 0; lastStepZ = stepZ;
        }
    }

    return result;
}

void World::setBlock(int wx, int wy, int wz, BlockID id) {
    if (wy < 0) return;

    const ChunkCoord cc = math::worldToChunk(wx, wy, wz);
    const BlockCoord local = math::worldToLocal(wx, wy, wz);

    ChunkPtr chunk;
    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        auto it = m_chunks.find(math::chunkKey(cc));
        if (it == m_chunks.end()) return;
        chunk = it->second;
    }

    if (chunk->getFast(local.x, local.y, local.z) == id) return;

    chunk->set(local.x, local.y, local.z, id);
    chunk->state = ChunkState::Generated;
    requestMesh(cc);

    const bool border[6] = {
        local.x == 0, local.x == config::CHUNK_SIZE_X - 1,
        local.y == 0, local.y == config::CHUNK_SIZE_Y - 1,
        local.z == 0, local.z == config::CHUNK_SIZE_Z - 1,
    };

    for (int i = 0; i < 6; ++i) {
        if (!border[i]) continue;
        const ChunkCoord nc{cc.x + DX[i], cc.y + DY[i], cc.z + DZ[i]};
        ChunkPtr neighbor;
        {
            std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
            auto it = m_chunks.find(math::chunkKey(nc));
            if (it != m_chunks.end()) neighbor = it->second;
        }
        if (neighbor) {
            neighbor->neighborDirty = true;
            requestMesh(nc);
        }
    }
}

size_t World::chunkCount() const {
    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
    return m_chunks.size();
}

void World::requestChunksAround(ChunkCoord playerChunk, const Vector3* velocityDir) {
    const int R  = m_renderDistance.load();
    const int R2 = R * R;
    constexpr int terrainMaxCy = (config::TERRAIN_BASE_HEIGHT + (int)config::TERRAIN_AMPLITUDE) / config::CHUNK_SIZE_Y;
    const int cyLo = std::max(0, std::min(playerChunk.y - 1, terrainMaxCy));
    const int cyHi = std::max(0, std::min(playerChunk.y + 1, terrainMaxCy));

    std::vector<ChunkCoord> toRequest;
    toRequest.reserve((2 * R + 1) * (2 * R + 1) * (cyHi - cyLo + 1));

    const bool hasVel = velocityDir && (velocityDir->x != 0.0f || velocityDir->z != 0.0f);
    auto score = [&](const ChunkCoord& c) -> float {
        float d2 = (float)math::chunkDistanceSq(c, playerChunk);
        if (hasVel) {
            const float cx = (float)(c.x - playerChunk.x);
            const float cz = (float)(c.z - playerChunk.z);
            const float lenSq = cx * cx + cz * cz;
            if (lenSq > 0.01f) {
                const float dot = (cx * velocityDir->x + cz * velocityDir->z) / sqrtf(lenSq);
                d2 *= (1.5f - 0.5f * dot);
            }
        }
        return d2;
    };

    for (int cy = cyLo; cy <= cyHi; ++cy) {
        for (int dx = -R; dx <= R; ++dx) {
            for (int dz = -R; dz <= R; ++dz) {
                if (dx * dx + dz * dz > R2) continue;
                toRequest.push_back({playerChunk.x + dx, cy, playerChunk.z + dz});
            }
        }
    }

    // Сортируем по приоритету (ближайшие первыми), чтобы при лимите очереди
    // в первую очередь создавались/генерировались видимые чанки.
    std::sort(toRequest.begin(), toRequest.end(),
              [&](const ChunkCoord& a, const ChunkCoord& b) { return score(a) < score(b); });

    const int budget = std::max(0, config::MAX_PENDING_GENERATION - m_pendingGenerations.load());

    std::vector<std::pair<ChunkPtr, ChunkCoord>> created;
    created.reserve((size_t)std::min(budget, (int)toRequest.size()));
    {
        std::unique_lock<std::shared_mutex> lock(m_chunksMutex);
        for (const auto& coord : toRequest) {
            if ((int)created.size() >= budget) break;
            const int64_t key = math::chunkKey(coord);
            if (m_chunks.find(key) != m_chunks.end()) continue;
            auto chunk = std::make_shared<Chunk>(coord);
            chunk->state = ChunkState::Generating;
            m_chunks.emplace(key, chunk);
            created.emplace_back(std::move(chunk), coord);
        }
    }

    m_pendingGenerations.fetch_add((int)created.size());

    for (auto& [chunkPtr, coord] : created) {
        const int prio = (int)score(coord);
        m_pool.enqueue(prio, [this, ch = chunkPtr, coord]() {
            m_generator.generate(*ch, &m_lastPlayerPos);
            ch->state = ChunkState::Generated;
            requestMesh(coord);
            for (int i = 0; i < 6; ++i) {
                const ChunkCoord nc{coord.x + DX[i], coord.y + DY[i], coord.z + DZ[i]};
                ChunkPtr neighbor;
                {
                    std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
                    auto it = m_chunks.find(math::chunkKey(nc));
                    if (it != m_chunks.end()) neighbor = it->second;
                }
                if (neighbor) {
                    neighbor->neighborDirty = true;
                    requestMesh(nc);
                }
            }
            m_pendingGenerations.fetch_sub(1);
        });
    }
}

void World::requestMesh(ChunkCoord coord) {
    ChunkPtr chunk = nullptr;
    ChunkPtr neighbors[6];

    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        auto it = m_chunks.find(math::chunkKey(coord));
        if (it == m_chunks.end()) return;
        chunk = it->second;

        ChunkState expected = chunk->state.load();
        if (expected == ChunkState::Meshing) return;
        if (expected != ChunkState::Generated && expected != ChunkState::MeshReady && expected != ChunkState::Uploaded) return;
        if (expected == ChunkState::Uploaded && !chunk->neighborDirty.load()) return;

        if (!chunk->state.compare_exchange_strong(expected, ChunkState::Meshing)) return;

        // Лимит очереди мешинга: при переполнении откатываем состояние, чанк
        // повторно закажется при следующем обновлении (requestPendingMeshes).
        if (m_pendingMeshes.load() >= config::MAX_PENDING_MESHES) {
            chunk->state.store(expected);
            return;
        }
        m_pendingMeshes.fetch_add(1);

        for (int i = 0; i < 6; ++i) {
            const ChunkCoord nc{coord.x + DX[i], coord.y + DY[i], coord.z + DZ[i]};
            auto nit = m_chunks.find(math::chunkKey(nc));
            if (nit == m_chunks.end()) continue;
            // Не читаем соседа, пока он ещё генерируется (иначе гонка с его set()).
            const ChunkState ns = nit->second->state.load();
            if (ns == ChunkState::Generated || ns == ChunkState::MeshReady || ns == ChunkState::Uploaded)
                neighbors[i] = nit->second;
        }
    }

    const int priority = math::chunkDistanceSq(coord, m_lastPlayerChunk);

    m_pool.enqueue(priority, [this, chunk, coord, neighbors]() {
        ChunkMeshes data = m_mesher->build(*chunk, neighbors, *g_textureAtlas);
        const int up = math::chunkDistanceSq(coord, m_lastPlayerChunk);
        auto payload = std::make_unique<ChunkMeshes>(std::move(data));
        enqueueUpload(coord, std::move(payload), up);
        chunk->state = ChunkState::MeshReady;
        m_pendingMeshes.fetch_sub(1);
        if (chunk->neighborDirty.exchange(false)) {
            requestMesh(coord);
        }
    });
}

void World::enqueueUpload(ChunkCoord coord, std::unique_ptr<ChunkMeshes> data, int priority) {
    std::unique_lock<std::mutex> lock(m_uploadMutex);
    m_uploadCv.wait(lock, [this] {
        return m_uploadStopping || (int)m_uploadQueue.size() < config::MAX_UPLOAD_QUEUE;
    });
    if (m_uploadStopping) return;
    m_uploadQueue.push({coord, std::move(data), priority});
}

void World::requestPendingMeshes() {
    // Дозаказ мешей для чанков, пропущенных из-за лимита очереди мешинга.
    if (m_pendingMeshes.load() >= config::MAX_PENDING_MESHES) return;
    std::vector<ChunkCoord> pending;
    {
        std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
        for (const auto& [key, chunk] : m_chunks) {
            if (chunk->state.load() == ChunkState::Generated)
                pending.push_back(chunk->coord());
        }
    }
    for (const auto& c : pending) requestMesh(c);
}

void World::processUploads(const TextureAtlas&) {
    const auto startTp = std::chrono::steady_clock::now();
    int uploads = 0;

    while (uploads < config::MAX_UPLOADS_PER_FRAME) {
        if ((uploads & 15) == 15) {
            const auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration<float, std::milli>(now - startTp).count() > config::MAX_UPLOAD_TIME_MS) break;
        }

        ChunkCoord coord;
        std::unique_ptr<ChunkMeshes> data;
        {
            std::lock_guard<std::mutex> lock(m_uploadMutex);
            if (m_uploadQueue.empty()) break;
            auto& top = const_cast<ChunkUploadTask&>(m_uploadQueue.top());
            coord = top.coord;
            data  = std::move(top.data);
            m_uploadQueue.pop();
        }
        m_uploadCv.notify_all();

        ChunkPtr chunk;
        {
            std::shared_lock<std::shared_mutex> lock(m_chunksMutex);
            auto it = m_chunks.find(math::chunkKey(coord));
            if (it != m_chunks.end()) chunk = it->second;
        }
        if (!chunk) continue;

        if (chunk->hasOpaqueMesh) { UnloadMesh(chunk->opaqueMesh); chunk->hasOpaqueMesh = false; }
        if (chunk->hasTransparentMesh) { UnloadMesh(chunk->transparentMesh); chunk->hasTransparentMesh = false; }

        if (data && !data->opaque.isEmpty()) {
            Mesh mesh{};
            mesh.vertexCount   = (int)(data->opaque.vertices.size() / 3);
            mesh.triangleCount = (int)(data->opaque.indices.size() / 3);
            mesh.vertices  = (float*)MemAlloc(sizeof(float) * data->opaque.vertices.size());
            mesh.normals   = (float*)MemAlloc(sizeof(float) * data->opaque.normals.size());
            mesh.texcoords = (float*)MemAlloc(sizeof(float) * data->opaque.texcoords.size());
            mesh.texcoords2 = (float*)MemAlloc(sizeof(float) * data->opaque.texcoords2.size());
            mesh.colors    = (unsigned char*)MemAlloc(sizeof(unsigned char) * data->opaque.colors.size());
            mesh.indices   = (unsigned short*)MemAlloc(sizeof(unsigned short) * data->opaque.indices.size());
            memcpy(mesh.vertices,  data->opaque.vertices.data(),  sizeof(float) * data->opaque.vertices.size());
            memcpy(mesh.normals,   data->opaque.normals.data(),   sizeof(float) * data->opaque.normals.size());
            memcpy(mesh.texcoords, data->opaque.texcoords.data(), sizeof(float) * data->opaque.texcoords.size());
            memcpy(mesh.texcoords2, data->opaque.texcoords2.data(), sizeof(float) * data->opaque.texcoords2.size());
            memcpy(mesh.colors,    data->opaque.colors.data(),    sizeof(unsigned char) * data->opaque.colors.size());
            memcpy(mesh.indices,   data->opaque.indices.data(),   sizeof(unsigned short) * data->opaque.indices.size());
            UploadMesh(&mesh, false);
            // После UploadMesh геометрия уже в VRAM (VAO/VBO); CPU-массивы
            // больше не нужны для отрисовки. Освобождаем их, чтобы не
            // дублировать данные в ОЗУ. mesh.indices НЕ обнуляем: raylib
            // использует `indices != NULL` как флаг indexed-отрисовки, а сами
            // индексы берутся из GPU element-buffer, привязанного в VAO.
            MemFree(mesh.vertices);  mesh.vertices  = nullptr;
            MemFree(mesh.normals);   mesh.normals   = nullptr;
            MemFree(mesh.texcoords); mesh.texcoords = nullptr;
            MemFree(mesh.texcoords2); mesh.texcoords2 = nullptr;
            MemFree(mesh.colors);    mesh.colors    = nullptr;
            chunk->opaqueMesh = mesh;
            chunk->hasOpaqueMesh = true;
        }

        if (data && !data->transparent.isEmpty()) {
            Mesh mesh{};
            mesh.vertexCount   = (int)(data->transparent.vertices.size() / 3);
            mesh.triangleCount = (int)(data->transparent.indices.size() / 3);
            mesh.vertices  = (float*)MemAlloc(sizeof(float) * data->transparent.vertices.size());
            mesh.normals   = (float*)MemAlloc(sizeof(float) * data->transparent.normals.size());
            mesh.texcoords = (float*)MemAlloc(sizeof(float) * data->transparent.texcoords.size());
            mesh.texcoords2 = (float*)MemAlloc(sizeof(float) * data->transparent.texcoords2.size());
            mesh.colors    = (unsigned char*)MemAlloc(sizeof(unsigned char) * data->transparent.colors.size());
            mesh.indices   = (unsigned short*)MemAlloc(sizeof(unsigned short) * data->transparent.indices.size());
            memcpy(mesh.vertices,  data->transparent.vertices.data(),  sizeof(float) * data->transparent.vertices.size());
            memcpy(mesh.normals,   data->transparent.normals.data(),   sizeof(float) * data->transparent.normals.size());
            memcpy(mesh.texcoords, data->transparent.texcoords.data(), sizeof(float) * data->transparent.texcoords.size());
            memcpy(mesh.texcoords2, data->transparent.texcoords2.data(), sizeof(float) * data->transparent.texcoords2.size());
            memcpy(mesh.colors,    data->transparent.colors.data(),    sizeof(unsigned char) * data->transparent.colors.size());
            memcpy(mesh.indices,   data->transparent.indices.data(),   sizeof(unsigned short) * data->transparent.indices.size());
            UploadMesh(&mesh, false);
            MemFree(mesh.vertices);  mesh.vertices  = nullptr;
            MemFree(mesh.normals);   mesh.normals   = nullptr;
            MemFree(mesh.texcoords); mesh.texcoords = nullptr;
            MemFree(mesh.texcoords2); mesh.texcoords2 = nullptr;
            MemFree(mesh.colors);    mesh.colors    = nullptr;
            chunk->transparentMesh = mesh;
            chunk->hasTransparentMesh = true;
        }
        chunk->state = ChunkState::Uploaded;
        ++uploads;
    }
}

void World::update(Vector3 playerPos) {
    const ChunkCoord playerChunk = math::worldToChunk(playerPos);
    Vector3 velocityDir{0.0f, 0.0f, 0.0f};
    if (!m_firstUpdate) {
        Vector3 d = Vector3Subtract(playerPos, m_lastPlayerPos);
        const float lenSq = Vector3LengthSqr(d);
        if (lenSq > 0.001f) velocityDir = Vector3Scale(d, 1.0f / sqrtf(lenSq));
    }
    m_lastPlayerPos = playerPos;

    if (m_firstUpdate) {
        m_lastPlayerChunk = playerChunk;
        m_firstUpdate = false;
        requestChunksAround(playerChunk, nullptr);
        unloadDistantChunks(playerChunk);
        return;
    }

    const bool dirty = m_renderDistanceDirty.exchange(false);
    m_updateTimer += GetFrameTime();
    if ((playerChunk != m_lastPlayerChunk) || dirty || (m_updateTimer >= config::UPDATE_INTERVAL_SEC)) {
        m_lastPlayerChunk = playerChunk;
        m_updateTimer = 0.0f;
        const Vector3* vd = (Vector3LengthSqr(velocityDir) > 0.5f) ? &velocityDir : nullptr;
        requestChunksAround(playerChunk, vd);
        unloadDistantChunks(playerChunk);
        requestPendingMeshes();
    }
}

void World::setRenderDistance(int v) {
    v = std::clamp(v, config::RENDER_DISTANCE_MIN, config::RENDER_DISTANCE_MAX);
    m_renderDistance.store(v);
    m_renderDistanceDirty.store(true);
}

void World::unloadDistantChunks(ChunkCoord playerChunk) {
    const int R2 = (m_renderDistance.load() + 2) * (m_renderDistance.load() + 2);
    std::vector<ChunkPtr> removed;
    removed.reserve(64);

    {
        std::unique_lock<std::shared_mutex> lock(m_chunksMutex);
        for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
            const ChunkCoord c = it->second->coord();
            const int dx = c.x - playerChunk.x, dy = c.y - playerChunk.y, dz = c.z - playerChunk.z;
            if (dx * dx + dy * dy + dz * dz > R2) {
                removed.push_back(std::move(it->second));
                it = m_chunks.erase(it);
            } else ++it;
        }
    }

    for (auto& ch : removed) {
        if (ch->hasOpaqueMesh && ch->opaqueMesh.vertexCount > 0) UnloadMesh(ch->opaqueMesh);
        if (ch->hasTransparentMesh && ch->transparentMesh.vertexCount > 0) UnloadMesh(ch->transparentMesh);
    }
}
} // namespace mc