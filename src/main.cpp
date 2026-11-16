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
#include "ui/GamemodeMenu.hpp"
#include "ui/HealthBar.hpp"

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

    GamemodeMenu gamemodeMenu;
    gamemodeMenu.load();

    HealthBar healthbar;
    healthbar.load();

    Player player;
    player.load();
    player.inventory().add(BlockRegistry::STONE, 64);
    player.inventory().add(BlockRegistry::DIRT, 64);

    const int hw = (int)std::thread::hardware_concurrency();
    const int numThreads = std::max(4, hw > 1 ? hw - 1 : 4);
    World world(config::SEED, numThreads);

    ParticleSystem particles;
    ItemEntityManager items;

    player.camera().get().position = { 16.0f, 120.0f, 16.0f };
    player.camera().setLookAngles(0.0f, -0.5f);

    bool quitRequested = false;

    while (!WindowShouldClose() && !quitRequested) {
        const float dt = std::min(GetFrameTime(), 0.05f);

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

        // ── Gamemode switcher (must update BEFORE gameplay computation) ──
        gamemodeMenu.update(dt, player.gameMode());
        {
            GameMode newGm;
            if (gamemodeMenu.takeModeChange(newGm))
                player.setGameMode(newGm);
        }

        // ── Compute gameplay AFTER input (prevents camera spike on UI open) ──
        const bool gameplay = !menu.isOpen() && !invScreen.isOpen() && !gamemodeMenu.isOpen();

        // ── Track focus transitions; skip one mouse frame on any open/close ──
        static bool prevGameplay = true;
        if (gameplay != prevGameplay) {
            player.camera().skipNextMouseFrame();
            prevGameplay = gameplay;
        }

        // ── World ──
        world.update(player.camera().get().position);
        world.processUploads(atlas);

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
            healthbar.draw(player.health());
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

        // ── Gamemode switcher overlay (draws even when !gameplay) ──
        gamemodeMenu.draw(atlas);

        if (gameplay) {
            renderer.drawCrosshair();
            renderer.drawDebugInfo(world, player.camera().get(),
                                   player.inventory().hotbar(player.inventory().selected()));
            renderer.drawFrameGraph();
        }

        EndDrawing();
    }

    EnableCursor();
    fflush(stdout);
    std::_Exit(0);
}
