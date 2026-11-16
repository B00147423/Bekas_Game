#include "Game.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>

namespace Game
{
    Game::Game()
        : m_physicsWorld(),
        m_world(std::random_device{}(), m_physicsWorld),
        m_player(m_physicsWorld, m_world)
    {
        const Vector2 spawnPosition = m_player.GetPlayerFeetPosition();

        m_world.Generate(spawnPosition);
        //m_world.UpdateFog(spawnPosition, m_lighting.GetLightRadius());
        m_camera.RefreshViewport();

        m_tileset.Load(GameConfig::World::GrassTexturePath, GameConfig::World::FoliageTexturePath, GameConfig::World::ForestTexturePath, m_lighting.GetShadowRenderer() );
        m_world.AddObject(
            m_tileset.GetSettlementTexture(),
            {
                m_player.GetPosition().x + GameConfig::Tent::SpawnOffset.x,
                m_player.GetPosition().y + GameConfig::Tent::SpawnOffset.y
            },
            GameConfig::Tent::ColliderSize,
            GameConfig::Tent::ColliderOffset,
            GameConfig::Tent::Shadow,
            true
        );
        m_lighting.Init();
        m_hoverShader =                 LoadShader(nullptr, "src/shaders/hover.fs");
        m_fogShader =                   LoadShader(nullptr, "src/shaders/fog.fs");
        m_fogResolutionLocation =       GetShaderLocation(m_fogShader, "resolution");
        m_fogTopLeftLocation =          GetShaderLocation(m_fogShader, "worldTopLeft");
        m_fogBottomRightLocation =      GetShaderLocation(m_fogShader, "worldBottomRight");
        m_fogWorldSizeLocation =        GetShaderLocation(m_fogShader, "worldSize");
        m_fogPlayerLocation =           GetShaderLocation(m_fogShader, "playerPos");
        m_fogRadiusLocation =           GetShaderLocation(m_fogShader, "revealRadius");
        m_fogDarknessLocation =         GetShaderLocation(m_fogShader, "exploredDarkness");
        m_fogFeatherLocation =          GetShaderLocation(m_fogShader, "feather");
        m_fogTextureLocation =          GetShaderLocation(m_fogShader, "explorationTexture");

        m_waterRenderer.Init(m_world);

        m_player.Init();
    }

    Game::~Game()
    {
        if (IsShaderValid(m_hoverShader) && m_hoverShader.id != rlGetShaderIdDefault()) UnloadShader(m_hoverShader);
        if (IsShaderValid(m_fogShader) && m_fogShader.id != rlGetShaderIdDefault())UnloadShader(m_fogShader);

    }

    void Game::Update(){
        m_lighting.Update(IsKeyPressed(KEY_F3));

        m_camera.Update();

        if (IsKeyPressed(KEY_F1))
            m_showColliders = !m_showColliders;

        if (IsKeyPressed(KEY_F2))
            m_showFlowField = !m_showFlowField;

        m_world.UpdateActiveArea(m_player.GetPlayerFeetPosition());
        const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        m_player.Update(clicked, clicked ? m_camera.ScreenToWorld(GetMousePosition()) : Vector2{});

        m_world.UpdateFog(m_player.GetPlayerFeetPosition(), m_lighting.GetLightRadius());
    }

    void Game::Draw()
    {
        EnsureRenderTargets();
    
        m_lighting.RenderScene(m_world, m_tileset, m_camera);
    
        if (m_lighting.ShouldRenderShadows())
            m_lighting.RenderShadows(m_world, m_tileset, m_camera, m_player.GetPlayerFeetPosition());
    
        m_waterRenderer.Capture(m_world, m_tileset, {
            m_player.GetTexture(),
            m_player.GetFrame(),
            m_player.GetBounds(),
            m_player.GetPlayerFeetPosition().y
        });

        m_lighting.PresentScene(m_camera, m_player.GetPlayerFeetPosition());
        m_waterRenderer.Draw(m_camera.GetCamera());
        DrawSceneObjects();
    
        // RenderFog(); // Leave disabled as you had it.
        DrawFPS(10, 10);
    }

    void Game::DrawSceneObjects()
    {
        const float playerFeetY = m_player.GetPlayerFeetPosition().y;

        m_camera.Begin();
        Vector2 topLeft = m_camera.WorldToScreen({0.0f, 0.0f});

        Vector2 bottomRight = m_camera.WorldToScreen({
            GameConfig::World::ChunkWidth * GameConfig::World::TileSize,
            GameConfig::World::ChunkHeight * GameConfig::World::TileSize
        });

        BeginScissorMode(
            static_cast<int>(topLeft.x),
            static_cast<int>(topLeft.y),
            static_cast<int>(bottomRight.x - topLeft.x),
            static_cast<int>(bottomRight.y - topLeft.y)
        );
        const Vector2 mouseWorld = m_camera.ScreenToWorld(GetMousePosition());
        m_lighting.DrawLitTrees(m_world, m_tileset, m_camera, m_player.GetPlayerFeetPosition(), playerFeetY, true);
        m_world.DrawObjects(playerFeetY, true, mouseWorld, m_hoverShader);

        m_player.DrawPlayer(m_lighting.GetSpriteTint());

        m_lighting.DrawLitTrees(m_world, m_tileset, m_camera, m_player.GetPlayerFeetPosition(), playerFeetY, false);
        m_world.DrawObjects(playerFeetY, false, mouseWorld, m_hoverShader);
        EndScissorMode();
        if (m_showFlowField)
        {
                const Vector2 viewMin=m_camera.GetWorldTopLeft(),viewMax=m_camera.GetWorldBottomRight();
                m_world.DrawDebugGrid({viewMin.x,viewMin.y,viewMax.x-viewMin.x,viewMax.y-viewMin.y});
                m_world.DrawFlowField(
                m_camera.GetWorldTopLeft(),
                m_camera.GetWorldBottomRight()
            );
        }

        if (m_showColliders)
            m_physicsWorld.DrawDebugColliders();

        m_camera.End();
    }

    // void Game::RenderFog()
    // {
    //     const Texture2D exploration = m_world.GetExplorationTexture();
    //     if (!IsShaderValid(m_fogShader) || m_fogShader.id == rlGetShaderIdDefault() || !exploration.id) {
    //         DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
    //         return;
    //     }
    //     const auto setVector = [&](int location, Vector2 value) {
    //         SetShaderValue(m_fogShader, location, &value, SHADER_UNIFORM_VEC2);
    //     };
    //     const auto setFloat = [&](int location, float value) {
    //         SetShaderValue(m_fogShader, location, &value, SHADER_UNIFORM_FLOAT);
    //     };
    //     setVector(m_fogResolutionLocation, {float(GetRenderWidth()), float(GetRenderHeight())});
    //     setVector(m_fogTopLeftLocation, m_camera.GetWorldTopLeft());
    //     setVector(m_fogBottomRightLocation, m_camera.GetWorldBottomRight());
    //     setVector(m_fogWorldSizeLocation, {World::Chunk::CHUNK_WIDTH * World::World::TileSize,
    //                             World::Chunk::CHUNK_HEIGHT * World::World::TileSize});
    //     setVector(m_fogPlayerLocation, m_player.GetPlayerFeetPosition());
    //     setFloat(m_fogRadiusLocation, std::min(m_lighting.GetLightRadius(), GameConfig::Fog::DiscoveryRadius));
    //     // Daytime fog hides undiscovered terrain without dimming explored ground.
    //     setFloat(m_fogDarknessLocation, m_lighting.IsFullBright() ? 0.0f : GameConfig::Fog::ExploredDarkness);
    //     setFloat(m_fogFeatherLocation, GameConfig::Fog::BoundaryFeather);

    //     BeginShaderMode(m_fogShader);
    //     SetShaderValueTexture(m_fogShader, m_fogTextureLocation, exploration);
    //     DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
    //     EndShaderMode();
    // }

}
