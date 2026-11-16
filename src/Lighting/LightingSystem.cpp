#include "LightingSystem.h"
#include "ShadowScheduler.h"
#include "World/World.h"
#include "Game/Camera.h"
#include <algorithm>
#include <cmath>

namespace Lighting {
LightingSystem::~LightingSystem() {
    if (m_lightingShader.id != 0) UnloadShader(m_lightingShader);
    m_shadowBuffer.Unload();
    m_gbuffer.Unload();
}
void LightingSystem::Init() {
    m_gbuffer.Init(GetRenderWidth(), GetRenderHeight());
    m_shadowBuffer.Init(GetRenderWidth(), GetRenderHeight());

    if (m_lightingShader.id != 0) UnloadShader(m_lightingShader);

    m_lightingShader = LoadShader(nullptr, "src/shaders/lighting.fs");
        if (!IsShaderValid(m_lightingShader)){
            TraceLog(LOG_WARNING, "Failed to load lighting.fs");
        }

        m_locLightPos =                 GetShaderLocation(m_lightingShader, "lightPos");
        m_locFramebufferScale =         GetShaderLocation(m_lightingShader,"framebufferScale");
        m_locResolution =               GetShaderLocation(m_lightingShader, "resolution");
        m_locLightRadius =              GetShaderLocation(m_lightingShader, "lightRadius");
        m_locLightColor =               GetShaderLocation(m_lightingShader, "lightColor");
        m_locLightIntensity =           GetShaderLocation(m_lightingShader, "lightIntensity");
        m_locAmbient =                  GetShaderLocation(m_lightingShader, "ambient");
        m_locNormalTexture =            GetShaderLocation(m_lightingShader, "normalTexture");
        m_locShadowTexture =            GetShaderLocation(m_lightingShader, "shadowTexture");
        m_locUseScreenPosition =        GetShaderLocation(m_lightingShader, "useScreenPosition");
        m_locUseShadows =               GetShaderLocation(m_lightingShader, "useShadows");

}

void LightingSystem::Update(bool toggleDaylight) {
        constexpr float cycleSeconds = 720.0f;

        // F3 switches between full-bright daytime and lit nighttime.
        if (toggleDaylight)
        {
            m_fullBright = !m_fullBright;
        }

        // Hold the selected time until F3 is pressed again.
        m_dayClock = m_fullBright ? 0.0f : cycleSeconds * 0.5f;

        float daylight = 0.5f + 0.5f * std::cos(2.0f * PI * m_dayClock / cycleSeconds);

        m_ambient = 0.40f + (0.95f - 0.30f) * daylight;
        m_lightIntensity = GameConfig::World::LightIntensity * (1.0f - daylight);

}

void LightingSystem::SetShadowSettings(float softness, float opacity, int samples)
{
    m_shadowSoftness = std::isfinite(softness) ? std::max(0.0f, softness) : 0.0f;
    m_shadowOpacity = std::isfinite(opacity) ? std::clamp(opacity, 0.0f, 1.0f) : 1.0f;
    m_shadowSamples = samples <= 4 ? 4 : samples <= 8 ? 8 : 16;
}

void LightingSystem::SetShadowResolutionScale(float scale) {
    m_shadowResolutionScale=std::isfinite(scale)?std::clamp(scale,0.25f,1.0f):1.0f;
}

void LightingSystem::EnsureRenderTargets() {
    const int width=GetRenderWidth(),height=GetRenderHeight();
    if(width<=0 || height<=0 || GetScreenWidth()<=0 || GetScreenHeight()<=0) return;
    m_gbuffer.Init(width,height);
    m_shadowBuffer.Resize(std::max(1,(int)std::ceil(width*m_shadowResolutionScale)),
        std::max(1,(int)std::ceil(height*m_shadowResolutionScale)),GetScreenWidth(),GetScreenHeight());
    const Vector2 framebufferScale{(float)width/GetScreenWidth(),(float)height/GetScreenHeight()};
    SetShaderValue(m_lightingShader,m_locFramebufferScale,&framebufferScale,SHADER_UNIFORM_VEC2);
}

void LightingSystem::RenderShadows(const World::World& world, const World::Tileset& tileset, const Game::Camera& camera, Vector2 lightWorldPosition)
{

    const float softness = std::max(0.0f, m_shadowSoftness);
    const float opacity = std::clamp(m_shadowOpacity, 0.0f, 1.0f);
    const Lighting::ShadowLight2D lights[]={{0,lightWorldPosition,m_lightRadius,softness,opacity,1,m_shadowSamples}};
    const auto work = Lighting::ShadowScheduler::Select(lights,camera.GetCamera(),
        GetScreenWidth(),GetScreenHeight(),m_shadowResolutionScale,{});
    if(work.empty()) {
        m_shadowBuffer.Begin();m_shadowBuffer.End();return;
    }
    const Rectangle bounds=work.front().screenBounds;
    const int left=(int)bounds.x,top=(int)bounds.y;
    const int right=left+(int)bounds.width,bottom=top+(int)bounds.height;

    const Vector2 viewMin = camera.GetWorldTopLeft();
    const Vector2 viewMax = camera.GetWorldBottomRight();
    const Rectangle view{viewMin.x, viewMin.y, viewMax.x-viewMin.x, viewMax.y-viewMin.y};
    const int count = work.front().samples;
    if (!m_shadowBuffer.PrepareSamples(count)) {
        m_shadowBuffer.Begin();
        m_shadowBuffer.BeginScissor({(float)left,(float)top,(float)(right-left),(float)(bottom-top)});
        camera.Begin();
        // Opaque grayscale also avoids repeated alpha darkening on overlapping edges.
        unsigned char visibility = (unsigned char)(255 * (1-opacity));
        world.DrawAutoShadows(tileset, m_shadowRenderer, lightWorldPosition,
            m_lightRadius * 2, view, m_lightRadius, {visibility, visibility, visibility, 255});
        camera.End();
        m_shadowBuffer.EndScissor();
        m_shadowBuffer.End();
        return;
    }
    const bool batched=m_gpuShadows && m_shadowRenderer.BeginCollection();
    if(batched) {
        world.DrawAutoShadows(tileset,m_shadowRenderer,lightWorldPosition,
            m_lightRadius*2,view,m_lightRadius+2*softness);
        m_shadowRenderer.EndCollection();
    }
    for (int sample = 0; sample < count; ++sample) {
        const Vector2 offset = Lighting::ShadowBuffer::SampleOffset(sample, count, softness);
        const Vector2 sampleLight{lightWorldPosition.x + offset.x, lightWorldPosition.y + offset.y};
        m_shadowBuffer.BeginSample(sample);
        m_shadowBuffer.BeginScissor({(float)left,(float)top,(float)(right-left),(float)(bottom-top)});
        camera.Begin();
        if(batched) m_shadowRenderer.DrawBatch(sampleLight,m_lightRadius*2,
            Lighting::ShadowBuffer::SampleColor(sample));
        else world.DrawAutoShadows(tileset, m_shadowRenderer, sampleLight,
            m_lightRadius * 2, view, m_lightRadius + softness,
            Lighting::ShadowBuffer::SampleColor(sample));
        camera.End();
        m_shadowBuffer.EndScissor();
        m_shadowBuffer.EndSample();
    }
    m_shadowBuffer.Resolve(count, opacity,
        {(float)left, (float)top, (float)(right-left), (float)(bottom-top)});
}

void LightingSystem::PresentScene(const Game::Camera& camera, Vector2 lightWorldPosition)
    {
        const Texture2D albedo = m_gbuffer.GetAlbedoTexture();
        const Texture2D normal = m_gbuffer.GetNormalTexture();
        const Texture2D shadowMask = m_shadowBuffer.GetTexture();


        if (m_fullBright)
        {
            DrawTexturePro(
                albedo,
                {
                    0.0f,
                    0.0f,
                    static_cast<float>(albedo.width),
                    -static_cast<float>(albedo.height)
                },
                {0,0,(float)GetScreenWidth(),(float)GetScreenHeight()},
                {0,0},0,
                m_daylightTint
            );

            return;
        }

        const Vector2 lightPos = camera.WorldToScreen(lightWorldPosition);
        const float screenLightRadius = camera.WorldLengthToScreen(m_lightRadius);


        Vector2 resolution = {
            static_cast<float>(GetScreenWidth()),
            static_cast<float>(GetScreenHeight())
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
                shadowMask
            );
        }



        int useScreenPosition = 0;

        SetShaderValue(
            m_lightingShader,
            m_locUseScreenPosition,
            &useScreenPosition,
            SHADER_UNIFORM_INT
        );

        DrawTexturePro(
            albedo,
            {
                0.0f,
                0.0f,
                static_cast<float>(albedo.width),
                -static_cast<float>(albedo.height)
            },
            {0,0,(float)GetScreenWidth(),(float)GetScreenHeight()},
            {0,0},0,
            WHITE
        );

        EndShaderMode();
    }

void LightingSystem::DrawLitTrees(const World::World& world, const World::Tileset& tileset, const Game::Camera& camera, Vector2 lightWorldPosition, float playerFeetY, bool behindPlayer)
    {
        const Vector2 viewMin=camera.GetWorldTopLeft(), viewMax=camera.GetWorldBottomRight();
        const Rectangle visibleBounds{viewMin.x,viewMin.y,viewMax.x-viewMin.x,viewMax.y-viewMin.y};


        if (m_fullBright)
        {
            world.DrawTrees(
                tileset,
                playerFeetY,
                behindPlayer,
                m_daylightTint, visibleBounds
            );

            return;
        }
        const Vector2 lightPos = camera.WorldToScreen(lightWorldPosition);
        const float screenLightRadius = camera.WorldLengthToScreen(m_lightRadius);

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

        world.DrawTrees(
            tileset,
            playerFeetY,
            behindPlayer, WHITE, visibleBounds
        );

        EndShaderMode();
    }

void LightingSystem::RenderScene(const World::World& world, const World::Tileset& tileset, const Game::Camera& camera){
        m_gbuffer.Begin();
        camera.Begin();
        const Vector2 topLeft = camera.GetWorldTopLeft();
        const Vector2 bottomRight = camera.GetWorldBottomRight();
        world.DrawGround(tileset, {topLeft.x, topLeft.y,
            bottomRight.x - topLeft.x, bottomRight.y - topLeft.y});
        camera.End();
        m_gbuffer.End();
    }

}
