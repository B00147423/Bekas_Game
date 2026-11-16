#pragma once
#include "GameConfig.h"
#include "GBuffer.h"
#include "ShadowBuffer.h"
#include "ShadowRenderer2D.h"
namespace World { class World; class Tileset; }
namespace Game { class Camera; }

namespace Lighting {
// Owns lighting resources and state. World still supplies shadow-casting geometry.
// Lifetime must stay inside the raylib graphics context lifetime.
class LightingSystem {
public:
    LightingSystem() = default;
    ~LightingSystem();
    LightingSystem(const LightingSystem&) = delete;
    LightingSystem& operator=(const LightingSystem&) = delete;
    void Init();
    void Update(bool toggleDaylight);
    void SetShadowSettings(float softness, float opacity, int samples);
    void SetShadowBackend(bool gpu) { m_gpuShadows = gpu; }
    void SetShadowResolutionScale(float scale);
    void EnsureRenderTargets();
    void RenderScene(const World::World& world, const World::Tileset& tileset, const Game::Camera& camera);
    void RenderShadows(const World::World& world, const World::Tileset& tileset, const Game::Camera& camera, Vector2 lightWorldPosition);
    void PresentScene(const Game::Camera& camera, Vector2 lightWorldPosition);
    void DrawLitTrees(const World::World& world, const World::Tileset& tileset, const Game::Camera& camera,
                      Vector2 lightWorldPosition, float playerFeetY, bool behindPlayer);
    bool ShouldRenderShadows() const { return !m_fullBright && m_lightIntensity > 0.001f; }
    bool IsFullBright() const { return m_fullBright; }
    Color GetSpriteTint() const { return m_fullBright ? m_daylightTint : WHITE; }
    float GetLightRadius() const { return m_lightRadius; }
    ShadowRenderer2D& GetShadowRenderer() { return m_shadowRenderer; }
    struct RenderDiagnostics {
        int width, height, shadowWidth, shadowHeight;
        ShadowRenderer2D::Stats geometry;
    };
    RenderDiagnostics GetRenderDiagnostics() const {
        return {m_gbuffer.GetWidth(), m_gbuffer.GetHeight(), m_shadowBuffer.GetWidth(),
                m_shadowBuffer.GetHeight(), m_shadowRenderer.GetStats()};
    }
private:
    Lighting::GBuffer m_gbuffer;
    Lighting::ShadowBuffer m_shadowBuffer;
    Lighting::ShadowRenderer2D m_shadowRenderer;
    float m_shadowSoftness = GameConfig::Shadows::Softness;
    float m_shadowOpacity = GameConfig::Shadows::Opacity;
    int m_shadowSamples = GameConfig::Shadows::Samples;
    bool m_gpuShadows = true;
    float m_shadowResolutionScale = 1.0f;
    Shader m_lightingShader{};
    int m_locLightPos = -1;
    int m_locResolution = -1;
    int m_locFramebufferScale = -1;
    int m_locLightRadius = -1;
    int m_locLightColor = -1;
    int m_locLightIntensity = -1;
    int m_locAmbient = -1;
    float m_dayClock = 0.0f;
    int m_locShadowTexture = -1;
    int m_locNormalTexture = -1;
    int m_locUseShadows = -1;
    int m_locUseScreenPosition = -1;
    Color m_daylightTint = GameConfig::World::DaylightTint;
    Vector3 m_lightColor = GameConfig::World::LightColor;
    float m_lightRadius = GameConfig::World::LightRadius;
    float m_lightIntensity = GameConfig::World::LightIntensity;
    float m_ambient = GameConfig::World::Ambient;
    bool m_fullBright = true;
};
}
