cmake_minimum_required(VERSION 3.20)

project(minecraft_clone
    VERSION 0.1.0
    DESCRIPTION "Multithreaded Minecraft-like voxel engine"
    LANGUAGES CXX
)

# ──────────────────────────────────────────────────────────────
# Стандарт C++
# ──────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ──────────────────────────────────────────────────────────────
# Опции
# ──────────────────────────────────────────────────────────────
option(ENABLE_LTO "Enable Link-Time Optimization in Release" OFF)

# ──────────────────────────────────────────────────────────────
# Флаги компилятора
# ──────────────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -O3 -march=native
        -Wno-unused-parameter
    )
endif()

if(ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
    if(ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    else()
        message(WARNING "LTO not supported: ${ipo_msg}")
    endif()
endif()

# ──────────────────────────────────────────────────────────────
# Threads
# ──────────────────────────────────────────────────────────────
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# ──────────────────────────────────────────────────────────────
# Raylib — системная установка
# ──────────────────────────────────────────────────────────────
# Вариант 1: CMake-конфиг (ставится через make install / vcpkg / apt)
find_package(raylib 4.5 QUIET)

if(NOT raylib_FOUND)
    # Вариант 2: pkg-config (часто единственное, что есть после make install)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)
    message(STATUS "raylib: using pkg-config fallback -> ${RAYLIB_LINK_LIBRARIES}")
    set(RAYLIB_TARGET PkgConfig::RAYLIB)
else()
    message(STATUS "raylib: found via CMake package (${raylib_VERSION})")
    set(RAYLIB_TARGET raylib)
endif()

# ──────────────────────────────────────────────────────────────
# Исходники
# ──────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp

    src/ui/PauseMenu.cpp
    src/ui/Inventory.cpp
    src/ui/HotbarRenderer.cpp

    src/core/ThreadPool.cpp
    src/core/TaskQueue.cpp

    src/world/Block.cpp
    src/world/BlockRegistry.cpp
    src/world/Chunk.cpp
    src/world/World.cpp
    src/world/WorldGenerator.cpp
    src/world/ParticleSystem.cpp

    src/meshing/GreedyMeshBuilder.cpp

    src/render/Renderer.cpp
    src/render/TextureAtlas.cpp
    src/render/Camera.cpp

    src/player/Player.cpp
    src/player/BlockBreaker.cpp
)

set(HEADERS
    src/ui/McFont.hpp
    src/ui/PauseMenu.hpp
    src/ui/GuiScale.hpp
    src/ui/ItemStack.hpp
    src/ui/Inventory.hpp
    src/ui/HotbarRenderer.hpp

    src/core/ThreadPool.hpp
    src/core/TaskQueue.hpp
    src/core/Types.hpp
    src/core/Config.hpp

    src/world/Block.hpp
    src/world/BlockRegistry.hpp
    src/world/Chunk.hpp
    src/world/World.hpp
    src/world/WorldGenerator.hpp
    src/world/ParticleSystem.hpp

    src/meshing/GreedyMeshBuilder.hpp

    src/render/Renderer.hpp
    src/render/TextureAtlas.hpp
    src/render/Camera.hpp

    src/player/Player.hpp
    src/player/BlockBreaker.hpp

    src/utils/Log.hpp
    src/utils/Math.hpp
)

add_executable(minecraft_clone ${SOURCES} ${HEADERS})

# ──────────────────────────────────────────────────────────────
# Линковка и include-пути
# ──────────────────────────────────────────────────────────────
target_link_libraries(minecraft_clone PRIVATE
    ${RAYLIB_TARGET}
    Threads::Threads
)

target_include_directories(minecraft_clone PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# ──────────────────────────────────────────────────────────────
# Ассеты
# ──────────────────────────────────────────────────────────────
set(ASSETS_SRC "${CMAKE_SOURCE_DIR}/assets")

target_compile_definitions(minecraft_clone PRIVATE
    ASSETS_DIR="${ASSETS_SRC}"
)

add_custom_command(TARGET minecraft_clone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${ASSETS_SRC}"
        "$<TARGET_FILE_DIR:minecraft_clone>/assets"
    COMMENT "Copying assets to $<TARGET_FILE_DIR:minecraft_clone>/assets"
)

# Кастомные шейдеры для greedy meshing
set(RESOURCES_SRC "${CMAKE_SOURCE_DIR}/resources")
add_custom_command(TARGET minecraft_clone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${RESOURCES_SRC}"
        "$<TARGET_FILE_DIR:minecraft_clone>/resources"
    COMMENT "Copying resources to $<TARGET_FILE_DIR:minecraft_clone>/resources"
)

# ──────────────────────────────────────────────────────────────
# Сводка
# ──────────────────────────────────────────────────────────────
message(STATUS "──────────────────────────────────────────────")
message(STATUS "minecraft_clone configuration:")
message(STATUS "  Build type       : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard     : ${CMAKE_CXX_STANDARD}")
message(STATUS "  raylib target    : ${RAYLIB_TARGET}")
message(STATUS "  LTO enabled      : ${ENABLE_LTO}")
message(STATUS "  Assets source    : ${ASSETS_SRC}")
message(STATUS "──────────────────────────────────────────────")
