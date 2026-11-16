#include "render/TextureAtlas.hpp"
#include <raylib.h>

#include <algorithm>
#include <cstdint>

namespace mc {

namespace {

    const Color kGrassTint  = { 145, 189, 89, 255 }; 
    const Color kLeavesTint = { 0x77, 0xAB, 0x2F, 255 };
    const Color kWaterTint  = { 63, 118, 228, 255 };

    struct TileFile { int index; const char* file; const char* fallback; };

    constexpr TileFile TILE_FILES[] = {
        {0, "stone.png", nullptr},
        {1, "dirt.png", nullptr},
        {2, "grass_block_top.png", nullptr},
        {3, "grass_block_side.png", nullptr},
        {5, "deepslate.png", nullptr},
        {6, "bedrock.png", nullptr},
        {7, "water_still.png", nullptr},
        {8, "sand.png", nullptr},
        {9, "oak_log.png", nullptr},
        {10, "oak_leaves.png", nullptr},
        {11, "coal_ore.png", nullptr},
        {12, "iron_ore.png", nullptr},
        {13, "deepslate_gold_ore.png", nullptr},
        {14, "birch_log.png", "oak_log.png"},
        {15, "birch_leaves.png", "oak_leaves.png"},
    };

    void bakeGrayscaleTint(Image& img, Color tint) noexcept {
        if (img.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) return;

        Color* pixels = reinterpret_cast<Color*>(img.data);
        const int count = img.width * img.height;

        for (int i = 0; i < count; ++i) {
            Color c = pixels[i];
            if (c.a == 0) continue;
            const int mx = std::max({ c.r, c.g, c.b });
            const int mn = std::min({ c.r, c.g, c.b });
            
            if (mx - mn <= 8) {
                c.r = (uint8_t)(c.r * tint.r / 255);
                c.g = (uint8_t)(c.g * tint.g / 255);
                c.b = (uint8_t)(c.b * tint.b / 255);
                pixels[i] = c;
            }
        }
    }

    void drawPaddedTile(Image* atlas, const Image* tile, int destX, int destY, int pad) noexcept {
        const int ts = (int)tile->width;
        const int aw = (int)atlas->width;
        Color* aPix = reinterpret_cast<Color*>(atlas->data);
        const Color* tPix = reinterpret_cast<const Color*>(tile->data);

        for (int y = 0; y < ts; ++y) {
            for (int x = 0; x < ts; ++x) {
                aPix[(destY + pad + y) * aw + (destX + pad + x)] = tPix[y * ts + x];
            }
        }

        for (int y = 0; y < ts; ++y) {
            const Color leftCol   = tPix[y * ts + 0];
            const Color rightCol  = tPix[y * ts + ts - 1];
            const int row = (destY + pad + y) * aw;
            for (int p = 0; p < pad; ++p) {
                aPix[row + (destX + p)]           = leftCol;
                aPix[row + (destX + pad + ts + p)] = rightCol;
            }
        }

        for (int x = 0; x < ts; ++x) {
            const Color topCol = tPix[0 * ts + x];
            const Color botCol = tPix[(ts - 1) * ts + x];
            const int tx = destX + pad + x;
            for (int p = 0; p < pad; ++p) {
                aPix[(destY + p) * aw + tx]            = topCol;
                aPix[(destY + pad + ts + p) * aw + tx]  = botCol;
            }
        }

        const Color tl = tPix[0];
        const Color tr = tPix[ts - 1];
        const Color bl = tPix[(ts - 1) * ts];
        const Color br = tPix[(ts - 1) * ts + ts - 1];
        for (int p = 0; p < pad; ++p) {
            for (int q = 0; q < pad; ++q) {
                aPix[(destY + q) * aw + (destX + p)]                           = tl;
                aPix[(destY + q) * aw + (destX + pad + ts + p)]                 = tr;
                aPix[(destY + pad + ts + q) * aw + (destX + p)]                 = bl;
                aPix[(destY + pad + ts + q) * aw + (destX + pad + ts + p)]       = br;
            }
        }
    }
} // namespace

bool TextureAtlas::load(const std::string& blockDir) {
    const int pad   = config::ATLAS_TILE_PAD;
    const int pitch = config::ATLAS_TILE_SIZE + 2 * pad;
    const int atlasW = config::ATLAS_COLUMNS * pitch;
    const int atlasH = config::ATLAS_ROWS    * pitch;

    const Color kDarkGrass = {
        static_cast<uint8_t>(kGrassTint.r * 0.65f),
        static_cast<uint8_t>(kGrassTint.g * 0.65f),
        static_cast<uint8_t>(kGrassTint.b * 0.65f),
        kGrassTint.a
    };
    Image atlas = GenImageColor(atlasW, atlasH, kDarkGrass);

    for (const auto& tf : TILE_FILES) {
        std::string path = blockDir + "/" + tf.file;
        Image tile = LoadImage(path.c_str());
        if (tile.data == nullptr && tf.fallback != nullptr) {
            TraceLog(LOG_WARNING, "TextureAtlas: '%s' not found, using fallback '%s'",
                     tf.file, tf.fallback);
            std::string fallbackPath = blockDir + "/" + tf.fallback;
            tile = LoadImage(fallbackPath.c_str());
        }
        if (tile.data == nullptr) {
            TraceLog(LOG_ERROR, "TextureAtlas: failed to load '%s'", path.c_str());
            UnloadImage(atlas);
            return false;
        }

        ImageFormat(&tile, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        if (tf.index == 7) {
            ImageCrop(&tile, Rectangle{ 0.0f, 0.0f,
                (float)config::ATLAS_TILE_SIZE, (float)config::ATLAS_TILE_SIZE });
        }

        if (tile.width != config::ATLAS_TILE_SIZE || tile.height != config::ATLAS_TILE_SIZE) {
            ImageResize(&tile, config::ATLAS_TILE_SIZE, config::ATLAS_TILE_SIZE);
        }

        if (tf.index == 2 || tf.index == 3) {
            bakeGrayscaleTint(tile, kGrassTint);
        } else if (tf.index == 10) {
            bakeGrayscaleTint(tile, kLeavesTint);
        } else if (tf.index == 7) {
            bakeGrayscaleTint(tile, kWaterTint);
        }

        const int col = tf.index % config::ATLAS_COLUMNS;
        const int row = tf.index / config::ATLAS_COLUMNS;
        const int destX = col * pitch;
        const int destY = row * pitch;

        drawPaddedTile(&atlas, &tile, destX, destY, pad);

        UnloadImage(tile);
    }

    // Cache the center-pixel color of each tile (particles / entity cubes).
    m_centerColors.fill(WHITE);
    for (const auto& tf : TILE_FILES) {
        const int col = tf.index % config::ATLAS_COLUMNS;
        const int row = tf.index / config::ATLAS_COLUMNS;
        const int px  = col * pitch + pad + config::ATLAS_TILE_SIZE / 2;
        const int py  = row * pitch + pad + config::ATLAS_TILE_SIZE / 2;
        m_centerColors[tf.index] = reinterpret_cast<Color*>(atlas.data)[py * atlasW + px];
    }
    m_hasCenterColors = true;

    m_texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);

    GenTextureMipmaps(&m_texture);
    SetTextureFilter(m_texture, TEXTURE_FILTER_ANISOTROPIC_8X);
    SetTextureWrap(m_texture, TEXTURE_WRAP_CLAMP);

    m_loaded = true;
    TraceLog(LOG_INFO, "TextureAtlas loaded: %dx%d (tile=%dpx, pad=%d, pitch=%d)",
             atlasW, atlasH, config::ATLAS_TILE_SIZE, pad, pitch);
    return true;
}

void TextureAtlas::unload() {
    if (m_loaded) {
        UnloadTexture(m_texture);
        m_texture = Texture2D{};
        m_loaded = false;
    }
}

TileUV TextureAtlas::tileUV(int tileIndex) const noexcept {
    if (tileIndex < 0) return { 0, 0, 0, 0 };
    const int col   = tileIndex % config::ATLAS_COLUMNS;
    const int row   = tileIndex / config::ATLAS_COLUMNS;
    const int pitch = config::ATLAS_TILE_SIZE + 2 * config::ATLAS_TILE_PAD;

    const float atlasW = (float)(config::ATLAS_COLUMNS * pitch);
    const float atlasH = (float)(config::ATLAS_ROWS    * pitch);

    const float x0 = (float)(col * pitch + config::ATLAS_TILE_PAD);
    const float y0 = (float)(row * pitch + config::ATLAS_TILE_PAD);

    // Half-texel offset для u0/v0 (защита от прозрачности на краях)
    const float hx = 0.5f / atlasW;
    const float hy = 0.5f / atlasH;
    
    // Полный размер тайла (16 пикселей вместо 15)
    const float tu = (float)config::ATLAS_TILE_SIZE / atlasW;
    const float tv = (float)config::ATLAS_TILE_SIZE / atlasH;

    const float u0 = (x0 / atlasW) + hx;
    const float v0 = (y0 / atlasH) + hy;

    return {
        u0,
        v0,
        u0 + tu,
        v0 + tv
    };
}

Color TextureAtlas::centerColor(int tileIndex) const noexcept {
    if (tileIndex < 0 || tileIndex >= (int)m_centerColors.size() || !m_hasCenterColors)
        return WHITE;
    return m_centerColors[tileIndex];
}

} // namespace mc