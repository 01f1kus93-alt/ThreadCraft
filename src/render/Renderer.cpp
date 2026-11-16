#include "render/Renderer.hpp"
#include <algorithm>
#include <raymath.h>
#include <rlgl.h>
#include <cmath>

namespace mc {

bool Renderer::load(const TextureAtlas& atlas) {
    m_shader = LoadShader("resources/greedy.vs", "resources/greedy.fs");
    if (m_shader.id == 0) {
        TraceLog(LOG_ERROR, "Renderer: failed to load greedy shader");
        return false;
    }

    int tileUVLoc = GetShaderLocation(m_shader, "tileUVs[0]");
    if (tileUVLoc == -1) tileUVLoc = GetShaderLocation(m_shader, "tileUVs");
    if (tileUVLoc != -1) {
        float uvs[256 * 4] = {0};
        for (int i = 0; i < 256; i++) {
            const TileUV uv = atlas.tileUV(i);
            uvs[i*4+0] = uv.u0;
            uvs[i*4+1] = uv.v0;
            uvs[i*4+2] = uv.u1;
            uvs[i*4+3] = uv.v1;
        }
        SetShaderValueV(m_shader, tileUVLoc, uvs, SHADER_UNIFORM_VEC4, 256);
    } else {
        TraceLog(LOG_ERROR, "Uniform 'tileUVs' not found in shader!");
    }

    m_mvpLoc = GetShaderLocation(m_shader, "mvp");

    m_chunkMat = LoadMaterialDefault();
    m_chunkMat.maps[MATERIAL_MAP_DIFFUSE].texture = atlas.texture();
    m_chunkMat.shader = m_shader;

    return true;
}

void Renderer::unload() {
    // Process exits via std::_Exit(0); the diffuse texture is the atlas
    // (owned by TextureAtlas) and is intentionally not unloaded here.
    if (m_shader.id != 0) UnloadShader(m_shader);
    m_shader = Shader{};
    m_chunkMat = Material{};
}

void Renderer::beginFrame(const Camera3D& cam) {
    const float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    const Matrix matView = GetCameraMatrix(cam);
    const Matrix matProj = MatrixPerspective(cam.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
    const Matrix matMVP  = MatrixMultiply(matView, matProj);
    if (m_mvpLoc != -1) SetShaderValueMatrix(m_shader, m_mvpLoc, matMVP);
    m_frustum.update(cam, aspect);
    m_camPos = cam.position;

    m_frameTimes[m_frameIndex] = GetFrameTime();
    m_frameIndex = (m_frameIndex + 1) % GRAPH_SIZE;
}

bool Renderer::isChunkVisible(Vector3 boxMin, Vector3 boxMax) const {
    const bool camInside =
        m_camPos.x >= boxMin.x && m_camPos.x <= boxMax.x &&
        m_camPos.y >= boxMin.y && m_camPos.y <= boxMax.y &&
        m_camPos.z >= boxMin.z && m_camPos.z <= boxMax.z;
    if (camInside) return true;

    // Inflate the AABB slightly so chunks touching the frustum edges (or the
    // camera sitting just outside) are not wrongly culled.
    const Vector3 bmin = { boxMin.x - 1.5f, boxMin.y - 1.5f, boxMin.z - 1.5f };
    const Vector3 bmax = { boxMax.x + 1.5f, boxMax.y + 1.5f, boxMax.z + 1.5f };
    return m_frustum.isBoxVisible(bmin, bmax);
}

void Renderer::drawWorld(const World& world) const {
    const bool debugFrustum = IsKeyDown(KEY_F9);

    // Temporary frustum-culling debug overlay (F9): green = drawn, red = culled.
    if (debugFrustum) {
        rlDisableShader();  // default shader before immediate-mode wire cubes
        for (const auto& [key, chunkPtr] : world.chunks()) {
            const bool hasMesh =
                (chunkPtr->hasOpaqueMesh && chunkPtr->opaqueMesh.vertexCount > 0) ||
                (chunkPtr->hasTransparentMesh && chunkPtr->transparentMesh.vertexCount > 0);
            if (!hasMesh) continue;
            const Vector3 size = {
                chunkPtr->boxMax.x - chunkPtr->boxMin.x,
                chunkPtr->boxMax.y - chunkPtr->boxMin.y,
                chunkPtr->boxMax.z - chunkPtr->boxMin.z
            };
            const Vector3 center = {
                (chunkPtr->boxMin.x + chunkPtr->boxMax.x) * 0.5f,
                (chunkPtr->boxMin.y + chunkPtr->boxMax.y) * 0.5f,
                (chunkPtr->boxMin.z + chunkPtr->boxMax.z) * 0.5f
            };
            DrawCubeWires(center, size.x, size.y, size.z,
                          isChunkVisible(chunkPtr->boxMin, chunkPtr->boxMax) ? GREEN : RED);
        }
    }

    for (const auto& [key, chunkPtr] : world.chunks()) {
        if (!chunkPtr->hasOpaqueMesh || chunkPtr->opaqueMesh.vertexCount == 0) continue;
        if (!isChunkVisible(chunkPtr->boxMin, chunkPtr->boxMax)) continue;
        DrawMesh(chunkPtr->opaqueMesh, m_chunkMat, MatrixIdentity());
    }
    for (const auto& [key, chunkPtr] : world.chunks()) {
        if (!chunkPtr->hasTransparentMesh || chunkPtr->transparentMesh.vertexCount == 0) continue;
        if (!isChunkVisible(chunkPtr->boxMin, chunkPtr->boxMax)) continue;
        DrawMesh(chunkPtr->transparentMesh, m_chunkMat, MatrixIdentity());
    }
}

void Renderer::drawSelection(const RaycastResult& hit) const {
    if (!hit.hit) return;
    const Vector3 center = { (float)hit.wx + 0.5f, (float)hit.wy + 0.5f, (float)hit.wz + 0.5f };
    DrawCubeWires(center, 1.002f, 1.002f, 1.002f, Color{ 0, 0, 0, 128 });
}

void Renderer::drawCrosshair() const {
    const int cx = GetScreenWidth() / 2;
    const int cy = GetScreenHeight() / 2;
    DrawLine(cx - 12, cy, cx - 2, cy, WHITE);
    DrawLine(cx + 2,  cy, cx + 12, cy, WHITE);
    DrawLine(cx, cy - 12, cx, cy - 2, WHITE);
    DrawLine(cx, cy + 2,  cx, cy + 12, WHITE);
}

void Renderer::drawDebugInfo(const World& world, const Camera3D& cam, const ItemStack& selected) const {
    DrawFPS(10, 10);
    DrawText(TextFormat("Chunks in memory: %zu", world.chunkCount()), 10, 40, 20, DARKGRAY);
    DrawText(TextFormat("Camera: %.1f, %.1f, %.1f",
                        cam.position.x, cam.position.y, cam.position.z), 10, 65, 20, DARKGRAY);
    if (!selected.isEmpty()) {
        DrawText(TextFormat("%d x%d", selected.id, selected.count), 10, 115, 20, DARKGRAY);
    }
}

void Renderer::drawFrameGraph() const {
    const int graphX = GetScreenWidth() - 220;
    const int graphY = 10;
    const int graphWidth = 200;
    const int graphHeight = 60;

    DrawRectangle(graphX, graphY, graphWidth, graphHeight, Color{0, 0, 0, 150});

    float maxTime = 0.0f;
    for (int i = 0; i < GRAPH_SIZE; ++i) {
        maxTime = std::max(maxTime, m_frameTimes[i]);
    }
    if (maxTime <= 0.0f) maxTime = 1.0f / 60.0f;

    const float stepX = (float)graphWidth / GRAPH_SIZE;
    for (int i = 0; i < GRAPH_SIZE - 1; ++i) {
        const float x1 = graphX + i * stepX;
        const float y1 = graphY + graphHeight - (m_frameTimes[i] / maxTime) * graphHeight;
        const float x2 = graphX + (i + 1) * stepX;
        const float y2 = graphY + graphHeight - (m_frameTimes[i + 1] / maxTime) * graphHeight;
        DrawLine((int)x1, (int)y1, (int)x2, (int)y2, LIME);
    }

    const float limit60  = 0.0166f;
    const float limit120 = 0.0083f;
    if (maxTime >= limit60) {
        const int y = (int)(graphY + graphHeight - (limit60 / maxTime) * graphHeight);
        DrawLine(graphX, y, graphX + graphWidth, y, RED);
    }
    if (maxTime >= limit120) {
        const int y = (int)(graphY + graphHeight - (limit120 / maxTime) * graphHeight);
        DrawLine(graphX, y, graphX + graphWidth, y, RED);
    }

    DrawText(TextFormat("%.2f ms", GetFrameTime() * 1000.0f), graphX, graphY + graphHeight + 5, 20, LIME);
}

} // namespace mc
