#pragma once
#include "core/Config.hpp"
#include <raylib.h>
#include <string>
#include <array>

namespace mc {

struct TileUV {
    float u0, v0, u1, v1;
};

class TextureAtlas {
public:
    bool load(const std::string& blockDir);
    void unload();

    Texture2D texture() const noexcept { return m_texture; }
    bool isLoaded() const noexcept { return m_loaded; }

    TileUV tileUV(int tileIndex) const noexcept;
    Color  centerColor(int tileIndex) const noexcept;

private:
    Texture2D m_texture{};
    bool m_loaded = false;

    std::array<Color, config::ATLAS_COLUMNS * config::ATLAS_ROWS> m_centerColors{};
    bool m_hasCenterColors = false;
};

// Глобальный указатель для доступа из рабочих потоков
extern TextureAtlas* g_textureAtlas;

} // namespace mc