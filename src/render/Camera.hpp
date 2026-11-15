#pragma once
#include <raylib.h>

namespace mc {

class FlyCamera {
public:
    FlyCamera();
    ~FlyCamera();

    void update(float dt);

    void setLookAngles(float yaw, float pitch);

    Camera3D&       get()       noexcept { return m_cam; }
    const Camera3D& get() const noexcept { return m_cam; }

    float moveSpeed        = 15.0f; // Скорость снижена для стабильности
    float mouseSensitivity = 0.0025f;

private:
    Camera3D m_cam{};
    float m_yaw   = 0.0f;
    float m_pitch = 0.0f;
};

} // namespace mc