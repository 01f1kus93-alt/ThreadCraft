#include "meshing/GreedyMeshBuilder.hpp"
#include "world/Chunk.hpp"
#include "world/BlockRegistry.hpp"
#include "render/TextureAtlas.hpp"
#include "core/Config.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace mc {

namespace {
    BlockID getBlock(const Chunk& chunk, const std::shared_ptr<Chunk> neighbors[6], int lx, int ly, int lz) {
        if (lx >= 0 && lx < config::CHUNK_SIZE_X &&
            ly >= 0 && ly < config::CHUNK_SIZE_Y &&
            lz >= 0 && lz < config::CHUNK_SIZE_Z) {
            return chunk.getFast(lx, ly, lz);
        }

        int nx = lx, ny = ly, nz = lz;
        int faceIdx = -1;

        if (lx < 0) { faceIdx = 0; nx += config::CHUNK_SIZE_X; }
        else if (lx >= config::CHUNK_SIZE_X) { faceIdx = 1; nx -= config::CHUNK_SIZE_X; }
        else if (ly < 0) { faceIdx = 2; ny += config::CHUNK_SIZE_Y; }
        else if (ly >= config::CHUNK_SIZE_Y) { faceIdx = 3; ny -= config::CHUNK_SIZE_Y; }
        else if (lz < 0) { faceIdx = 4; nz += config::CHUNK_SIZE_Z; }
        else if (lz >= config::CHUNK_SIZE_Z) { faceIdx = 5; nz -= config::CHUNK_SIZE_Z; }

        if (faceIdx != -1 && nx >= 0 && nx < config::CHUNK_SIZE_X &&
            ny >= 0 && ny < config::CHUNK_SIZE_Y &&
            nz >= 0 && nz < config::CHUNK_SIZE_Z) {
            if (neighbors[faceIdx]) {
                return neighbors[faceIdx]->getFast(nx, ny, nz);
            }
        }

        return BLOCK_AIR;
    }
    constexpr unsigned char FACE_COLORS[6][4] = {
        {190, 190, 190, 255}, // NegX
        {190, 190, 190, 255}, // PosX
        {128, 128, 128, 255}, // NegY (дно)
        {255, 255, 255, 255}, // PosY (верх)
        {160, 160, 160, 255}, // NegZ
        {160, 160, 160, 255}, // PosZ
    };

    bool isLeaf(BlockID id) noexcept {
        return id == BlockRegistry::LEAVES || id == BlockRegistry::BIRCH_LEAVES;
    }

    // face_id занимает 9 бит (block id * 2 + side, до 511). Глубину листвы
    // пакуем в старшие биты, чтобы одинаковые грани с разной глубиной не
    // сливались в один greedy-квад.
    constexpr int FACE_MASK         = 511;  // (1 << 9) - 1
    constexpr int LEAF_DEPTH_SHIFT  = 9;

}

ChunkMeshes GreedyMeshBuilder::build(const Chunk& chunk,
                                     const std::shared_ptr<Chunk> neighbors[6],
                                     const TextureAtlas& atlas) const {
    ChunkMeshes meshes;
    if (!chunk.hasBlocks) return meshes;

    int size[3] = { config::CHUNK_SIZE_X, config::CHUNK_SIZE_Y, config::CHUNK_SIZE_Z };
    if (chunk.maxY + 1 < size[1]) size[1] = chunk.maxY + 1;
    const int worldBase[3] = {
        chunk.coord().x * config::CHUNK_SIZE_X,
        chunk.coord().y * config::CHUNK_SIZE_Y,
        chunk.coord().z * config::CHUNK_SIZE_Z
    };

    // Глубина листвы: 0 = внешняя граница кроны (рядом с воздухом/не-листвой),
    // 1 = следующий слой внутрь и т.д. Нужна шейдеру, чтобы только внешние
    // слои были «дырявыми», а внутренность кроны — непрозрачной.
    const int sy = size[1];
    std::vector<uint8_t> leafDepth((size_t)config::CHUNK_VOLUME, 255);
    auto leafIdx = [](int lx, int ly, int lz) -> int {
        return (ly * config::CHUNK_SIZE_Z + lz) * config::CHUNK_SIZE_X + lx;
    };
    auto localLeafIdx = [&](int lx, int ly, int lz) -> int {
        if (lx < 0 || lx >= config::CHUNK_SIZE_X ||
            ly < 0 || ly >= sy ||
            lz < 0 || lz >= config::CHUNK_SIZE_Z) return -1;
        return leafIdx(lx, ly, lz);
    };

    {
        static const int dirs[6][3] = {
            {-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}
        };
        std::vector<int> queue;
        queue.reserve(4096);

        for (int ly = 0; ly < sy; ++ly) {
            for (int lz = 0; lz < config::CHUNK_SIZE_Z; ++lz) {
                for (int lx = 0; lx < config::CHUNK_SIZE_X; ++lx) {
                    if (!isLeaf(chunk.getFast(lx, ly, lz))) continue;

                    bool boundary = false;
                    for (const auto& dir : dirs) {
                        const BlockID n = getBlock(chunk, neighbors,
                                                   lx + dir[0], ly + dir[1], lz + dir[2]);
                        if (!isLeaf(n)) { boundary = true; break; }
                    }
                    if (boundary) {
                        const int idx = leafIdx(lx, ly, lz);
                        leafDepth[idx] = 0;
                        queue.push_back(idx);
                    }
                }
            }
        }

        size_t head = 0;
        while (head < queue.size()) {
            const int cur = queue[head++];
            const uint8_t d = leafDepth[cur];
            if (d >= 254) continue;

            const int lx = cur % config::CHUNK_SIZE_X;
            const int tmp = cur / config::CHUNK_SIZE_X;
            const int lz = tmp % config::CHUNK_SIZE_Z;
            const int ly = tmp / config::CHUNK_SIZE_Z;

            for (const auto& dir : dirs) {
                const int nx = lx + dir[0];
                const int ny = ly + dir[1];
                const int nz = lz + dir[2];
                if (nx < 0 || nx >= config::CHUNK_SIZE_X ||
                    ny < 0 || ny >= sy ||
                    nz < 0 || nz >= config::CHUNK_SIZE_Z) continue;
                if (!isLeaf(chunk.getFast(nx, ny, nz))) continue;

                const int nidx = leafIdx(nx, ny, nz);
                if (leafDepth[nidx] > d + 1) {
                    leafDepth[nidx] = (uint8_t)(d + 1);
                    queue.push_back(nidx);
                }
            }
        }
    }

    std::vector<int> mask;
    const int MAX_MERGE = std::min(config::MAX_MERGE, 255); 

    for (int d = 0; d < 3; ++d) {
        const int u = (d + 1) % 3;
        const int v = (d + 2) % 3;
        const int sizeU = size[u];
        const int sizeV = size[v];
        mask.resize(sizeU * sizeV);

        int x[3] = {0, 0, 0};
        int q[3] = {0, 0, 0};
        q[d] = 1;

        for (x[d] = -1; x[d] < size[d]; ++x[d]) {
            int n = 0;
            for (x[v] = 0; x[v] < sizeV; ++x[v]) {
                for (x[u] = 0; x[u] < sizeU; ++x[u], ++n) {
                    const BlockID a = getBlock(chunk, neighbors, x[0], x[1], x[2]);
                    const BlockID b = getBlock(chunk, neighbors, x[0] + q[0], x[1] + q[1], x[2] + q[2]);

                    bool a_opq = (a != BLOCK_AIR) && BlockRegistry::get(a).isOpaque;
                    bool b_opq = (b != BLOCK_AIR) && BlockRegistry::get(b).isOpaque;
                    bool a_leaf = isLeaf(a);
                    bool b_leaf = isLeaf(b);
                    bool a_water = (a == BlockRegistry::WATER), b_water = (b == BlockRegistry::WATER);
                    int face_id = 0;
                    int leafDepthValue = 0;
                    if (a != b) {
                        if (a_leaf && b_leaf) {
                            // Между листвой рисуем только одну грань (ту, что
                            // глубже внутрь кроны). Это даёт непрозрачный фон
                            // за внешним «дырявым» слоем листвы.
                            const int ai = localLeafIdx(x[0], x[1], x[2]);
                            const int bi = localLeafIdx(x[0] + q[0], x[1] + q[1], x[2] + q[2]);
                            const int da = (ai >= 0) ? leafDepth[ai] : 255;
                            const int db = (bi >= 0) ? leafDepth[bi] : 255;

                            if (ai >= 0 && bi >= 0) {
                                if (db > da) { face_id = (int)b * 2 + 0; leafDepthValue = db; }
                                else         { face_id = (int)a * 2 + 1; leafDepthValue = da; }
                            } else if (ai < 0 && bi >= 0) {
                                face_id = (int)b * 2 + 0; leafDepthValue = db;
                            } else if (ai >= 0 && bi < 0) {
                                face_id = (int)a * 2 + 1; leafDepthValue = da;
                            }
                        } else if (a_water && b_water) {
                            // Грани между водой не рисуем
                        } else if (a != BLOCK_AIR && !b_opq) {
                            face_id = (int)a * 2 + 1;
                        } else if (b != BLOCK_AIR && !a_opq) {
                            face_id = (int)b * 2 + 0;
                        }
                    }
                    mask[n] = (face_id != 0) ? (face_id | (leafDepthValue << LEAF_DEPTH_SHIFT)) : 0;
                }
            }

            n = 0;
            for (int j = 0; j < sizeV; ++j) {
                for (int i = 0; i < sizeU; ) {
                    x[v] = j;
                    x[u] = i;

                    const int c = mask[n];
                    if (c != 0) {
                        const int face_id = c & FACE_MASK;
                        const int leafDepthValue = c >> LEAF_DEPTH_SHIFT;
                        const bool mergeable = (BlockID)(face_id / 2) != BlockRegistry::WATER;

                        int w = 1;
                        while (mergeable && i + w < sizeU && mask[n + w] == c && w < MAX_MERGE) ++w;

                        int h = 1;
                        bool done = false;
                        while (mergeable && j + h < sizeV && h < MAX_MERGE) {
                            for (int k = 0; k < w; ++k) {
                                if (mask[n + k + h * sizeU] != c) { done = true; break; }
                            }
                            if (done) break;
                            ++h;
                        }

                        BlockID b_id = face_id / 2;
                        bool posFace = (face_id % 2) == 1;
                        bool isTrans = BlockRegistry::get(b_id).isTransparent;
                        ChunkMeshData& target = isTrans ? meshes.transparent : meshes.opaque;

                        int face_dir = d * 2 + (posFace ? 1 : 0);
                        int tile = BlockRegistry::get(b_id).faceTiles[face_dir];
                        const TileUV uv = atlas.tileUV(tile);

                        float du[3] = {0, 0, 0}, dv[3] = {0, 0, 0};
                        du[u] = (float)w; dv[v] = (float)h;

                        float p[3];
                        p[0] = (float)x[0] + worldBase[0];
                        p[1] = (float)x[1] + worldBase[1];
                        p[2] = (float)x[2] + worldBase[2];
                        p[d] += 1.0f;

                        float vx[4][3];
                        if (posFace) {
                            if (d == 0) {
                                vx[0][0] = p[0]+dv[0]; vx[0][1] = p[1]+dv[1]; vx[0][2] = p[2]+dv[2];
                                vx[1][0] = p[0];       vx[1][1] = p[1];       vx[1][2] = p[2];
                                vx[2][0] = p[0]+du[0]; vx[2][1] = p[1]+du[1]; vx[2][2] = p[2]+du[2];
                                vx[3][0] = p[0]+du[0]+dv[0]; vx[3][1] = p[1]+du[1]+dv[1]; vx[3][2] = p[2]+du[2]+dv[2];
                            } else if (d == 1) {
                                vx[0][0] = p[0];             vx[0][1] = p[1];             vx[0][2] = p[2];
                                vx[1][0] = p[0]+du[0];       vx[1][1] = p[1]+du[1];       vx[1][2] = p[2]+du[2];
                                vx[2][0] = p[0]+du[0]+dv[0]; vx[2][1] = p[1]+du[1]+dv[1]; vx[2][2] = p[2]+du[2]+dv[2];
                                vx[3][0] = p[0]+dv[0];       vx[3][1] = p[1]+dv[1];       vx[3][2] = p[2]+dv[2];
                            } else {
                                vx[0][0] = p[0];       vx[0][1] = p[1];       vx[0][2] = p[2];
                                vx[1][0] = p[0]+du[0]; vx[1][1] = p[1]+du[1]; vx[1][2] = p[2]+du[2];
                                vx[2][0] = p[0]+du[0]+dv[0]; vx[2][1] = p[1]+du[1]+dv[1]; vx[2][2] = p[2]+du[2]+dv[2];
                                vx[3][0] = p[0]+dv[0]; vx[3][1] = p[1]+dv[1]; vx[3][2] = p[2]+dv[2];
                            }
                        } else {
                            if (d == 0) {
                                vx[0][0] = p[0];       vx[0][1] = p[1];       vx[0][2] = p[2];
                                vx[1][0] = p[0]+dv[0]; vx[1][1] = p[1]+dv[1]; vx[1][2] = p[2]+dv[2];
                                vx[2][0] = p[0]+du[0]+dv[0]; vx[2][1] = p[1]+du[1]+dv[1]; vx[2][2] = p[2]+du[2]+dv[2];
                                vx[3][0] = p[0]+du[0]; vx[3][1] = p[1]+du[1]; vx[3][2] = p[2]+du[2];
                            } else if (d == 1) {
                                vx[0][0] = p[0];       vx[0][1] = p[1];       vx[0][2] = p[2];
                                vx[1][0] = p[0]+dv[0]; vx[1][1] = p[1]+dv[1]; vx[1][2] = p[2]+dv[2];
                                vx[2][0] = p[0]+du[0]+dv[0]; vx[2][1] = p[1]+du[1]+dv[1]; vx[2][2] = p[2]+du[2]+dv[2];
                                vx[3][0] = p[0]+du[0]; vx[3][1] = p[1]+du[1]; vx[3][2] = p[2]+du[2];
                            } else {
                                vx[0][0] = p[0]+du[0]; vx[0][1] = p[1]+du[1]; vx[0][2] = p[2]+du[2];
                                vx[1][0] = p[0];       vx[1][1] = p[1];       vx[1][2] = p[2];
                                vx[2][0] = p[0]+dv[0]; vx[2][1] = p[1]+dv[1]; vx[2][2] = p[2]+dv[2];
                                vx[3][0] = p[0]+du[0]+dv[0]; vx[3][1] = p[1]+du[1]+dv[1]; vx[3][2] = p[2]+du[2]+dv[2];
                            }
                        }

                        float nx[3] = {0, 0, 0};
                        nx[d] = posFace ? 1.0f : -1.0f;
                        const unsigned short base = (unsigned short)(target.vertices.size() / 3);
                        float uvs[4][2] = { {uv.u0, uv.v1}, {uv.u1, uv.v1}, {uv.u1, uv.v0}, {uv.u0, uv.v0} };

                        const unsigned char brightness = FACE_COLORS[face_dir][0];

                        // ОРИГИНАЛЬНЫЕ И ПРАВИЛЬНЫЕ repeat_u / repeat_v
                        float repeat_u = 1.0f;
                        float repeat_v = 1.0f;

                        if (d == 0) {
                            repeat_u = (float)h;
                            repeat_v = (float)w;
                        } else if (d == 1) {
                            if (posFace) {
                                repeat_u = (float)w;
                                repeat_v = (float)h;
                            } else {
                                repeat_u = (float)h;
                                repeat_v = (float)w;
                            }
                        } else {
                            repeat_u = (float)w;
                            repeat_v = (float)h;
                        }

                        for (int i_v = 0; i_v < 4; ++i_v) {
                            target.vertices.push_back(vx[i_v][0]);
                            target.vertices.push_back(vx[i_v][1]);
                            target.vertices.push_back(vx[i_v][2]);
                            target.normals.push_back(nx[0]);
                            target.normals.push_back(nx[1]);
                            target.normals.push_back(nx[2]);
                            target.texcoords.push_back(uvs[i_v][0]);
                            target.texcoords.push_back(uvs[i_v][1]);

                            target.colors.push_back(static_cast<unsigned char>(tile));
                            target.colors.push_back(static_cast<unsigned char>(repeat_u));
                            target.colors.push_back(static_cast<unsigned char>(repeat_v));
                            target.colors.push_back(brightness);

                            target.texcoords2.push_back((float)leafDepthValue);
                            target.texcoords2.push_back(0.0f);
                        }

                        target.indices.push_back(base + 0); target.indices.push_back(base + 1); target.indices.push_back(base + 2);
                        target.indices.push_back(base + 0); target.indices.push_back(base + 2); target.indices.push_back(base + 3);

                        for (int l = 0; l < h; ++l)
                            for (int k = 0; k < w; ++k)
                                mask[n + k + l * sizeU] = 0;

                        i += w; n += w;
                    } else {
                        ++i; ++n;
                    }
                }
            }
        }
    }
    return meshes;
}
} // namespace mc