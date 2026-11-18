#pragma once
#include "core/Config.hpp"
#include "ui/McFont.hpp"

namespace mc {

enum class MenuAction { None, Resume, Quit };
enum class Screen { Pause, Options, Graphics };

class PauseMenu {
public:
    bool load();            // текстуры; при неудаче работает в процедурном режиме
    void unload();

    bool isOpen() const;
    void open();            // EnableCursor()
    void close();           // DisableCursor() + флаг пропуска mouse delta
    void toggle();
    void handleEscape();    // per-screen Esc: close / go up
    MenuAction update();    // клики по активным кнопкам, возвращает действие
    void draw() const;

    bool consumeMouseSkip();                     // true один раз после close()
    bool takeRenderDistanceChange(int& outValue); // true один раз после отпускания слайдера
    void setRenderDistance(int v);                // инициализация значения слайдера
    bool takeFovChange(float& outFov);            // true один раз после отпускания слайдера FOV
    void setFov(float v);                         // инициализация значения слайдера FOV
    bool takeLeafLayersChange(int& outValue);     // true один раз после отпускания слайдера слоёв листвы
    void setLeafLayers(int v);                    // инициализация значения слайдера слоёв листвы

private:
    void drawPauseScreen(int s, int sw, int sh, int mx, int my) const;
    void drawOptionsScreen(int s, int cx, int mx, int my) const;
    void drawGraphicsScreen(int s, int cx, int mx, int my) const;

    Texture2D m_widgetsTex{};
    McFont    m_font;
    bool m_loaded = false;
    bool m_open = false;
    bool m_skipMouse = false;

    Screen m_screen = Screen::Pause;

    // Слайдер дальности прорисовки
    int   m_renderDistance = config::RENDER_DISTANCE;
    bool  m_draggingSlider = false;
    int   m_dragStartValue = 0;
    bool  m_hasRenderDistanceChange = false;
    int   m_pendingRenderDistance = 0;

    // Слайдер FOV
    float m_fov                 = 75.0f;
    bool  m_draggingFovSlider   = false;
    float m_dragStartFov        = 75.0f;
    bool  m_hasFovChange        = false;
    float m_pendingFov          = 75.0f;

    // Слайдер видимых слоёв листвы
    int  m_leafLayers             = config::LEAVES_VISIBLE_LAYERS_DEFAULT;
    bool m_draggingLeafLayers     = false;
    int  m_dragStartLeafLayers    = 0;
    bool m_hasLeafLayersChange    = false;
    int  m_pendingLeafLayers      = 0;
};

} // namespace mc