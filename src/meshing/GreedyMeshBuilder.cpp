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
                    bool a_leaf = (a == BlockRegistry::LEAVES), b_leaf = (b == BlockRegistry::LEAVES);
                    bool a_water = (a == BlockRegistry::WATER), b_water = (b == BlockRegistry::WATER);
                    int face_id = 0;
                    if (a != b) {
                        if ((a_leaf && b_leaf) || (a_water && b_water)) {
                            // Грани между листвой/водой не рисуем
                        } else if (a != BLOCK_AIR && !b_opq) {
                            face_id = (int)a * 2 + 1;
                        } else if (b != BLOCK_AIR && !a_opq) {
                            face_id = (int)b * 2 + 0;
                        }
                    }
                    mask[n] = face_id;
                }
            }

            n = 0;
            for (int j = 0; j < sizeV; ++j) {
                for (int i = 0; i < sizeU; ) {
                    x[v] = j;
                    x[u] = i;

                    const int c = mask[n];
                    if (c != 0) {
                        const bool mergeable = (BlockID)(c / 2) != BlockRegistry::WATER;

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

                        BlockID b_id = c / 2;
                        bool posFace = (c % 2) == 1;
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