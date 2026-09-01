#include "Camera.h"

#include <algorithm>
#include <cmath>

namespace Game
{
    Camera::Camera()
    {
        m_camera.target = { 0.0f, 0.0f };
        m_camera.offset = {
            static_cast<float>(GetScreenWidth()) * 0.5f,
            static_cast<float>(GetScreenHeight()) * 0.5f
        };
        m_camera.rotation = 0.0f;
        m_camera.zoom = 1.0f;
    }

    void Camera::Update()
    {
        m_camera.offset = {
            static_cast<float>(GetScreenWidth()) * 0.5f,
            static_cast<float>(GetScreenHeight()) * 0.5f
        };

        // Middle-mouse drag pans the world under the cursor
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
        {
            const Vector2 delta = GetMouseDelta();
            m_camera.target.x -= delta.x / m_camera.zoom;
            m_camera.target.y -= delta.y / m_camera.zoom;
        }

        // Arrow-key pan (pixels/sec). WASD left free for player/direct control later.
        Vector2 move = { 0.0f, 0.0f };
        if (IsKeyDown(KEY_RIGHT))
            move.x += 1.0f;
        if (IsKeyDown(KEY_LEFT))
            move.x -= 1.0f;
        if (IsKeyDown(KEY_DOWN))
            move.y += 1.0f;
        if (IsKeyDown(KEY_UP))
            move.y -= 1.0f;

        if (move.x != 0.0f || move.y != 0.0f)
        {
            const float len = std::sqrt(move.x * move.x + move.y * move.y);
            move.x /= len;
            move.y /= len;

            const float dt = GetFrameTime();
            m_camera.target.x += move.x * m_moveSpeed * dt / m_camera.zoom;
            m_camera.target.y += move.y * m_moveSpeed * dt / m_camera.zoom;
        }

        const float wheel = GetMouseWheelMove();
        if (wheel != 0.0f)
        {
            m_camera.zoom += wheel * m_zoomSpeed;
            m_camera.zoom = std::clamp(m_camera.zoom, m_minZoom, m_maxZoom);
        }
    }

    void Camera::SetTarget(Vector2 target)
    {
        m_camera.target = target;
    }

    Vector2 Camera::ScreenToWorld(Vector2 screenPosition) const
    {
        return GetScreenToWorld2D(screenPosition, m_camera);
    }

    Vector2 Camera::WorldToScreen(Vector2 worldPosition) const
    {
        return GetWorldToScreen2D(worldPosition, m_camera);
    }

    float Camera::WorldLengthToScreen(float worldLength) const
    {
        return worldLength * m_camera.zoom;
    }

    float Camera::GetZoom() const
    {
        return m_camera.zoom;
    }

    void Camera::Begin() const
    {
        BeginMode2D(m_camera);
    }

    void Camera::End() const
    {
        EndMode2D();
    }

    const Camera2D& Camera::GetCamera() const
    {
        return m_camera;
    }

    Vector2 Camera::GetWorldTopLeft() const
    {
        return GetScreenToWorld2D(
            {0.0f, 0.0f},
            m_camera
        );
    }

    Vector2 Camera::GetWorldBottomRight() const
    {
        return GetScreenToWorld2D(
            {
                static_cast<float>(GetScreenWidth()),
                static_cast<float>(GetScreenHeight())
            },
            m_camera
        );
    }
}
