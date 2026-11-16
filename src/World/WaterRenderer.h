
#pragma once

#include <raylib.h>
#include "Chunk.h"
#include <vector>
#include <memory>

namespace World {

class World;
class Tileset;

class WaterRenderer {
public:
    struct ReflectedSprite {
        Texture2D texture{};
        Rectangle source{};
        Rectangle destination{};
        float feetY = 0.0f;
    };

    WaterRenderer() = default;
    ~WaterRenderer();

    WaterRenderer(const WaterRenderer&) = delete;
    WaterRenderer& operator=(const WaterRenderer&) = delete;

    void Init(const World& world);
    void Unload();
    void Capture(const World& world, const Tileset& tileset, const ReflectedSprite& sprite);
    void Draw(const Camera2D& camera);

private:
    std::vector<std::unique_ptr<WaterRenderer>> m_ponds;
    void InitTiles(const std::vector<Vector2>& waterTiles);

    // Water generation
    void GenerateWaterMask(const std::vector<Vector2>& waterTiles);
    void SmoothWaterMask(std::vector<float>& mask, int width, int height);
    std::vector<unsigned char> GenerateWaterCliffs(const std::vector<float>& mask, int width, int height);
    void CreateWaterMaskTexture( const std::vector<float>& mask, const std::vector<unsigned char>& waterDepth, int width, int height);
    void CreateWaterMesh();
    void GenerateWaterDistances(const std::vector<float>& mask, int width, int height);

    // Capture
    void CreateCaptureTextures();

    // Rendering
    Camera3D CreateWaterCamera(const Camera2D& camera2D) const;
    void UpdateWaterShader();
    void DrawWaterCliffs(const Camera2D& camera2D) const;
    float ReflectionWaterDistance(float feetX, float feetY) const;
    void DrawSpriteReflection(Texture2D texture, Rectangle source, Rectangle destination, float feetY) const;
    void DrawWaterReflections(const World& world, const Tileset& tileset) const;

    // Water resources
    Model m_waterModel{};
    Texture2D m_waterCliffTexture{};
    Texture2D m_submergedCliffTexture{};
    Shader m_waterShader{};

    RenderTexture2D m_waterScene{};
    RenderTexture2D m_waterGround{};
    Texture2D m_waterMask{};

    // Shader locations
    int m_waterTimeLocation = -1;
    int m_waterSceneLocation = -1;
    int m_waterResolutionLocation = -1;
    int m_waterTopLocation = -1;
    int m_waterTileOriginLocation = -1;
    int m_waterCaptureOriginLocation = -1;
    int m_waterGroundLocation = -1;
    int m_waterMaskLocation = -1;
    int m_waterMaskBoundsLocation = -1;

    // Water geometry
    Vector2 m_waterMaskOrigin{};
    Vector2 m_waterMaskSize{};
    Vector3 m_waterCenter{};
    float m_waterTop = 0.0f;

    // Water distances
    std::vector<float> m_waterDistances;
    int m_waterMaskWidth = 0;
    int m_waterMaskHeight = 0;

    // Pond-sized capture
    Vector2 m_captureOrigin{};
    int m_captureWidth = 0;
    int m_captureHeight = 0;
};

}
