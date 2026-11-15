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

void FlyCamera::update(float dt) {
    // ─────────────── Look (мышь) ───────────────
    const Vector2 delta = GetMouseDelta();
    m_yaw   += delta.x * mouseSensitivity;
    m_pitch -= delta.y * mouseSensitivity;

    constexpr float MAX_PITCH = 1.553343f; // ~89°
    if (m_pitch >  MAX_PITCH) m_pitch =  MAX_PITCH;
    if (m_pitch < -MAX_PITCH) m_pitch = -MAX_PITCH;

    const Vector3 forward = {
        std::cos(m_pitch) * std::sin(m_yaw),
        std::sin(m_pitch),
       -std::cos(m_pitch) * std::cos(m_yaw)
    };
    const Vector3 right = { std::cos(m_yaw), 0.0f, std::sin(m_yaw) };

    // ─────────────── Move (клавиши) ───────────────
    Vector3 move{};
    if (IsKeyDown(KEY_W))              move = Vector3Add(move, forward);
    if (IsKeyDown(KEY_S))              move = Vector3Subtract(move, forward);
    if (IsKeyDown(KEY_D))              move = Vector3Add(move, right);
    if (IsKeyDown(KEY_A))              move = Vector3Subtract(move, right);
    if (IsKeyDown(KEY_SPACE))          move.y += 1.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT))     move.y -= 1.0f;

    if (Vector3LengthSqr(move) > 0.0f) {
        move = Vector3Normalize(move);
        float speed = moveSpeed;
        if (IsKeyDown(KEY_LEFT_CONTROL)) speed *= 4.0f;
        m_cam.position = Vector3Add(m_cam.position, Vector3Scale(move, speed * dt));
    }

    m_cam.target = Vector3Add(m_cam.position, forward);
}

void FlyCamera::setLookAngles(float yaw, float pitch) {
    m_yaw   = yaw;
    m_pitch = pitch;
    constexpr float MAX_PITCH = 1.553343f;
    if (m_pitch >  MAX_PITCH) m_pitch =  MAX_PITCH;
    if (m_pitch < -MAX_PITCH) m_pitch = -MAX_PITCH;

    const Vector3 forward = {
        std::cos(m_pitch) * std::sin(m_yaw),
        std::sin(m_pitch),
       -std::cos(m_pitch) * std::cos(m_yaw)
    };
    m_cam.target = Vector3Add(m_cam.position, forward);
}

} // namespace mc