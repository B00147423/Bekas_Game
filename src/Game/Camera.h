#pragma once
#include "raylib.h"


namespace Game
{
    class Camera
    {
    public:
        Camera();

        void Update();
        void RefreshViewport();

        void SetTarget(Vector2 target);

        Vector2 ScreenToWorld(Vector2 screenPosition) const;
        Vector2 WorldToScreen(Vector2 worldPosition) const;
        float WorldLengthToScreen(float worldLength) const;
        float GetZoom() const;

        Vector2 GetWorldTopLeft() const;
        Vector2 GetWorldBottomRight() const;
  
        void Begin() const;
        void End() const;

        const Camera2D& GetCamera() const;

    private:
        Camera2D m_camera{};
        int m_viewportWidth = 0;
        int m_viewportHeight = 0;

        float m_moveSpeed = 500.0f;
        float m_zoomSpeed = 0.1f;

        float defaultZoom = 1.0f;
        float m_minZoom = 0.75f;
        float m_maxZoom = 2.0f;

        
    };
}
