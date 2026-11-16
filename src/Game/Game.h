#pragma once

#include <raylib.h>
#include "GameConfig.h"
#include "Lighting/LightingSystem.h"
#include <random>
#include <box2d/box2d.h>

#include "Game/Player/Player.h"
#include "World/World.h"
#include "World/WaterRenderer.h"
#include "Rendering/Tileset.h"
#include "Physics/PhysicsWorld.h"
#include "Camera.h"
namespace Game
{
    class Game
    {
    public:
        Game();
        ~Game();

        Game(const Game&) = delete;
        Game& operator=(const Game&) = delete;

        void Update();
        void Draw();
        void SetShadowSettings(float softness, float opacity, int samples) { m_lighting.SetShadowSettings(softness, opacity, samples); }
        void SetShadowBackend(bool gpu) { m_lighting.SetShadowBackend(gpu); }
        void SetShadowResolutionScale(float scale) { m_lighting.SetShadowResolutionScale(scale); }
        void EnsureRenderTargets() { m_camera.RefreshViewport(); m_lighting.EnsureRenderTargets(); }
        using RenderDiagnostics = Lighting::LightingSystem::RenderDiagnostics;
        RenderDiagnostics GetRenderDiagnostics() const { return m_lighting.GetRenderDiagnostics(); }

    private:
        //void RenderFog();
        void DrawSceneObjects();

        World::Tileset m_tileset;

        Physics::PhysicsWorld m_physicsWorld;
        World::World m_world;
        Player::Player m_player; // Destroyed before the world and physics it borrows.
        Camera m_camera;

        Shader m_hoverShader{};
        Shader m_fogShader{};
        int m_fogResolutionLocation = -1;
        int m_fogTopLeftLocation = -1;
        int m_fogBottomRightLocation = -1;
        int m_fogWorldSizeLocation = -1;
        int m_fogPlayerLocation = -1;
        int m_fogRadiusLocation = -1;
        int m_fogDarknessLocation = -1;
        int m_fogFeatherLocation = -1;
        int m_fogTextureLocation = -1;

        bool m_showColliders = false;
        bool m_showFlowField = false;

        Lighting::LightingSystem m_lighting;
        World::WaterRenderer m_waterRenderer;
    };
}
