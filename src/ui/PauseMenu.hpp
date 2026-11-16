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
};

} // namespace mc