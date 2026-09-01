#pragma once

#include <raylib.h>
#include <random>
#include <box2d/box2d.h>

#include "../Animation/Animation.h"
#include "../Lighting/GBuffer.h"
#include "../World/World.h"
#include "../World/Tileset.h"
#include "../Lighting/OcclusionBuffer.h"
#include "../Lighting/ShadowBuffer.h"
#include "../Physics/PhysicsWorld.h"
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

    private:
        void CreatePlayerBody();
        void SyncPlayerFromPhysics();
        void PlayerInput();
        void RenderScene();
        void DrawWorld();
        void DrawPlayer();
        void PresentScene();
        void RenderOcclusion();
        void RenderShadows();
        void RenderBlur();
        void DrawLitTrees(float playerFeetY, bool behindPlayer);

        // World-space light origin (player feet). Shaders get screen-space copies.
        Vector2 GetLightWorldPosition() const;

        World::Tileset m_tileset;

        Animation::Animation m_idleAnimation;
        Animation::Animation m_runAnimation;

        Physics::PhysicsWorld m_physicsWorld;
        World::World m_world;
        Camera m_camera;

        Texture2D m_idleTexture{};
        Texture2D m_runTexture{};

        Lighting::GBuffer m_gbuffer;
        Lighting::OcclusionBuffer m_occlusionBuffer;
        Lighting::ShadowBuffer m_shadowBuffer;
        Lighting::ShadowBuffer m_blurBufferA;
        Lighting::ShadowBuffer m_blurBufferB;
        
        Shader m_lightingShader{};
        Shader m_shadowShader{};
        Shader m_blurShader{};

        int m_locLightPos = -1;
        int m_locResolution = -1;
        int m_locLightRadius = -1;
        int m_locLightColor = -1;
        int m_locLightIntensity = -1;
        int m_locAmbient = -1;
        int m_locOcclusionTexture = -1;
        int m_locShadowTexture = -1;
        int m_shadowLocOcclusionTexture = -1;
        int m_shadowLocLightPos = -1;
        int m_shadowLocResolution = -1;
        int m_shadowLocLightRadius = -1;
        int m_blurLocResolution = -1;
        int m_blurLocDirection = -1;
        int m_locNormalTexture = -1;
        int m_locUseShadows = -1;
        int m_locUseScreenPosition = -1;

        Vector3 m_lightColor = { 1.0f, 0.95f, 0.85f };
        bool m_playerMoving = false;
        float m_lightRadius = 500.0f;
        float m_lightIntensity = 1.5f;
        float m_ambient = 0.08f;

        b2BodyId m_playerBody{};

        Vector2 m_position{};
        float m_playerSpeed = 150.0f;
        bool m_showColliders = false;
        bool m_showFlowField = false;


        bool m_fullBright = false;
    };
}
