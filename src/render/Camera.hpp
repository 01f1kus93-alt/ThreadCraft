#pragma once
#include <raylib.h>

namespace mc {

class FlyCamera {
public:
    FlyCamera();
    ~FlyCamera();

    void update(float dt);
    void updateLookOnly(float dt);   // mouse look only, no WASD movement

    void setLookAngles(float yaw, float pitch);
    void setPosition(Vector3 p) noexcept;

    /// Skip applying GetMouseDelta on the next updateLookOnly call.
    /// Used after EnableCursor/DisableCursor transitions to avoid camera jumps.
    void skipNextMouseFrame() noexcept { m_skipMouseFrame = true; }

    float yaw()   const noexcept { return m_yaw; }
    float pitch() const noexcept { return m_pitch; }

    Camera3D&       get()       noexcept { return m_cam; }
    const Camera3D& get() const noexcept { return m_cam; }

    float moveSpeed        = 15.0f; // Скорость снижена для стабильности
    float mouseSensitivity = 0.0025f;
    bool  movementEnabled  = true;

private:
    Vector3 forward() const noexcept;

    Camera3D m_cam{};
    float m_yaw   = 0.0f;
    float m_pitch = 0.0f;
    bool  m_skipMouseFrame = false;
};

} // namespace mc
