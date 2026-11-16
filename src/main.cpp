#include "core/Config.hpp"
#include "world/BlockRegistry.hpp"
#include "world/World.hpp"
#include "world/ParticleSystem.hpp"
#include "world/ItemEntityManager.hpp"
#include "render/TextureAtlas.hpp"
#include "render/Renderer.hpp"
#include "player/Player.hpp"
#include "ui/PauseMenu.hpp"
#include "ui/HotbarRenderer.hpp"
#include "ui/InventoryScreen.hpp"
#include "ui/Inventory.hpp"

#include <raylib.h>
#include <algorithm>
#include <thread>
#include <cstdio>
#include <cstdlib>

using namespace mc;

TextureAtlas* mc::g_textureAtlas = nullptr;

int main() {
    BlockRegistry::init();

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "ThreadCraft");
    SetTargetFPS(3000);
    SetExitKey(KEY_NULL);

    TextureAtlas atlas;
    if (!atlas.load("assets/minecraft/textures/block")) {
        CloseWindow();
        return 1;
    }
    mc::g_textureAtlas = &atlas;

    Renderer renderer;
    if (!renderer.load(atlas)) {
        CloseWindow();
        return 1;
    }

    PauseMenu menu;
    menu.load();

    HotbarRenderer hotbar;
    hotbar.load();

    InventoryScreen invScreen;
    invScreen.load();

    Player player;
    player.load();
    player.inventory().add(BlockRegistry::STONE, 64);
    player.inventory().add(BlockRegistry::DIRT, 64);

    const int hw = (int)std::thread::hardware_concurrency();
    const int numThreads = std::max(4, hw > 1 ? hw - 1 : 4);
    World world(config::SEED, numThreads);

    ParticleSystem particles;
    ItemEntityManager items;

    player.camera().get().position = { 16.0f, 60.0f, 16.0f };
    player.camera().setLookAngles(0.0f, -0.5f);

    bool quitRequested = false;
    float gamemodeToastTimer = 0.0f;

    while (!WindowShouldClose() && !quitRequested) {
        const float dt = std::min(GetFrameTime(), 0.05f);
        const bool gameplay = !menu.isOpen() && !invScreen.isOpen();

        // ── Input ──
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (invScreen.isOpen()) invScreen.close(player.inventory());
            else                     menu.handleEscape();
        }
        if (IsKeyPressed(KEY_E) && !menu.isOpen()) {
            invScreen.toggle(player.inventory());
        }

        const bool menuSkip = menu.consumeMouseSkip();
        const bool invSkip  = invScreen.consumeMouseSkip();
        if (menuSkip || invSkip) {
            (void)GetMouseDelta();
        }

        // ── World ──
        world.update(player.camera().get().position);
        world.processUploads(atlas);

        // ── Gamemode toggle (F3 + F4) ──
        static bool f3Held = false, f4Held = false, comboDone = false;
        if (IsKeyPressed(KEY_F3))  f3Held = true;
        if (IsKeyReleased(KEY_F3)) f3Held = false;
        if (IsKeyPressed(KEY_F4))  f4Held = true;
        if (IsKeyReleased(KEY_F4)) f4Held = false;
        if (f3Held && f4Held && !comboDone) {
            comboDone = true;
            player.setGameMode(player.gameMode() == GameMode::Creative
                                   ? GameMode::Survival : GameMode::Creative);
            gamemodeToastTimer = 2.0f;
        }
        if (!f3Held || !f4Held) comboDone = false;

        // ── Player ──
        player.update(dt, world, gameplay);
        player.handleInteraction(dt, world, particles, items, gameplay);

        // ── Simulation ──
        items.update(dt, player.camera().get().position, world, player.inventory());
        particles.update(dt);

        // ── 3D ──
        renderer.beginFrame(player.camera().get());

        BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(player.camera().get());

            renderer.drawWorld(world);
            items.draw3D(player.camera().get(), atlas);
            particles.draw3D(player.camera().get());
            player.breaker().drawCrack();
            renderer.drawSelection(player.lastHit());

        EndMode3D();

        // ── First-person arm (2D sprite, behind hotbar/HUD) ──
        if (gameplay) {
            renderer.drawPlayerArm(player.skinTexture(), player.swingAngle());
        }

        // ── 2D UI ──
        if (!invScreen.isOpen()) {
            hotbar.draw(player.inventory(), atlas);
        }
        invScreen.update(player.inventory(), items, player.camera().get().position);
        invScreen.draw(player.inventory(), atlas);

        if (menu.isOpen()) {
            menu.draw();
            const MenuAction action = menu.update();
            int rd = 0;
            if (menu.takeRenderDistanceChange(rd)) {
                world.setRenderDistance(rd);
            }
            if (action == MenuAction::Resume) menu.close();
            else if (action == MenuAction::Quit) quitRequested = true;
        }

        if (gameplay) {
            renderer.drawCrosshair();
            renderer.drawDebugInfo(world, player.camera().get(),
                                   player.inventory().hotbar(player.inventory().selected()));
            renderer.drawFrameGraph();

            if (gamemodeToastTimer > 0.0f) {
                gamemodeToastTimer -= dt;
                const char* msg = (player.gameMode() == GameMode::Creative)
                    ? "Gamemode: Creative" : "Gamemode: Survival";
                const int w = MeasureText(msg, 30);
                DrawText(msg, (GetScreenWidth() - w) / 2, 80, 30, YELLOW);
            }
            const char* modeMsg = (player.gameMode() == GameMode::Creative)
                ? "Mode: Creative" : "Mode: Survival";
            DrawText(modeMsg, 10, GetScreenHeight() - 30, 20, YELLOW);
        }

        EndDrawing();
    }

    EnableCursor();
    fflush(stdout);
    std::_Exit(0);
}
