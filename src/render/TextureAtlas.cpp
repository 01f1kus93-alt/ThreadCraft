#include "render/TextureAtlas.hpp"
#include <raylib.h>

#include <algorithm>
#include <cstdint>

namespace mc {

namespace {

    // Minecraft plains grass (#91BD59) — цвет травы текущего биома.
    // Используется для заливки фона атласа и запекания grayscale-травы.
    const Color kGrassTint  = { 145, 189, 89, 255 }; // Plains grass (#91BD59)
    const Color kLeavesTint = { 0x77, 0xAB, 0x2F, 255 }; // Plains foliage (#77AB2F)

    struct TileFile { int index; const char* file; };

    // Тайловые индексы совпадают с BlockRegistry (stone=0, dirt=1, grass_top=2, grass_side=3)
    constexpr TileFile TILE_FILES[] = {
        {0, "stone.png"},
        {1, "dirt.png"},
        {2, "grass_block_top.png"},
        {3, "grass_block_side.png"},
        {5, "deepslate.png"},
        {6, "bedrock.png"},
        {8, "sand.png"},
        {9, "oak_log.png"},
        {10, "oak_leaves.png"},
        {11, "coal_ore.png"},
        {12, "iron_ore.png"},
        {13, "deepslate_gold_ore.png"},
    };

    // Ванильные текстуры травы и листвы — grayscale; цвет накладывается в рантайме.
    // Запекаем фиксированный тинт только в истинно серые пиксели (R≈G≈B).
    void bakeGrayscaleTint(Image& img, Color tint) noexcept {
        if (img.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) return;

        Color* pixels = reinterpret_cast<Color*>(img.data);
        const int count = img.width * img.height;

        for (int i = 0; i < count; ++i) {
            Color c = pixels[i];
            if (c.a == 0) continue;

            const int mx = std::max({ c.r, c.g, c.b });
            const int mn = std::min({ c.r, c.g, c.b });
            
            // Строгий фильтр: только истинно серые пиксели (R≈G≈B).
            // Порог 8 не даёт зацепить коричневые/сероватые камешки в земле.
            if (mx - mn <= 8) {
                c.r = (uint8_t)(c.r * tint.r / 255);
                c.g = (uint8_t)(c.g * tint.g / 255);
                c.b = (uint8_t)(c.b * tint.b / 255);
                pixels[i] = c;
            }
        }
    }

    // Вставка тайла в атлас с replication-padding (texture bleeding).
    // Каждый тайл окружён отступом из повторённых краевых пикселей,
    // чтобы билинейная/трилинейная выборка и мипмапы не задевали соседние тайлы.
    void drawPaddedTile(Image* atlas, const Image* tile, int destX, int destY, int pad) noexcept {
        const int ts = (int)tile->width;
        const int aw = (int)atlas->width;
        Color* aPix = reinterpret_cast<Color*>(atlas->data);
        const Color* tPix = reinterpret_cast<const Color*>(tile->data);

        // 1. Копируем внутреннюю область тайла 1:1
        for (int y = 0; y < ts; ++y) {
            for (int x = 0; x < ts; ++x) {
                aPix[(destY + pad + y) * aw + (destX + pad + x)] = tPix[y * ts + x];
            }
        }

        // 2. Левый и правый padding — повторяем крайние столбцы тайла
        for (int y = 0; y < ts; ++y) {
            const Color leftCol   = tPix[y * ts + 0];
            const Color rightCol  = tPix[y * ts + ts - 1];
            const int row = (destY + pad + y) * aw;
            for (int p = 0; p < pad; ++p) {
                aPix[row + (destX + p)]           = leftCol;
                aPix[row + (destX + pad + ts + p)] = rightCol;
            }
        }

        // 3. Верхний и нижний padding — повторяем крайние строки
        for (int x = 0; x < ts; ++x) {
            const Color topCol = tPix[0 * ts + x];
            const Color botCol = tPix[(ts - 1) * ts + x];
            const int tx = destX + pad + x;
            for (int p = 0; p < pad; ++p) {
                aPix[(destY + p) * aw + tx]            = topCol;
                aPix[(destY + pad + ts + p) * aw + tx]  = botCol;
            }
        }

        // 4. Угловые квадраты padding (повторяем пиксели углов)
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
    // Заливка целой ячейки тайла (включая padding) одним цветом.
    // Используется для процедурных тайлов без файла текстуры (например, вода).
    void fillTile(Image* atlas, int tileIndex, Color color) noexcept {
        const int pitch = config::ATLAS_TILE_SIZE + 2 * config::ATLAS_TILE_PAD;
        const int col   = tileIndex % config::ATLAS_COLUMNS;
        const int row   = tileIndex / config::ATLAS_COLUMNS;
        const int dx = col * pitch;
        const int dy = row * pitch;

        Color* aPix = reinterpret_cast<Color*>(atlas->data);
        for (int y = 0; y < pitch; ++y) {
            for (int x = 0; x < pitch; ++x) {
                aPix[(dy + y) * atlas->width + (dx + x)] = color;
            }
        }
    }
} // namespace

bool TextureAtlas::load(const std::string& blockDir) {
    const int pad   = config::ATLAS_TILE_PAD;
    const int pitch = config::ATLAS_TILE_SIZE + 2 * pad;
    const int atlasW = config::ATLAS_COLUMNS * pitch;
    const int atlasH = config::ATLAS_ROWS    * pitch;

    // Фон атласа (виден только если какой-то padding остался незаполнен):
    // затемнённая трава.
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
        if (tile.data == nullptr) {
            TraceLog(LOG_ERROR, "TextureAtlas: failed to load '%s'", path.c_str());
            UnloadImage(atlas);
            return false;
        }

        ImageFormat(&tile, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        if (tile.width != config::ATLAS_TILE_SIZE || tile.height != config::ATLAS_TILE_SIZE) {
            ImageResize(&tile, config::ATLAS_TILE_SIZE, config::ATLAS_TILE_SIZE);
        }

        // Запекаем зелёный тинт в grayscale-пиксели травы и листвы
        if (tf.index == 2 || tf.index == 3) {
            bakeGrayscaleTint(tile, kGrassTint);
        } else if (tf.index == 10) {
            bakeGrayscaleTint(tile, kLeavesTint);
        }

        const int col = tf.index % config::ATLAS_COLUMNS;
        const int row = tf.index / config::ATLAS_COLUMNS;
        const int destX = col * pitch;
        const int destY = row * pitch;

        drawPaddedTile(&atlas, &tile, destX, destY, pad);

        UnloadImage(tile);
    }

    // Вода (BlockRegistry::WATER == 7) не загружается из файла, поэтому
    // заливаем её тайл синим; иначе он останется цветом фона (тёмная трава),
    // из-за чего вода выглядит зелёной.
    const Color kWaterColor = { 0, 100, 255, 200 };
    fillTile(&atlas, 7, kWaterColor);

    m_texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);

    // ─────────────────────────────────────────────────────────────
    // Мипмапы + анизотропная трилинейная фильтрация + CLAMP:
    //   • GenTextureMipmaps — предварительный расчёт мип-цепочки
    //     для дистанционного сглаживания текстур.
    //   • TEXTURE_FILTER_ANISOTROPIC_8X — билинейная + мип-уровни +
    //     анизотропное сглаживание, устраняющее «рябь»/мерцание
    //     дальних тайлов под любым углом.
    //   • TEXTURE_WRAP_CLAMP — не даёт UV выйти за границы [0,1].
    // ─────────────────────────────────────────────────────────────
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

    // Внутренняя (без padding) область тайла в пикселях атласа
    const float x0 = (float)(col * pitch + config::ATLAS_TILE_PAD);
    const float y0 = (float)(row * pitch + config::ATLAS_TILE_PAD);
    const float x1 = x0 + config::ATLAS_TILE_SIZE;
    const float y1 = y0 + config::ATLAS_TILE_SIZE;

    // Отступ в полпикселя: при линейной фильтрации + мипмапах
    // UV-координаты попадают в центр крайних текселей тайла,
    // а не на стык с padding соседних тайлов. На высоких
    // мип-уровнях отступ становится субпиксельным и ни на что
    // не влияет.
    const float hx = 0.5f / atlasW;
    const float hy = 0.5f / atlasH;

    return {
        (x0 / atlasW) + hx,
        (y0 / atlasH) + hy,
        (x1 / atlasW) - hx,
        (y1 / atlasH) - hy
    };
}

} // namespace mc