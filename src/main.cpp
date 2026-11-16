#include "core/Config.hpp"
#include "world/BlockRegistry.hpp"
#include "world/World.hpp"
#include "render/TextureAtlas.hpp"
#include "render/Camera.hpp"
#include "utils/Frustum.hpp"
#include "ui/PauseMenu.hpp"

#include <raylib.h>
#include <raymath.h>
#include <thread>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>

using namespace mc;

TextureAtlas* mc::g_textureAtlas = nullptr;

int main() {
    BlockRegistry::init();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "ThreadCraft — Stage 1: Render Optimization");
    SetTargetFPS(300);
    SetExitKey(KEY_NULL);           

    TextureAtlas atlas;
    if (!atlas.load("assets/minecraft/textures/block")) {
        CloseWindow();
        return 1;
    }
    mc::g_textureAtlas = &atlas;

    PauseMenu menu;
    menu.load();

    // ─── Кастомный шейдер ───
    Shader greedyShader = LoadShader("resources/greedy.vs", "resources/greedy.fs");
    
    // Безопасный поиск локации для массива (некоторые драйверы требуют [0])
    int tileUVLoc = GetShaderLocation(greedyShader, "tileUVs[0]");
    if (tileUVLoc == -1) tileUVLoc = GetShaderLocation(greedyShader, "tileUVs");
    
    if (tileUVLoc != -1) {
        float uvs[256 * 4] = {0};
        for (int i = 0; i < 256; i++) {
            TileUV uv = atlas.tileUV(i);
            uvs[i*4+0] = uv.u0;
            uvs[i*4+1] = uv.v0;
            uvs[i*4+2] = uv.u1;
            uvs[i*4+3] = uv.v1;
        }
        SetShaderValueV(greedyShader, tileUVLoc, uvs, SHADER_UNIFORM_VEC4, 256);
    } else {
        TraceLog(LOG_ERROR, "Uniform 'tileUVs' not found in shader!");
    }

    int mvpLoc = GetShaderLocation(greedyShader, "mvp");

    Material mat = LoadMaterialDefault();
    mat.maps[MATERIAL_MAP_DIFFUSE].texture = atlas.texture();
    mat.shader = greedyShader;

    const int hw = (int)std::thread::hardware_concurrency();
    const int numThreads = std::max(4, hw > 1 ? hw - 1 : 4);
    World world(config::SEED, numThreads);

    FlyCamera cam;
    cam.get().position = { 16.0f, 60.0f, 16.0f };
    cam.setLookAngles(0.0f, -0.5f);

    Frustum frustum;

    BlockID selectedBlock = BlockRegistry::STONE;

    constexpr int GRAPH_SIZE = 120;
    float frameTimes[GRAPH_SIZE] = {0};
    int frameIndex = 0;
    bool quitRequested = false;

    while (!WindowShouldClose() && !quitRequested) {
        const float dt = GetFrameTime();

        if (IsKeyPressed(KEY_ESCAPE)) {
            menu.handleEscape();
        }

        if (menu.consumeMouseSkip()) {
            (void)GetMouseDelta();
        }

        world.update(cam.get().position);
        world.processUploads(atlas);

        if (!menu.isOpen()) {
            frameTimes[frameIndex] = dt;
            frameIndex = (frameIndex + 1) % GRAPH_SIZE;
            cam.update(dt);
        }

        const int centerX = GetScreenWidth() / 2;
        const int centerY = GetScreenHeight() / 2;
        const Vector2 mousePos = { (float)centerX, (float)centerY };
        const Ray ray = GetScreenToWorldRay(mousePos, cam.get());
        const RaycastResult hit = world.raycast(ray.position, ray.direction, 8.0f);

        if (!menu.isOpen() && hit.hit) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                world.setBlock(hit.wx, hit.wy, hit.wz, BLOCK_AIR);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                world.setBlock(hit.wx + hit.nx, hit.wy + hit.ny, hit.wz + hit.nz, selectedBlock);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
                selectedBlock = world.getBlock(hit.wx, hit.wy, hit.wz);
            }
        }

        const Camera3D& camera = cam.get();
        const float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();

        // Принудительно обновляем матрицу MVP в шейдере каждый кадр
        if (mvpLoc != -1) {
            Matrix matView = GetCameraMatrix(camera);
            Matrix matProj = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.1f, 1000.0f);
            Matrix matMVP = MatrixMultiply(matView, matProj);
            SetShaderValueMatrix(greedyShader, mvpLoc, matMVP);
        }

        frustum.update(camera, aspect);

        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(camera);

            // 1. Отрисовка непрозрачных блоков
            for (auto& [key, chunkPtr] : world.chunks()) {
                if (!chunkPtr->hasOpaqueMesh || chunkPtr->opaqueMesh.vertexCount == 0) continue;
                if (!frustum.isBoxVisible(chunkPtr->boxMin, chunkPtr->boxMax)) continue;
                DrawMesh(chunkPtr->opaqueMesh, mat, MatrixIdentity());
            }

            // 2. Отрисовка прозрачных блоков (листва/вода).
            for (auto& [key, chunkPtr] : world.chunks()) {
                if (!chunkPtr->hasTransparentMesh || chunkPtr->transparentMesh.vertexCount == 0) continue;
                if (!frustum.isBoxVisible(chunkPtr->boxMin, chunkPtr->boxMax)) continue;
                DrawMesh(chunkPtr->transparentMesh, mat, MatrixIdentity());
            }

            if (hit.hit) {
                const Vector3 cubeCenter = {
                    (float)hit.wx + 0.5f, (float)hit.wy + 0.5f, (float)hit.wz + 0.5f
                };
                DrawCubeWires(cubeCenter, 1.002f, 1.002f, 1.002f, Color{ 0, 0, 0, 128 });
            }

        EndMode3D();

        if (menu.isOpen()) {
            menu.draw();
            const MenuAction action = menu.update();
            int rd = 0;
            if (menu.takeRenderDistanceChange(rd)) {
                world.setRenderDistance(rd);
            }
            if (action == MenuAction::Resume) {
                menu.close();
            } else if (action == MenuAction::Quit) {
                quitRequested = true;
            }
        } else {
            DrawLine(centerX - 12, centerY, centerX - 2, centerY, WHITE);
            DrawLine(centerX + 2,  centerY, centerX + 12, centerY, WHITE);
            DrawLine(centerX, centerY - 12, centerX, centerY - 2, WHITE);
            DrawLine(centerX, centerY + 2,  centerX, centerY + 12, WHITE);

            DrawFPS(10, 10);
            DrawText(TextFormat("Chunks in memory: %zu", world.chunkCount()), 10, 40, 20, DARKGRAY);
            DrawText(TextFormat("Camera: %.1f, %.1f, %.1f", camera.position.x, camera.position.y, camera.position.z), 10, 65, 20, DARKGRAY);
            DrawText(TextFormat("Selected: %d", selectedBlock), 10, 115, 20, DARKGRAY);

            {
                const int graphX = GetScreenWidth() - 220;
                const int graphY = 10;
                const int graphWidth = 200;
                const int graphHeight = 60;

                DrawRectangle(graphX, graphY, graphWidth, graphHeight, Color{0, 0, 0, 150});

                float maxTime = 0.0f;
                for (int i = 0; i < GRAPH_SIZE; ++i) {
                    maxTime = std::max(maxTime, frameTimes[i]);
                }
                if (maxTime <= 0.0f) maxTime = 1.0f / 60.0f;

                const float stepX = (float)graphWidth / GRAPH_SIZE;
                for (int i = 0; i < GRAPH_SIZE - 1; ++i) {
                    const float x1 = graphX + i * stepX;
                    const float y1 = graphY + graphHeight - (frameTimes[i] / maxTime) * graphHeight;
                    const float x2 = graphX + (i + 1) * stepX;
                    const float y2 = graphY + graphHeight - (frameTimes[i + 1] / maxTime) * graphHeight;
                    DrawLine((int)x1, (int)y1, (int)x2, (int)y2, LIME);
                }

                const float limit60 = 0.0166f;
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
        }

        EndDrawing();
    }

    EnableCursor();
    fflush(stdout);
    std::_Exit(0);
}