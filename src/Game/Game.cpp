#include "Game.h"

namespace Game
{
    namespace
    {
        constexpr float kPlayerSpriteSize = 48.0f;
        // Physics body sits near the sprite feet (draw uses top-left)
        constexpr float kPlayerBodyOffsetX = kPlayerSpriteSize * 0.5f;
        constexpr float kPlayerBodyOffsetY = kPlayerSpriteSize * 0.65f;

        static constexpr Vector2 kPlayerSpawnPosition = { 500.0f, 300.0f };
        static constexpr Vector2 kPlayerColliderSize = { 10.0f, 10.0f };

        constexpr int kCharacterFrameWidth = 32;
        constexpr int kCharacterFrameHeight = 32;
        constexpr int kCharacterFramesPerRow = 29;

        constexpr int kIdleFirstFrame = 0;
        constexpr int kIdleLastFrame = 1;
        constexpr float kIdleFrameDuration = 0.15f;

        constexpr int kWalkFirstFrame = 2;
        constexpr int kWalkLastFrame = 4;
        constexpr float kWalkFrameDuration = 0.10f;
    }
    Game::Game()
        : m_physicsWorld(),
        m_world(std::random_device{}(), m_physicsWorld),
        m_idleAnimation(
        kIdleFirstFrame,
        kIdleLastFrame,
        kCharacterFramesPerRow,
        1,
        kIdleFrameDuration,
        Animation::Animation::AnimationType::REPEATING,
        kCharacterFrameWidth,
        kCharacterFrameHeight
    ),
    
    m_runAnimation(
        kWalkFirstFrame,
        kWalkLastFrame,
        kCharacterFramesPerRow,
        1,
        kWalkFrameDuration,
        Animation::Animation::AnimationType::REPEATING,
        kCharacterFrameWidth,
        kCharacterFrameHeight
    )
    {
        m_position = kPlayerSpawnPosition;

        Vector2 spawnPosition = {
            m_position.x + kPlayerBodyOffsetX,
            m_position.y + kPlayerBodyOffsetY
        };

        m_world.Generate(spawnPosition);
        m_camera.SetTarget(spawnPosition);

        m_idleTexture = LoadTexture("assets/human.png");
        m_runTexture = LoadTexture("assets/human.png");
        m_tileset.Load(
            "assets/grass_top_layer.png",
            "assets/Trees_Flowers.png",
            "assets/47Tiles_grass.png"
        );

        m_gbuffer.Init(GetScreenWidth(), GetScreenHeight());
        m_occlusionBuffer.Init(GetScreenWidth(), GetScreenHeight());
        m_shadowBuffer.Init(GetScreenWidth(), GetScreenHeight());
        m_blurBufferA.Init(GetScreenWidth(), GetScreenHeight());
        m_blurBufferB.Init(GetScreenWidth(), GetScreenHeight());

        m_lightingShader = LoadShader(nullptr, "src/shaders/lighting.fs");

        if (!IsShaderValid(m_lightingShader))
            TraceLog(LOG_WARNING, "Failed to load lighting.fs");

        m_shadowShader = LoadShader(nullptr, "src/shaders/shadow.fs");

        if (!IsShaderValid(m_shadowShader))
            TraceLog(LOG_WARNING, "Failed to load shadow.fs");

        m_blurShader = LoadShader(nullptr, "src/shaders/blur.fs");
        if (!IsShaderValid(m_blurShader))
            TraceLog(LOG_WARNING, "Failed to load blur.fs");

        m_blurLocResolution = GetShaderLocation(m_blurShader, "resolution");

        m_blurLocDirection = GetShaderLocation(m_blurShader, "direction");
        m_shadowLocOcclusionTexture =GetShaderLocation(m_shadowShader, "occlusionTexture");
        m_shadowLocLightPos = GetShaderLocation(m_shadowShader, "lightPos");
        m_shadowLocResolution = GetShaderLocation(m_shadowShader, "resolution");
        m_shadowLocLightRadius = GetShaderLocation(m_shadowShader, "lightRadius");

        m_locLightPos = GetShaderLocation(m_lightingShader, "lightPos");
        m_locResolution = GetShaderLocation(m_lightingShader, "resolution");
        m_locLightRadius = GetShaderLocation(m_lightingShader, "lightRadius");
        m_locLightColor = GetShaderLocation(m_lightingShader, "lightColor");
        m_locLightIntensity = GetShaderLocation(m_lightingShader, "lightIntensity");
        m_locAmbient = GetShaderLocation(m_lightingShader, "ambient");
        m_locNormalTexture = GetShaderLocation(m_lightingShader, "normalTexture");
        m_locShadowTexture = GetShaderLocation(m_lightingShader, "shadowTexture");
        m_locUseScreenPosition = GetShaderLocation(m_lightingShader, "useScreenPosition");
        
        m_locUseShadows = GetShaderLocation(m_lightingShader, "useShadows");

        CreatePlayerBody();
    }


    Game::~Game()
    {
        if (m_idleTexture.id != 0)
            UnloadTexture(m_idleTexture);

        if (m_runTexture.id != 0)
            UnloadTexture(m_runTexture);

        if (m_lightingShader.id != 0)
            UnloadShader(m_lightingShader);
            
        if (m_shadowShader.id != 0)
            UnloadShader(m_shadowShader);
        if (m_blurShader.id != 0)
            UnloadShader(m_blurShader);

        m_blurBufferA.Unload();
        m_blurBufferB.Unload();
        m_shadowBuffer.Unload();
        m_occlusionBuffer.Unload();
        m_gbuffer.Unload();
    }

    void Game::CreatePlayerBody()
    {
        Vector2 playerFeet = {
            m_position.x + kPlayerBodyOffsetX,
            m_position.y + kPlayerBodyOffsetY
        };


        m_playerBody = m_physicsWorld.CreateDynamicBox(
            playerFeet,
            kPlayerColliderSize
        );
    }

    void Game::SyncPlayerFromPhysics()
    {
        Vector2 pos = m_physicsWorld.GetBodyPosition(m_playerBody);

        m_position.x = pos.x - kPlayerBodyOffsetX;
        m_position.y = pos.y - kPlayerBodyOffsetY;
    }

    void Game::PlayerInput()
    {
        Vector2 playerFeet = {
            m_position.x + kPlayerBodyOffsetX,
            m_position.y + kPlayerBodyOffsetY
        };

        Vector2 direction = m_world.GetFlowDirection(playerFeet);

        m_playerMoving =
            direction.x != 0.0f ||
            direction.y != 0.0f;

        if (m_playerMoving)
        {
            if (direction.x < 0.0f && direction.y < 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::UpLeft);
            else if (direction.x > 0.0f && direction.y < 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::UpRight);
            else if (direction.x < 0.0f && direction.y > 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::DownLeft);
            else if (direction.x > 0.0f && direction.y > 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::DownRight);
            else if (direction.x < 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::Left);
            else if (direction.x > 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::Right);
            else if (direction.y < 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::Up);
            else if (direction.y > 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::Down);

            // Keep idle facing the same way for when movement stops
            m_idleAnimation.SetDirection(m_runAnimation.GetDirection());
            m_runAnimation.updateAnimation();
        }
        else
        {
            m_idleAnimation.updateAnimation();
        }

        Vector2 velocity = {
            direction.x * m_playerSpeed,
            direction.y * m_playerSpeed
        };

        const Vector2 before = playerFeet;

        m_physicsWorld.SetVelocity(m_playerBody, velocity);
        m_physicsWorld.Step(GetFrameTime());
        SyncPlayerFromPhysics();

        // If flow wants to move but physics barely moved, slide around the blocker
        if (m_playerMoving)
        {
            const Vector2 after = {
                m_position.x + kPlayerBodyOffsetX,
                m_position.y + kPlayerBodyOffsetY
            };
            const float dx = after.x - before.x;
            const float dy = after.y - before.y;
            const float movedSq = dx * dx + dy * dy;

            if (movedSq < 0.25f)
            {
                const Vector2 slide = m_world.GetSlideDirection(before, direction);
                if (slide.x != 0.0f || slide.y != 0.0f)
                {
                    m_physicsWorld.SetVelocity(
                        m_playerBody,
                        {
                            slide.x * m_playerSpeed,
                            slide.y * m_playerSpeed
                        }
                    );
                    m_physicsWorld.Step(GetFrameTime());
                    SyncPlayerFromPhysics();
                }
            }
        }
    }

    Vector2 Game::GetLightWorldPosition() const
    {
        return {
            m_position.x + kPlayerBodyOffsetX,
            m_position.y + kPlayerBodyOffsetY
        };
    }

    void Game::Update()
    {
        m_camera.Update();

        if (IsKeyPressed(KEY_F1))
            m_showColliders = !m_showColliders;

        if (IsKeyPressed(KEY_F2))
            m_showFlowField = !m_showFlowField;

        if (IsKeyPressed(KEY_F3))
             m_fullBright = !m_fullBright;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            Vector2 mouseWorld =
                m_camera.ScreenToWorld(GetMousePosition());

            m_world.SetFlowFieldTarget(mouseWorld);
        }

        PlayerInput();
    }

    void Game::DrawPlayer()
    {
        if (m_playerMoving)
        {
            DrawTexturePro(
                m_runTexture,
                m_runAnimation.GetFrame(),
                {
                    m_position.x,
                    m_position.y,
                    kPlayerSpriteSize,
                    kPlayerSpriteSize
                },
                { 0.0f, 0.0f },
                0.0f,
                WHITE
            );
        }
        else
        {
            DrawTexturePro(
                m_idleTexture,
                m_idleAnimation.GetFrame(),
                {
                    m_position.x,
                    m_position.y,
                    kPlayerSpriteSize,
                    kPlayerSpriteSize
                },
                { 0.0f, 0.0f },
                0.0f,
                WHITE
            );
        }
    }

    void Game::RenderBlur(){
        const Texture2D shadow = m_shadowBuffer.GetTexture();

        Vector2 resolution = {
            static_cast<float>(shadow.width),
            static_cast<float>(shadow.height)
        };

        SetShaderValue(m_blurShader, m_blurLocResolution, &resolution, SHADER_UNIFORM_VEC2);

        // PASS 1: HORIZONTAL
        Vector2 horizontal = { 1.0f, 0.0f };
        SetShaderValue(
            m_blurShader,
            m_blurLocDirection,
            &horizontal,
            SHADER_UNIFORM_VEC2
        );

        m_blurBufferA.Begin();
        BeginShaderMode(m_blurShader);

        DrawTextureRec(
            shadow,
            {
                0.0f,
                0.0f,
                static_cast<float>(shadow.width),
                -static_cast<float>(shadow.height)
            },
            { 0.0f, 0.0f },
            WHITE
        );

        EndShaderMode();
        m_blurBufferA.End();

        // PASS 2: VERTICAL
        const Texture2D horizontalBlur = m_blurBufferA.GetTexture();
        Vector2 vertical = { 0.0f, 1.0f };

        SetShaderValue(
            m_blurShader,
            m_blurLocDirection,
            &vertical,
            SHADER_UNIFORM_VEC2
        );

        m_blurBufferB.Begin();
        BeginShaderMode(m_blurShader);
        DrawTextureRec(
            horizontalBlur,
            {
                0.0f,
                0.0f,
                static_cast<float>(horizontalBlur.width),
                -static_cast<float>(horizontalBlur.height)
            },
            { 0.0f, 0.0f },
            WHITE
        );

        EndShaderMode();

        m_blurBufferB.End();
    }
    void Game::RenderShadows()
    {
        const Texture2D mask = m_occlusionBuffer.GetTexture();

        // Shaders work in screen space; keep m_lightRadius as world units.
        const Vector2 lightPos = m_camera.WorldToScreen(GetLightWorldPosition());
        const float screenLightRadius = m_camera.WorldLengthToScreen(m_lightRadius);

        Vector2 resolution = {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        };

        SetShaderValue(m_shadowShader, m_shadowLocLightPos, &lightPos, SHADER_UNIFORM_VEC2);
        SetShaderValue(m_shadowShader, m_shadowLocResolution, &resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(m_shadowShader, m_shadowLocLightRadius, &screenLightRadius, SHADER_UNIFORM_FLOAT);

        m_shadowBuffer.Begin();

        BeginShaderMode(m_shadowShader);

        if (m_shadowLocOcclusionTexture >= 0){
            SetShaderValueTexture(
                m_shadowShader,
                m_shadowLocOcclusionTexture,
                mask
            );
        }

        DrawTextureRec(
            mask,
            {
                0.0f,
                0.0f,
                static_cast<float>(mask.width),
                -static_cast<float>(mask.height)
            },
            { 0.0f, 0.0f },
            WHITE
        );

        EndShaderMode();

        m_shadowBuffer.End();
    }

    void Game::RenderOcclusion(){
        m_occlusionBuffer.Begin();
        m_camera.Begin();
        // WE WILL DRAW TREE BLOCKERS WHITE HERE
        m_world.DrawOccluders();
        m_camera.End();
        m_occlusionBuffer.End();
    }

    void Game::RenderScene(){
        m_gbuffer.Begin();
        m_camera.Begin();
        DrawWorld();
        m_camera.End();
        m_gbuffer.End();
    }

    void Game::PresentScene()
    {
        const Texture2D albedo = m_gbuffer.GetAlbedoTexture();
        const Texture2D normal = m_gbuffer.GetNormalTexture();
        const Texture2D blurredShadow = m_blurBufferB.GetTexture();


        if (m_fullBright)
        {
            DrawTextureRec(
                albedo,
                {
                    0.0f,
                    0.0f,
                    static_cast<float>(albedo.width),
                    -static_cast<float>(albedo.height)
                },
                { 0.0f, 0.0f },
                WHITE
            );

            return;
        }

        const Vector2 lightPos = m_camera.WorldToScreen(GetLightWorldPosition());
        const float screenLightRadius = m_camera.WorldLengthToScreen(m_lightRadius);


        Vector2 resolution = {
            static_cast<float>(albedo.width),
            static_cast<float>(albedo.height)
        };


        SetShaderValue(m_lightingShader, m_locLightPos, &lightPos, SHADER_UNIFORM_VEC2
        );

        SetShaderValue(m_lightingShader, m_locResolution, &resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue( m_lightingShader, m_locLightRadius, &screenLightRadius, SHADER_UNIFORM_FLOAT);
        SetShaderValue( m_lightingShader, m_locLightColor, &m_lightColor,SHADER_UNIFORM_VEC3);
        SetShaderValue(m_lightingShader, m_locLightIntensity, &m_lightIntensity, SHADER_UNIFORM_FLOAT
        );

        SetShaderValue(m_lightingShader,  m_locAmbient,   &m_ambient,  SHADER_UNIFORM_FLOAT );
        BeginShaderMode(m_lightingShader);
        int useShadows = 1;

        SetShaderValue(m_lightingShader, m_locUseShadows, &useShadows, SHADER_UNIFORM_INT );
        if (m_locNormalTexture >= 0){
            SetShaderValueTexture(
                m_lightingShader,
                m_locNormalTexture,
                normal
            );
        }

        if (m_locShadowTexture >= 0)
        {
            SetShaderValueTexture(
                m_lightingShader,
                m_locShadowTexture,
                blurredShadow
            );
        }



        int useScreenPosition = 0;

        SetShaderValue(
            m_lightingShader,
            m_locUseScreenPosition,
            &useScreenPosition,
            SHADER_UNIFORM_INT
        );

        DrawTextureRec(
            albedo,
            {
                0.0f,
                0.0f,
                static_cast<float>(albedo.width),
                -static_cast<float>(albedo.height)
            },
            { 0.0f, 0.0f },
            WHITE
        );

        EndShaderMode();
    }


    void Game::DrawLitTrees(float playerFeetY, bool behindPlayer)
    {

        if (m_fullBright)
        {
            m_world.DrawTrees(
                m_tileset,
                playerFeetY,
                behindPlayer
            );

            return;
        }
        const Vector2 lightPos = m_camera.WorldToScreen(GetLightWorldPosition());
        const float screenLightRadius = m_camera.WorldLengthToScreen(m_lightRadius);

        Vector2 resolution = {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
        };

        int useShadows = 0;

        SetShaderValue(m_lightingShader, m_locLightPos, &lightPos, SHADER_UNIFORM_VEC2);

        SetShaderValue(m_lightingShader,m_locResolution, &resolution, SHADER_UNIFORM_VEC2);

        SetShaderValue(m_lightingShader, m_locLightRadius, &screenLightRadius, SHADER_UNIFORM_FLOAT);

        SetShaderValue(m_lightingShader, m_locLightColor, &m_lightColor, SHADER_UNIFORM_VEC3);

        SetShaderValue(m_lightingShader, m_locLightIntensity, &m_lightIntensity, SHADER_UNIFORM_FLOAT);

        SetShaderValue(m_lightingShader, m_locAmbient,  &m_ambient,SHADER_UNIFORM_FLOAT);

        SetShaderValue(m_lightingShader, m_locUseShadows, &useShadows, SHADER_UNIFORM_INT);
        int useScreenPosition = 1;

        SetShaderValue(m_lightingShader, m_locUseScreenPosition,  &useScreenPosition, SHADER_UNIFORM_INT );
        BeginShaderMode(m_lightingShader);

        m_world.DrawTrees(
            m_tileset,
            playerFeetY,
            behindPlayer
        );

        EndShaderMode();
    }

    void Game::DrawWorld()
    {
        m_world.DrawGround(m_tileset);
    }

    void Game::Draw()
    {
        // 1. Ground -> G-buffer
        RenderScene();

        // 2. Trees -> occlusion mask
        RenderOcclusion();

        // 3. Occlusion mask -> shadow buffer
        RenderShadows();
        //blur the shadow buffer to create a soft shadow effect
        RenderBlur(); 

        // 4. Display shadow buffer for debugging
        PresentScene();

        const float playerFeetY = m_position.y + kPlayerBodyOffsetY;

        m_camera.Begin();

        DrawLitTrees(playerFeetY, true);
        DrawPlayer();
        DrawLitTrees(playerFeetY, false);

        if (m_showFlowField)
        {
            m_world.DrawDebugGrid();
            m_world.DrawFlowField(
                m_camera.GetWorldTopLeft(),
                m_camera.GetWorldBottomRight()
            );
        }

        if (m_showColliders)
            m_physicsWorld.DrawDebugColliders();

        m_camera.End();

        DrawFPS(10, 10);
    }
}
