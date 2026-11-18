#include "ui/PauseMenu.hpp"
#include "ui/GuiScale.hpp"
#include "core/Config.hpp"
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace mc {

namespace {

struct Button {
    const char* label;
    int x, y, w;               // GUI-px
    bool active;
    MenuAction action;
    bool opensOptions = false; // переход на экран Options
};

constexpr int BTN_H   = 20;
constexpr int GAP     = 4;
constexpr int BLOCK_W = 200;   // ширина полосы кнопок в widgets.png
constexpr int BLOCK_H = 6 * BTN_H + 5 * GAP;  // 140

const Button BUTTONS[] = {
    { "Back to Game",            0,   0, 200, true,  MenuAction::Resume },
    { "Advancements",            0,  24,  98, false, MenuAction::None },
    { "Statistics",            102,  24,  98, false, MenuAction::None },
    { "Mods",                    0,  48, 200, false, MenuAction::None },
    { "Options",                 0,  72,  98, true,  MenuAction::None, true },
    { "Open to LAN",           102,  72,  98, false, MenuAction::None },
    { "Save and Quit to Title",  0,  96, 200, true,  MenuAction::Quit },
};

constexpr const char* TITLE = "Game Menu";
constexpr int TITLE_Y       = 40;

std::u32string toU32(const char* s) {
    std::u32string out;
    for (; *s; ++s)
        out.push_back(static_cast<char32_t>(static_cast<unsigned char>(*s)));
    return out;
}

// ─── Фон кнопки: левая и правая половины полосы 200px из widgets.png ───
// x, y — экранные пиксели; wGui — ширина в GUI-px; s — масштаб.
void drawButtonBg(const Texture2D& tex, int x, int y, int wGui, int srcY, int s) {
    const int halfL = wGui / 2;
    const int halfR = wGui - halfL;

    DrawTexturePro(tex,
        Rectangle{ 0.0f, (float)srcY, (float)halfL, (float)BTN_H },
        Rectangle{ (float)x, (float)y, (float)(halfL * s), (float)(BTN_H * s) },
        {}, 0.0f, WHITE);
    DrawTexturePro(tex,
        Rectangle{ (float)(BLOCK_W - halfR), (float)srcY, (float)halfR, (float)BTN_H },
        Rectangle{ (float)(x + halfL * s), (float)y, (float)(halfR * s), (float)(BTN_H * s) },
        {}, 0.0f, WHITE);
}

// bw, bh — в ЭКРАННЫХ пикселях
void drawTexturedButton(const Texture2D& tex, const McFont& font,
                        const char* label, bool active,
                        int bx, int by, int bw, int bh,
                        bool hover, int s) {
    const int srcY = active ? (hover ? 86 : 66) : 46;
    drawButtonBg(tex, bx, by, bw / s, srcY, s);

    const Color tc = active ? Color{ 255, 255, 255, 255 }
                            : Color{ 160, 160, 160, 255 };
    const std::u32string ulabel = toU32(label);
    const int tw = font.measure(ulabel.c_str()) * s;
    const int tx = bx + (bw - tw) / 2;
    const int ty = by + (bh - 8 * s) / 2;
    font.draw(ulabel.c_str(), tx, ty, s, tc, true);
}

void drawProceduralButton(const char* label, bool active,
                          int bx, int by, int bw, int bh,
                          bool hover, int s) {
    Color fill{ 120, 120, 120, 255 };
    if (!active) fill = Color{ 50, 50, 50, 255 };
    else if (hover) fill = Color{ 160, 160, 160, 255 };
    DrawRectangle(bx, by, bw, bh, fill);

    const Color light{ 200, 200, 200, 255 };
    const Color dark {  60,  60,  60, 255 };
    const int bevel = s;
    DrawRectangle(bx, by, bw, bevel, light);
    DrawRectangle(bx, by, bevel, bh, light);
    DrawRectangle(bx, by + bh - bevel, bw, bevel, dark);
    DrawRectangle(bx + bw - bevel, by, bevel, bh, dark);

    const Color tc = active ? Color{ 255, 255, 255, 255 }
                            : Color{ 160, 160, 160, 255 };
    const int fontSize = 8 * s;
    const int tw = MeasureText(label, fontSize);
    DrawText(label, bx + (bw - tw) / 2, by + (bh - fontSize) / 2, fontSize, tc);
}

// ─── Slider ─────────────────────────────────────────────────────────────
constexpr int SLIDER_W    = 200;
constexpr int SLIDER_H    = 20;
constexpr int HANDLE_W    = 8;
constexpr int RD_MIN      = config::RENDER_DISTANCE_MIN;
constexpr int RD_MAX      = config::RENDER_DISTANCE_MAX;
constexpr int FOV_MIN     = 30;
constexpr int FOV_MAX     = 110;
constexpr int LEAF_LAYERS_MIN = config::LEAVES_VISIBLE_LAYERS_MIN;
constexpr int LEAF_LAYERS_MAX = config::LEAVES_VISIBLE_LAYERS_MAX;

// w — ширина в ЭКРАННЫХ пикселях
void drawSliderTextured(const Texture2D& tex, const McFont& font,
                        const char* label,
                        int x, int y, int w, float t,
                        bool active, bool handleHover, int s) {
    // Трек (всегда тёмный, srcY = 46), та же схема половинок, что у кнопки
    drawButtonBg(tex, x, y, w / s, 46, s);

    // Ручка
    const int handleSrcY = active ? (handleHover ? 86 : 66) : 46;
    const int hx = x + (int)(t * (float)(w - HANDLE_W * s));
    DrawTexturePro(tex,
        Rectangle{ 0.0f, (float)handleSrcY, 4.0f, (float)SLIDER_H },
        Rectangle{ (float)hx, (float)y, (float)(4 * s), (float)(SLIDER_H * s) },
        {}, 0.0f, WHITE);
    DrawTexturePro(tex,
        Rectangle{ 196.0f, (float)handleSrcY, 4.0f, (float)SLIDER_H },
        Rectangle{ (float)(hx + 4 * s), (float)y, (float)(4 * s), (float)(SLIDER_H * s) },
        {}, 0.0f, WHITE);

    const Color tc = active ? Color{ 255, 255, 255, 255 }
                            : Color{ 160, 160, 160, 255 };
    const std::u32string ulabel = toU32(label);
    const int tw = font.measure(ulabel.c_str()) * s;
    const int tx = x + (w - tw) / 2;
    const int ty = y + (SLIDER_H * s - 8 * s) / 2;
    font.draw(ulabel.c_str(), tx, ty, s, tc, true);
}

void drawSliderProcedural(const char* label,
                          int x, int y, int w, float t,
                          bool active, bool handleHover, int s) {
    DrawRectangle(x, y, w, SLIDER_H * s, Color{ 60, 60, 60, 255 });

    const int hw = HANDLE_W * s;
    const int hx = x + (int)(t * (float)(w - hw));
    const Color hc = active ? (handleHover ? Color{ 200, 200, 200, 255 }
                                           : Color{ 150, 150, 150, 255 })
                            : Color{ 100, 100, 100, 255 };
    DrawRectangle(hx, y, hw, SLIDER_H * s, hc);

    const Color tc = active ? Color{ 255, 255, 255, 255 }
                            : Color{ 160, 160, 160, 255 };
    const int fontSize = 8 * s;
    const int tw = MeasureText(label, fontSize);
    DrawText(label, x + (w - tw) / 2, y + (SLIDER_H * s - fontSize) / 2, fontSize, tc);
}

int sliderValueFromMouse(int mx, int sliderX, int sliderW, int handleW) {
    float t = (float)(mx - sliderX - handleW / 2) / (float)(sliderW - handleW);
    t = std::clamp(t, 0.0f, 1.0f);
    return std::clamp(RD_MIN + (int)(t * (RD_MAX - RD_MIN) + 0.5f), RD_MIN, RD_MAX);
}

float fovValueFromMouse(int mx, int sliderX, int sliderW, int handleW) {
    float t = (float)(mx - sliderX - handleW / 2) / (float)(sliderW - handleW);
    t = std::clamp(t, 0.0f, 1.0f);
    return std::clamp((float)FOV_MIN + t * (float)(FOV_MAX - FOV_MIN),
                      (float)FOV_MIN, (float)FOV_MAX);
}

int leafLayersValueFromMouse(int mx, int sliderX, int sliderW, int handleW) {
    float t = (float)(mx - sliderX - handleW / 2) / (float)(sliderW - handleW);
    t = std::clamp(t, 0.0f, 1.0f);
    return std::clamp(LEAF_LAYERS_MIN + (int)(t * (LEAF_LAYERS_MAX - LEAF_LAYERS_MIN) + 0.5f),
                      LEAF_LAYERS_MIN, LEAF_LAYERS_MAX);
}

} // anonymous namespace

// ─── PauseMenu ───────────────────────────────────────────────────────────

bool PauseMenu::load() {
    m_widgetsTex = LoadTexture("assets/minecraft/textures/gui/widgets.png");
    if (m_widgetsTex.id != 0) {
        SetTextureFilter(m_widgetsTex, TEXTURE_FILTER_POINT);
    }

    const bool fontOk = m_font.load();

    if (m_widgetsTex.id == 0 || !fontOk) {
        if (m_widgetsTex.id != 0) UnloadTexture(m_widgetsTex);
        m_widgetsTex = Texture2D{};
        m_font.unload();
        m_loaded = false;
        TraceLog(LOG_WARNING, "Pause menu: assets missing, using procedural fallback");
        return false;
    }
    m_loaded = true;
    return true;
}

void PauseMenu::unload() {
    if (m_widgetsTex.id != 0) UnloadTexture(m_widgetsTex);
    m_widgetsTex = Texture2D{};
    m_font.unload();
    m_loaded = false;
    m_open = false;
    m_skipMouse = false;
    m_screen = Screen::Pause;
    m_draggingSlider = false;
    m_draggingFovSlider = false;
    m_draggingLeafLayers = false;
}

bool PauseMenu::isOpen() const { return m_open; }

void PauseMenu::open() { m_open = true; EnableCursor(); (void)GetMouseDelta(); }

void PauseMenu::close() {
    m_open = false;
    m_screen = Screen::Pause;
    m_draggingSlider = false;
    m_draggingFovSlider = false;
    m_draggingLeafLayers = false;
    DisableCursor();
    m_skipMouse = true;
}

void PauseMenu::toggle() {
    if (m_open) close();
    else        open();
}

void PauseMenu::handleEscape() {
    if (!m_open) { open(); return; }
    switch (m_screen) {
    case Screen::Pause:
        close();
        break;
    case Screen::Options:
        m_screen = Screen::Pause;
        break;
    case Screen::Graphics:
        // Если слайдер тянули в момент Esc, не теряем значение
        if (m_draggingSlider) {
            m_draggingSlider = false;
            if (m_renderDistance != m_dragStartValue) {
                m_pendingRenderDistance = m_renderDistance;
                m_hasRenderDistanceChange = true;
            }
        }
        if (m_draggingFovSlider) {
            m_draggingFovSlider = false;
            if (m_fov != m_dragStartFov) {
                m_pendingFov = m_fov;
                m_hasFovChange = true;
            }
        }
        if (m_draggingLeafLayers) {
            m_draggingLeafLayers = false;
            if (m_leafLayers != m_dragStartLeafLayers) {
                m_pendingLeafLayers = m_leafLayers;
                m_hasLeafLayersChange = true;
            }
        }
        m_screen = Screen::Options;
        break;
    }
}

bool PauseMenu::takeRenderDistanceChange(int& outValue) {
    if (!m_hasRenderDistanceChange) return false;
    outValue = m_pendingRenderDistance;
    m_hasRenderDistanceChange = false;
    return true;
}

void PauseMenu::setRenderDistance(int v) {
    v = std::clamp(v, RD_MIN, RD_MAX);
    m_renderDistance = v;
    m_hasRenderDistanceChange = false;
    m_pendingRenderDistance = 0;
}

bool PauseMenu::takeFovChange(float& outFov) {
    if (!m_hasFovChange) return false;
    outFov = m_pendingFov;
    m_hasFovChange = false;
    return true;
}

void PauseMenu::setFov(float v) {
    v = std::clamp(v, (float)FOV_MIN, (float)FOV_MAX);
    m_fov = v;
    m_hasFovChange = false;
    m_pendingFov = 0.0f;
}

bool PauseMenu::takeLeafLayersChange(int& outValue) {
    if (!m_hasLeafLayersChange) return false;
    outValue = m_pendingLeafLayers;
    m_hasLeafLayersChange = false;
    return true;
}

void PauseMenu::setLeafLayers(int v) {
    v = std::clamp(v, LEAF_LAYERS_MIN, LEAF_LAYERS_MAX);
    m_leafLayers = v;
    m_hasLeafLayersChange = false;
    m_pendingLeafLayers = 0;
}

MenuAction PauseMenu::update() {
    if (!m_open) return MenuAction::None;

    const int s  = guiScale();
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    const int mx = GetMouseX(), my = GetMouseY();

    // ─── Pause screen ─────────────────────────────────────────────────
    if (m_screen == Screen::Pause) {
        if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            return MenuAction::None;

        const int blockLeft = (sw - BLOCK_W * s) / 2;
        const int blockTop  = ((sh / s) - BLOCK_H) / 2 * s;

        for (const Button& b : BUTTONS) {
            if (!b.active) continue;
            const int bx = blockLeft + b.x * s;
            const int by = blockTop  + b.y * s;
            if (mx >= bx && mx < bx + b.w * s &&
                my >= by && my < by + BTN_H * s) {
                if (b.opensOptions) {
                    m_screen = Screen::Options;
                    return MenuAction::None;
                }
                return b.action;
            }
        }
        return MenuAction::None;
    }

    // ─── Options screen ───────────────────────────────────────────────
    if (m_screen == Screen::Options) {
        const int cx = sw / 2;
        const int colL = cx - 155 * s;
        const int btnW = 150 * s;
        const int topRowY = 66 * s;
        const int handleW = HANDLE_W * s;

        // ── FOV slider drag/release ──
        if (m_draggingFovSlider) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                m_fov = fovValueFromMouse(mx, colL, btnW, handleW);
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                m_draggingFovSlider = false;
                if (std::abs(m_fov - m_dragStartFov) > 0.1f) {
                    m_pendingFov = m_fov;
                    m_hasFovChange = true;
                }
            }
            // Don't return; still allow pressing Done while dragging
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            // FOV слайдер (левая колонка, первый ряд)
            if (mx >= colL && mx < colL + btnW &&
                my >= topRowY && my < topRowY + BTN_H * s) {
                m_draggingFovSlider = true;
                m_dragStartFov = m_fov;
                m_fov = fovValueFromMouse(mx, colL, btnW, handleW);
                return MenuAction::None;
            }

            // Done
            {
                const int doneX = (sw - 200 * s) / 2;
                const int doneY = 212 * s;
                if (mx >= doneX && mx < doneX + 200 * s &&
                    my >= doneY && my < doneY + BTN_H * s) {
                    // Commit FOV on Done if dragging ended
                    if (m_draggingFovSlider) {
                        m_draggingFovSlider = false;
                        if (std::abs(m_fov - m_dragStartFov) > 0.1f) {
                            m_pendingFov = m_fov;
                            m_hasFovChange = true;
                        }
                    }
                    m_screen = Screen::Pause;
                    return MenuAction::None;
                }
            }

            // "Graphics Settings" кнопка (ряд 1, левая колонка)
            const int gy = 114 * s;
            if (mx >= colL && mx < colL + btnW &&
                my >= gy   && my < gy + BTN_H * s) {
                if (m_draggingFovSlider) {
                    m_draggingFovSlider = false;
                    if (std::abs(m_fov - m_dragStartFov) > 0.1f) {
                        m_pendingFov = m_fov;
                        m_hasFovChange = true;
                    }
                }
                m_screen = Screen::Graphics;
                return MenuAction::None;
            }
        }
        return MenuAction::None;
    }

    // ─── Graphics screen ──────────────────────────────────────────────
    if (m_screen == Screen::Graphics) {
        const int sliderX = (sw - SLIDER_W * s) / 2;
        const int sliderY = 66 * s;    // дальность прорисовки
        const int leafY   = 92 * s;    // слои листвы
        const int sliderW = SLIDER_W * s;
        const int handleW = HANDLE_W * s;

        if (m_draggingSlider) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                m_renderDistance = sliderValueFromMouse(mx, sliderX, sliderW, handleW);
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                m_draggingSlider = false;
                if (m_renderDistance != m_dragStartValue) {
                    m_pendingRenderDistance = m_renderDistance;
                    m_hasRenderDistanceChange = true;
                }
            }
        }

        if (m_draggingLeafLayers) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                m_leafLayers = leafLayersValueFromMouse(mx, sliderX, sliderW, handleW);
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                m_draggingLeafLayers = false;
                if (m_leafLayers != m_dragStartLeafLayers) {
                    m_pendingLeafLayers = m_leafLayers;
                    m_hasLeafLayersChange = true;
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            // Слайдер дальности прорисовки
            if (mx >= sliderX && mx < sliderX + sliderW &&
                my >= sliderY && my < sliderY + SLIDER_H * s) {
                m_draggingSlider = true;
                m_dragStartValue = m_renderDistance;
                m_renderDistance = sliderValueFromMouse(mx, sliderX, sliderW, handleW);
                return MenuAction::None;
            }

            // Слайдер видимых слоёв листвы
            if (mx >= sliderX && mx < sliderX + sliderW &&
                my >= leafY && my < leafY + SLIDER_H * s) {
                m_draggingLeafLayers = true;
                m_dragStartLeafLayers = m_leafLayers;
                m_leafLayers = leafLayersValueFromMouse(mx, sliderX, sliderW, handleW);
                return MenuAction::None;
            }

            // Done (y = 118 GUI-px)
            const int doneY = 118 * s;
            const int doneX = (sw - 200 * s) / 2;
            if (mx >= doneX && mx < doneX + 200 * s &&
                my >= doneY && my < doneY + BTN_H * s) {
                m_screen = Screen::Options;
                return MenuAction::None;
            }
        }
        return MenuAction::None;
    }

    return MenuAction::None;
}

void PauseMenu::draw() const {
    if (!m_open) return;

    const int s  = guiScale();
    const int sw = GetScreenWidth(), sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, Color{ 0, 0, 0, 96 });

    const int mx = GetMouseX(), my = GetMouseY();

    switch (m_screen) {
    case Screen::Pause:
        drawPauseScreen(s, sw, sh, mx, my);
        break;
    case Screen::Options:
        drawOptionsScreen(s, sw / 2, mx, my);
        break;
    case Screen::Graphics:
        drawGraphicsScreen(s, sw / 2, mx, my);
        break;
    }
}

void PauseMenu::drawPauseScreen(int s, int sw, int sh, int mx, int my) const {
    const int blockLeft = (sw - BLOCK_W * s) / 2;
    const int blockTop  = ((sh / s) - BLOCK_H) / 2 * s;

    // Заголовок
    if (m_loaded) {
        const std::u32string title = toU32(TITLE);
        const int tw = m_font.measure(title.c_str()) * s;
        m_font.draw(title.c_str(), (sw - tw) / 2, TITLE_Y * s, s, WHITE, true);
    } else {
        const int fontSize = 8 * s;
        const int tw = MeasureText(TITLE, fontSize);
        DrawText(TITLE, (sw - tw) / 2, TITLE_Y * s, fontSize, WHITE);
    }

    // Кнопки
    for (const Button& b : BUTTONS) {
        const int bx = blockLeft + b.x * s;
        const int by = blockTop  + b.y * s;
        const int bw = b.w * s;
        const int bh = BTN_H * s;
        const bool hover = b.active && mx >= bx && mx < bx + bw &&
                           my >= by && my < by + bh;

        if (m_loaded) {
            drawTexturedButton(m_widgetsTex, m_font, b.label, b.active,
                               bx, by, bw, bh, hover, s);
        } else {
            drawProceduralButton(b.label, b.active,
                                 bx, by, bw, bh, hover, s);
        }
    }
}

void PauseMenu::drawOptionsScreen(int s, int cx, int mx, int my) const {
    // Заголовок
    const char* title = "Options";
    if (m_loaded) {
        const std::u32string t = toU32(title);
        const int tw = m_font.measure(t.c_str()) * s;
        m_font.draw(t.c_str(), (2 * cx - tw) / 2, TITLE_Y * s, s, WHITE, true);
    } else {
        const int fontSize = 8 * s;
        const int tw = MeasureText(title, fontSize);
        DrawText(title, (2 * cx - tw) / 2, TITLE_Y * s, fontSize, WHITE);
    }

    // ─── FOV (активный слайдер) + Online (неактивная кнопка) ───
    const int topRowY = 66 * s;
    const int colL = cx - 155 * s;
    const int colR = cx + 5 * s;
    const int btnW = 150 * s;
    const float fovT = (m_fov - (float)FOV_MIN) / (float)(FOV_MAX - FOV_MIN);
    const int handleW = HANDLE_W * s;
    const int handleX = colL + (int)(fovT * (float)(btnW - handleW));
    const bool handleHover = m_draggingFovSlider ||
                             (mx >= handleX && mx < handleX + handleW &&
                              my >= topRowY && my < topRowY + BTN_H * s);

    char fovLabel[64];
    snprintf(fovLabel, sizeof(fovLabel), "FOV: %.0f", m_fov);

    if (m_loaded) {
        drawSliderTextured(m_widgetsTex, m_font, fovLabel,
                           colL, topRowY, btnW, fovT, true, handleHover, s);
        drawTexturedButton(m_widgetsTex, m_font, "Online", false,
                           colR, topRowY, btnW, BTN_H * s, false, s);
    } else {
        drawSliderProcedural(fovLabel,
                             colL, topRowY, btnW, fovT, true, handleHover, s);
        drawProceduralButton("Online", false,
                             colR, topRowY, btnW, BTN_H * s, false, s);
    }

    // ─── Сетка 5 рядов × 2 колонки ───
    const int gridRows[] = { 90, 114, 138, 162, 186 };
    const char* gridLabels[5][2] = {
        { "Skin Customization", "Music & Sounds" },
        { "Graphics Settings",  "Controls" },
        { "Language",           "Chat Settings" },
        { "Resource Packs",     "Accessibility" },
        { "Telemetry Data",     "Credits & Attribution" },
    };
    const bool gridActive[5][2] = {
        { false, false }, { true,  false },
        { false, false }, { false, false },
        { false, false },
    };

    for (int row = 0; row < 5; ++row) {
        const int gy = gridRows[row] * s;
        for (int col = 0; col < 2; ++col) {
            const int gx = (col == 0) ? colL : colR;
            const bool hover = gridActive[row][col] &&
                               mx >= gx && mx < gx + btnW &&
                               my >= gy && my < gy + BTN_H * s;
            if (m_loaded)
                drawTexturedButton(m_widgetsTex, m_font, gridLabels[row][col],
                                   gridActive[row][col], gx, gy, btnW,
                                   BTN_H * s, hover, s);
            else
                drawProceduralButton(gridLabels[row][col], gridActive[row][col],
                                     gx, gy, btnW, BTN_H * s, hover, s);
        }
    }

    // ─── Done ───
    const int doneY = 212 * s;
    const int doneW = 200 * s;
    const int doneX = cx - 100 * s;
    const bool doneHover = mx >= doneX && mx < doneX + doneW &&
                           my >= doneY && my < doneY + BTN_H * s;
    if (m_loaded)
        drawTexturedButton(m_widgetsTex, m_font, "Done", true,
                           doneX, doneY, doneW, BTN_H * s, doneHover, s);
    else
        drawProceduralButton("Done", true,
                             doneX, doneY, doneW, BTN_H * s, doneHover, s);
}

void PauseMenu::drawGraphicsScreen(int s, int cx, int mx, int my) const {
    // Заголовок
    const char* title = "Graphics Settings";
    if (m_loaded) {
        const std::u32string t = toU32(title);
        const int tw = m_font.measure(t.c_str()) * s;
        m_font.draw(t.c_str(), (2 * cx - tw) / 2, TITLE_Y * s, s, WHITE, true);
    } else {
        const int fontSize = 8 * s;
        const int tw = MeasureText(title, fontSize);
        DrawText(title, (2 * cx - tw) / 2, TITLE_Y * s, fontSize, WHITE);
    }

    // ─── Слайдер дальности прорисовки ───
    const int sliderX = cx - 100 * s;
    const int sliderY = 66 * s;
    const int sliderW = SLIDER_W * s;
    const int handleW = HANDLE_W * s;

    {
        const float t = (float)(m_renderDistance - RD_MIN) / (float)(RD_MAX - RD_MIN);
        const int handleX = sliderX + (int)(t * (float)(sliderW - handleW));
        const bool handleHover = m_draggingSlider ||
                                 (mx >= handleX && mx < handleX + handleW &&
                                  my >= sliderY && my < sliderY + SLIDER_H * s);

        char labelBuf[64];
        snprintf(labelBuf, sizeof(labelBuf), "Render Distance: %d chunks", m_renderDistance);

        if (m_loaded) {
            drawSliderTextured(m_widgetsTex, m_font, labelBuf,
                               sliderX, sliderY, sliderW, t,
                               true, handleHover, s);
        } else {
            drawSliderProcedural(labelBuf,
                                 sliderX, sliderY, sliderW, t,
                                 true, handleHover, s);
        }
    }

    // ─── Слайдер видимых слоёв листвы ───
    const int leafY = 92 * s;
    {
        const float t = (float)(m_leafLayers - LEAF_LAYERS_MIN) /
                        (float)(LEAF_LAYERS_MAX - LEAF_LAYERS_MIN);
        const int handleX = sliderX + (int)(t * (float)(sliderW - handleW));
        const bool handleHover = m_draggingLeafLayers ||
                                 (mx >= handleX && mx < handleX + handleW &&
                                  my >= leafY && my < leafY + SLIDER_H * s);

        char labelBuf[64];
        snprintf(labelBuf, sizeof(labelBuf), "Foliage Layers: %d", m_leafLayers);

        if (m_loaded) {
            drawSliderTextured(m_widgetsTex, m_font, labelBuf,
                               sliderX, leafY, sliderW, t,
                               true, handleHover, s);
        } else {
            drawSliderProcedural(labelBuf,
                                 sliderX, leafY, sliderW, t,
                                 true, handleHover, s);
        }
    }

    // ─── Done ───
    const int doneY = 118 * s;
    const int doneW = 200 * s;
    const int doneX = cx - 100 * s;
    const bool doneHover = mx >= doneX && mx < doneX + doneW &&
                           my >= doneY && my < doneY + BTN_H * s;
    if (m_loaded)
        drawTexturedButton(m_widgetsTex, m_font, "Done", true,
                           doneX, doneY, doneW, BTN_H * s, doneHover, s);
    else
        drawProceduralButton("Done", true,
                             doneX, doneY, doneW, BTN_H * s, doneHover, s);
}

bool PauseMenu::consumeMouseSkip() {
    const bool v = m_skipMouse;
    m_skipMouse = false;
    return v;
}

} // namespace mc