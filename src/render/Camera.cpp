#include "render/Camera.hpp"
#include <raymath.h>
#include <cmath>
#include <cstdlib>

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

void FlyCamera::shake(float intensity, float duration) noexcept {
    m_shakeIntensity = intensity;
    m_shakeDuration  = (duration > 0.0f) ? duration : 0.0f;
    m_shakeTimer     = m_shakeDuration;
}

void FlyCamera::updateShake(float dt) noexcept {
    if (m_shakeTimer > 0.0f) {
        m_shakeTimer -= dt;
        if (m_shakeTimer < 0.0f) m_shakeTimer = 0.0f;

        // Плавное затухание: чем ближе таймер к 0, тем слабее тряска.
        const float fade = (m_shakeDuration > 0.0f) ? (m_shakeTimer / m_shakeDuration) : 0.0f;
        const float amp  = m_shakeIntensity * fade;

        m_shakeOffset = {
            (float)(rand() % 100 - 50) * 0.001f * amp,
            (float)(rand() % 100 - 50) * 0.001f * amp,
            (float)(rand() % 100 - 50) * 0.001f * amp,
        };
    } else {
        m_shakeOffset = { 0.0f, 0.0f, 0.0f };
    }
}

void FlyCamera::updateLookOnly(float dt) {
    updateShake(dt);

    const Vector2 delta = GetMouseDelta();
    if (m_skipMouseFrame) {
        m_skipMouseFrame = false;
    } else {
        m_yaw   += delta.x * mouseSensitivity;
        m_pitch -= delta.y * mouseSensitivity;
    }

    constexpr float MAX_PITCH = 1.553343f; // ~89°
    if (m_pitch >  MAX_PITCH) m_pitch =  MAX_PITCH;
    if (m_pitch < -MAX_PITCH) m_pitch = -MAX_PITCH;

    m_cam.target = Vector3Add(Vector3Add(m_cam.position, forward()), m_shakeOffset);
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
        m_cam.target = Vector3Add(Vector3Add(m_cam.position, fwd), m_shakeOffset);
    }
}

void FlyCamera::setPosition(Vector3 p) noexcept {
    m_cam.position = p;
    m_cam.target = Vector3Add(Vector3Add(m_cam.position, forward()), m_shakeOffset);
}

void FlyCamera::setLookAngles(float yaw, float pitch) {
    m_yaw   = yaw;
    m_pitch = pitch;
    constexpr float MAX_PITCH = 1.553343f;
    if (m_pitch >  MAX_PITCH) m_pitch =  MAX_PITCH;
    if (m_pitch < -MAX_PITCH) m_pitch = -MAX_PITCH;
    m_cam.target = Vector3Add(Vector3Add(m_cam.position, forward()), m_shakeOffset);
}

} // namespace mc
