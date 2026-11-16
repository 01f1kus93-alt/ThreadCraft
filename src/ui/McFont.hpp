#pragma once
#include <raylib.h>
#include <cstdint>
#include <algorithm>

namespace mc {

// Глиф: шрифт, колонка/строка, ячейка, ascent, advance
struct McGlyph {
    int texIdx;   // 0=ascii, 1=nonlatin, 2=accented
    int col, row;
    int w, h;
    int ascent;
    int advance;
};

// Загруженные текстуры шрифта + отрисовка текста
struct McFont {
    Texture2D tex[3] = {};

    static constexpr int TEX_ASCII    = 0;
    static constexpr int TEX_NONLATIN = 1;
    static constexpr int TEX_ACCENTED = 2;

    // Ширина (advance) каждой ячейки 16x16, считается из текстуры при load():
    // [0] = ascii.png, [1] = nonlatin_european.png.
    // 0 = не посчитано (используется значение из таблицы глифов).
    uint8_t m_adv[2][256] = {};

    // Advance = (самый правый непрозрачный столбец + 1) + 1 пиксель зазора.
    // Пустая ячейка = 4 (ширина пробела).
    static void calcAdvances(const char* path, uint8_t* out) {
        Image img = LoadImage(path);
        if (img.data == nullptr) return;
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        const Color* px = reinterpret_cast<const Color*>(img.data);
        const int cellsX = std::min(img.width  / 8, 16);
        const int cellsY = std::min(img.height / 8, 16);

        for (int cy = 0; cy < cellsY; ++cy) {
            for (int cx = 0; cx < cellsX; ++cx) {
                int w = 0;
                for (int x = 0; x < 8; ++x) {
                    for (int y = 0; y < 8; ++y) {
                        if (px[(cy * 8 + y) * img.width + cx * 8 + x].a > 0) {
                            w = x + 1;
                            break;
                        }
                    }
                }
                out[cy * 16 + cx] = (uint8_t)(w > 0 ? w + 1 : 4);
            }
        }
        UnloadImage(img);
    }

    // ─── Таблица глифов без учёта посчитанной ширины ───
    bool glyphRaw(char32_t cp, McGlyph& g) const noexcept {
        // ASCII printable (кроме пробела)
        if (cp >= 0x21 && cp <= 0x7E) {
            int idx = (int)cp;
            g = { TEX_ASCII, idx % 16, idx / 16, 8, 8, 7, 8 };
            return true;
        }
        // Кириллица — по include/default.json
        switch (cp) {
        case U'В': g={TEX_NONLATIN, 8,4, 8,8,7,8}; return true;
        case U'Д': g={TEX_NONLATIN,10,4, 8,8,7,8}; return true;
        case U'М': g={TEX_NONLATIN, 1,5, 8,8,7,8}; return true;
        case U'Н': g={TEX_NONLATIN, 2,5, 8,8,7,8}; return true;
        case U'О': g={TEX_NONLATIN, 3,5, 8,8,7,8}; return true;
        case U'С': g={TEX_NONLATIN, 6,5, 8,8,7,8}; return true;
        case U'а': g={TEX_NONLATIN, 5,6, 8,8,7,8}; return true;
        case U'в': g={TEX_NONLATIN, 7,6, 8,8,7,8}; return true;
        case U'г': g={TEX_NONLATIN, 8,6, 8,8,7,8}; return true;
        case U'д': g={TEX_NONLATIN, 9,6, 8,8,7,8}; return true;
        case U'е': g={TEX_NONLATIN,10,6, 8,8,7,8}; return true;
        case U'ж': g={TEX_NONLATIN,11,6, 8,8,7,8}; return true;
        case U'и': g={TEX_NONLATIN,13,6, 8,8,7,8}; return true;
        case U'к': g={TEX_NONLATIN,14,6, 8,8,7,8}; return true;
        case U'л': g={TEX_NONLATIN,15,6, 8,8,7,8}; return true;
        case U'м': g={TEX_NONLATIN, 0,7, 8,8,7,8}; return true;
        case U'н': g={TEX_NONLATIN, 1,7, 8,8,7,8}; return true;
        case U'о': g={TEX_NONLATIN, 2,7, 8,8,7,8}; return true;
        case U'р': g={TEX_NONLATIN, 4,7, 8,8,7,8}; return true;
        case U'с': g={TEX_NONLATIN, 5,7, 8,8,7,8}; return true;
        case U'т': g={TEX_NONLATIN, 6,7, 8,8,7,8}; return true;
        case U'у': g={TEX_NONLATIN, 7,7, 8,8,7,8}; return true;
        case U'х': g={TEX_NONLATIN, 9,7, 8,8,7,8}; return true;
        case U'ы': g={TEX_NONLATIN,15,7, 8,8,7,8}; return true;
        case U'ь': g={TEX_NONLATIN, 0,8, 8,8,7,8}; return true;
        case U'ю': g={TEX_NONLATIN, 2,8, 8,8,7,8}; return true;
        case U'я': g={TEX_NONLATIN, 3,8, 8,8,7,8}; return true;
        // «й» — в accented.png (9×12, ascent 10, advance 9)
        case U'й': g={TEX_ACCENTED, 5,13, 9,12,10,9}; return true;
        default: return false;
        }
    }

    // Глиф с реальной шириной, посчитанной из текстуры
    bool glyph(char32_t cp, McGlyph& g) const noexcept {
        if (!glyphRaw(cp, g)) return false;
        if (g.texIdx <= 1) {
            const uint8_t a = m_adv[g.texIdx][g.row * 16 + g.col];
            if (a != 0) g.advance = a;
        }
        return true;
    }

    bool load() {
        tex[TEX_ASCII] = LoadTexture("assets/minecraft/textures/font/ascii.png");
        if (tex[TEX_ASCII].id == 0) { unload(); return false; }
        SetTextureFilter(tex[TEX_ASCII], TEXTURE_FILTER_POINT);

        tex[TEX_NONLATIN] = LoadTexture("assets/minecraft/textures/font/nonlatin_european.png");
        if (tex[TEX_NONLATIN].id == 0) { unload(); return false; }
        SetTextureFilter(tex[TEX_NONLATIN], TEXTURE_FILTER_POINT);

        tex[TEX_ACCENTED] = LoadTexture("assets/minecraft/textures/font/accented.png");
        if (tex[TEX_ACCENTED].id == 0) { unload(); return false; }
        SetTextureFilter(tex[TEX_ACCENTED], TEXTURE_FILTER_POINT);

        calcAdvances("assets/minecraft/textures/font/ascii.png",             m_adv[0]);
        calcAdvances("assets/minecraft/textures/font/nonlatin_european.png", m_adv[1]);
        return true;
    }

    void unload() {
        for (auto& t : tex) {
            if (t.id != 0) UnloadTexture(t);
            t = Texture2D{};
        }
    }

    // Ширина строки в GUI-пикселях
    int measure(const char32_t* s) const noexcept {
        int w = 0;
        for (int i = 0; s[i]; ++i) {
            if (s[i] == U' ') { w += 4; continue; }
            McGlyph g;
            w += glyph(s[i], g) ? g.advance : 8;
        }
        // последний символ не нужен зазор справа
        return w > 0 ? w - 1 : 0;
    }

    // Нарисовать строку. (x,y) — левый верхний угол в SCREEN-пикселях
    void draw(const char32_t* s, int x, int y, int scale,
              Color color, bool shadow) const noexcept {
        int cx = x;
        int baselineY = y + 7 * scale;          // default ascent 7
        for (int i = 0; s[i]; ++i) {
            if (s[i] == U' ') { cx += 4 * scale; continue; }
            McGlyph g;
            if (!glyph(s[i], g)) { cx += 8 * scale; continue; }

            Rectangle src = {
                (float)(g.col * g.w), (float)(g.row * g.h),
                (float)g.w, (float)g.h
            };
            int drawY = baselineY - g.ascent * scale;
            Rectangle dst = {
                (float)cx, (float)drawY,
                (float)(g.w * scale), (float)(g.h * scale)
            };

            if (shadow) {
                Color sh = { (uint8_t)(color.r/4), (uint8_t)(color.g/4),
                             (uint8_t)(color.b/4), (uint8_t)(color.a/4) };
                Rectangle shDst = { dst.x + scale, dst.y + scale,
                                    dst.width, dst.height };
                DrawTexturePro(tex[g.texIdx], src, shDst, {}, 0, sh);
            }
            DrawTexturePro(tex[g.texIdx], src, dst, {}, 0, color);
            cx += g.advance * scale;
        }
    }
};

} // namespace mc