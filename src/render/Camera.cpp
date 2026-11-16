#include "render/Camera.hpp"
#include <raymath.h>
#include <cmath>

namespace mc {

FlyCamera::FlyCamera() {
    m_cam.position   = { 0.0f, 40.0f, 40.0f };
    m_cam.target     = { 0.0f, 40.0f,  0.0f };
    m_cam.up         = { 0.0f,  1.0f,  0.0f };
    m_cam.fovy       = 75.0f;
    m_cam.projection = CAMERA_PERSPECTIVE;
    DisableCursor();
}

FlyCamera::~FlyCamera() {
    EnableCursor();
}

Vector3 FlyCamera::forward() const noexcept {
    return {
        std::cos(m_pitch) * std::sin(m_yaw),
        std::sin(m_pitch),
       -std::cos(m_pitch) * std::cos(m_yaw)
    };
}

void FlyCamera::updateLookOnly(float dt) {
    (void)dt;  // look is driven by GetMouseDelta, not by dt

    const Vector2 delta = GetMouseDelta();
    m_yaw   += delta.x * mouseSensitivity;
    m_pitch -= delta.y * mouseSensitivity;

    constexpr float MAX_PITCH = 1.553343f; // ~89°
    if (m_pitch >  MAX_PITCH) m_pitch =  MAX_PITCH;
    if (m_pitch < -MAX_PITCH) m_pitch = -MAX_PITCH;

    m_cam.target = Vector3Add(m_cam.position, forward());
}

void FlyCamera::update(float dt) {
    updateLookOnly(dt);

    if (!movementEnabled) return;

    const Vector3 fwd = forward();
    const Vector3 right = { std::cos(m_yaw), 0.0f, std::sin(m_yaw) };

    Vector3 move{};
    if (IsKeyDown(KEY_W))              move = Vector3Add(move, fwd);
    if (IsKeyDown(KEY_S))              move = Vector3Subtract(move, fwd);
    if (IsKeyDown(KEY_D))              move = Vector3Add(move, right);
    if (IsKeyDown(KEY_A))              move = Vector3Subtract(move, right);
    if (IsKeyDown(KEY_SPACE))          move.y += 1.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT))     move.y -= 1.0f;

    if (Vector3LengthSqr(move) > 0.0f) {
        move = Vector3Normalize(move);
        float speed = moveSpeed;
        if (IsKeyDown(KEY_LEFT_CONTROL)) speed *= 4.0f;
        m_cam.position = Vector3Add(m_cam.position, Vector3Scale(move, speed * dt));
        m_cam.target = Vector3Add(m_cam.position, fwd);
    }
}

void FlyCamera::setPosition(Vector3 p) noexcept {
    m_cam.position = p;
    m_cam.target = Vector3Add(m_cam.position, forward());
}

void FlyCamera::setLookAngles(float yaw, float pitch) {
    m_yaw   = yaw;
    m_pitch = pitch;
    constexpr float MAX_PITCH = 1.553343f;
    if (m_pitch >  MAX_PITCH) m_pitch =  MAX_PITCH;
    if (m_pitch < -MAX_PITCH) m_pitch = -MAX_PITCH;
    m_cam.target = Vector3Add(m_cam.position, forward());
}

} // namespace mc
