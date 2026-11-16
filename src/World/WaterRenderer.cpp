#include "WaterRenderer.h"
#include "World.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <queue>

namespace World {
WaterRenderer::~WaterRenderer() { Unload(); }

void WaterRenderer::Unload() {
        m_ponds.clear();
        if (m_waterModel.meshCount > 0)UnloadModel(m_waterModel);
        if (m_waterCliffTexture.id) UnloadTexture(m_waterCliffTexture);
        if (m_submergedCliffTexture.id) UnloadTexture(m_submergedCliffTexture);

        if (m_waterShader.id != 0 && m_waterShader.id != rlGetShaderIdDefault())UnloadShader(m_waterShader);

        if (m_waterScene.id != 0) UnloadRenderTexture(m_waterScene);
        if (m_waterGround.id != 0) UnloadRenderTexture(m_waterGround);
        if (m_waterMask.id) UnloadTexture(m_waterMask);
        m_waterModel = {};
        m_waterCliffTexture = {};
        m_submergedCliffTexture = {};
        m_waterShader = {};
        m_waterScene = {};
        m_waterGround = {};
        m_waterMask = {};
        m_waterDistances.clear();
        m_waterMaskWidth = 0;
        m_waterMaskHeight = 0;
        m_captureOrigin = {};
        m_captureWidth = 0;
        m_captureHeight = 0;
        m_waterTop = 0.0f;
}

void WaterRenderer::Init(const World& world) {
    Unload();
    const auto waterTiles=world.GetWaterTiles();
    // Split spatially separate water into independent captures and masks.
    std::set<std::pair<int,int>> remaining;
    for(auto tile:waterTiles) remaining.emplace(int(tile.x/World::TileSize),int(tile.y/World::TileSize));
    std::vector<std::vector<Vector2>> regions;
    while(!remaining.empty()) {
        auto first=*remaining.begin();remaining.erase(remaining.begin());
        std::queue<std::pair<int,int>> pending;pending.push(first);
        auto& region=regions.emplace_back();
        while(!pending.empty()) {
            auto [x,y]=pending.front();pending.pop();
            region.push_back({(x+0.5f)*World::TileSize,(y+0.5f)*World::TileSize});
            for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
                auto cell=std::pair{x+dx,y+dy};
                if(remaining.erase(cell)) pending.push(cell);
            }
        }
    }
    if(regions.size()==1) InitTiles(regions.front());
    else for(const auto& region:regions) {
        auto pond=std::make_unique<WaterRenderer>();pond->InitTiles(region);m_ponds.push_back(std::move(pond));
    }
}

void WaterRenderer::InitTiles(const std::vector<Vector2>& waterTiles) {
    m_waterShader = LoadShader(
        GameConfig::Water::SurfaceVertexShaderPath,
        GameConfig::Water::SurfaceFragmentShaderPath
    );

    if (!IsShaderValid(m_waterShader) || m_waterShader.id == rlGetShaderIdDefault()) {
        TraceLog(LOG_ERROR, "Water shader failed to load!");
        return;
    }

    if (waterTiles.empty()) return;

    GenerateWaterMask(waterTiles);
    CreateCaptureTextures();

    m_waterGroundLocation = GetShaderLocation(m_waterShader, "groundTexture");
    m_waterSceneLocation = GetShaderLocation(m_waterShader, "sceneTexture");
    m_waterResolutionLocation = GetShaderLocation(m_waterShader, "resolution");
    m_waterTopLocation = GetShaderLocation(m_waterShader, "waterTop");
    m_waterTimeLocation = GetShaderLocation(m_waterShader, "time");
    m_waterTileOriginLocation = GetShaderLocation(m_waterShader, "tileOrigin");
    m_waterCaptureOriginLocation = GetShaderLocation(m_waterShader, "captureOrigin");
}

void WaterRenderer::Capture(const World& world, const Tileset& tileset, const ReflectedSprite& sprite) {
        if(!m_ponds.empty()) {
            for(auto& pond:m_ponds) pond->Capture(world,tileset,sprite);
            return;
        }
        if (!m_waterGround.id || !m_waterScene.id) return;

        // Capture in fixed world coordinates, including scenery outside the
        // viewport. Camera movement never changes the reflection source.
        Camera2D captureCamera{};
        captureCamera.target = m_captureOrigin;
        captureCamera.zoom = 1.0f;
        BeginTextureMode(m_waterGround);
        ClearBackground({25, 60, 65, 255});
        BeginMode2D(captureCamera);
        world.DrawGround(tileset, {m_captureOrigin.x, m_captureOrigin.y,
            float(m_captureWidth), float(m_captureHeight)});

        if (m_submergedCliffTexture.id)
        {
            DrawTextureV(m_submergedCliffTexture, m_waterMaskOrigin, WHITE);
        }
        EndMode2D();
        EndTextureMode();

        BeginTextureMode(m_waterScene);
        ClearBackground(BLANK);
        BeginMode2D(captureCamera);
        DrawWaterReflections(world, tileset);
        DrawSpriteReflection(sprite.texture, sprite.source, sprite.destination, sprite.feetY);
        EndMode2D();
        EndTextureMode();
}

void WaterRenderer::Draw(const Camera2D& camera2D) {
    if(!m_ponds.empty()) {
        for(auto& pond:m_ponds) pond->Draw(camera2D);
        return;
    }
    if (!IsShaderValid(m_waterShader) ||
        m_waterShader.id == rlGetShaderIdDefault() ||
        m_waterMask.id == 0 ||
        m_waterModel.meshCount == 0) {
        return;
    }

    const Camera3D waterCamera = CreateWaterCamera(camera2D);

    UpdateWaterShader();

    BeginMode3D(waterCamera);
    DrawModel(m_waterModel, m_waterCenter, 1.0f, WHITE);
    EndMode3D();

    DrawWaterCliffs(camera2D);
}

Camera3D WaterRenderer::CreateWaterCamera(const Camera2D& camera2D) const {
    const Vector2 center = GetScreenToWorld2D(
        {
            GetScreenWidth() * 0.5f,
            GetScreenHeight() * 0.5f
        },
        camera2D
    );

    Camera3D waterCamera{};
    waterCamera.position = {center.x, GameConfig::Water::CameraHeight, center.y};
    waterCamera.target = {center.x, 0.0f, center.y};
    waterCamera.up = {0.0f, 0.0f, -1.0f};
    waterCamera.fovy = GetScreenHeight() / camera2D.zoom;
    waterCamera.projection = CAMERA_ORTHOGRAPHIC;

    return waterCamera;
}
    void WaterRenderer::CreateCaptureTextures() {
        const float padding = float(GameConfig::Water::CapturePadding);

        m_captureOrigin = {
            m_waterMaskOrigin.x - padding,
            m_waterMaskOrigin.y - padding
        };

        m_captureWidth = static_cast<int>(std::ceil(m_waterMaskSize.x + padding * 2.0f));
        m_captureHeight = static_cast<int>(std::ceil(m_waterMaskSize.y + padding * 2.0f));

        m_waterScene = LoadRenderTexture(m_captureWidth, m_captureHeight);
        m_waterGround = LoadRenderTexture(m_captureWidth, m_captureHeight);

        SetTextureFilter(m_waterScene.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(m_waterGround.texture, TEXTURE_FILTER_BILINEAR);
    }

void WaterRenderer::UpdateWaterShader() {
    const float time = static_cast<float>(GetTime());

    const Vector2 resolution = {
        float(m_captureWidth),
        float(m_captureHeight)
    };

    SetShaderValue(m_waterShader, m_waterCaptureOriginLocation,&m_captureOrigin, SHADER_UNIFORM_VEC2);
    SetShaderValue(m_waterShader, m_waterTimeLocation,  &time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_waterShader, m_waterResolutionLocation, &resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(m_waterShader, m_waterTopLocation, &m_waterTop, SHADER_UNIFORM_FLOAT);

    // Keep the existing DrawModel material bindings.
    m_waterShader.locs[SHADER_LOC_MAP_ALBEDO] = m_waterGroundLocation;
    m_waterShader.locs[SHADER_LOC_MAP_METALNESS] = m_waterSceneLocation;
    m_waterShader.locs[SHADER_LOC_MAP_ROUGHNESS] = m_waterMaskLocation;
    m_waterModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = m_waterGround.texture;
    m_waterModel.materials[0].maps[MATERIAL_MAP_METALNESS].texture = m_waterScene.texture;
    m_waterModel.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = m_waterMask;

    const Vector2 waterOrigin = {
        m_waterCenter.x,
        m_waterCenter.z
    };

    const float maskBounds[4] = {
        m_waterMaskOrigin.x,
        m_waterMaskOrigin.y,
        m_waterMaskSize.x,
        m_waterMaskSize.y
    };

    SetShaderValue(m_waterShader, m_waterTileOriginLocation,&waterOrigin, SHADER_UNIFORM_VEC2);
    SetShaderValue(m_waterShader, m_waterMaskBoundsLocation, maskBounds, SHADER_UNIFORM_VEC4);
}

void WaterRenderer::DrawWaterCliffs(const Camera2D& camera2D) const {
    if (!m_waterCliffTexture.id)
        return;

    BeginMode2D(camera2D);
    DrawTextureV(m_waterCliffTexture, m_waterMaskOrigin, WHITE);
    EndMode2D();
}

void WaterRenderer::GenerateWaterMask(const std::vector<Vector2>& waterTiles)
{

    if (waterTiles.empty())
        return;

    const float tileSize = World::TileSize;

    float minX = waterTiles[0].x - tileSize * 0.5f;
    float maxX = waterTiles[0].x + tileSize * 0.5f;
    float minY = waterTiles[0].y - tileSize * 0.5f;
    float maxY = waterTiles[0].y + tileSize * 0.5f;

    for (const Vector2 tile : waterTiles)
    {
        minX = std::min(minX, tile.x - tileSize * 0.5f);
        maxX = std::max(maxX, tile.x + tileSize * 0.5f);
        minY = std::min(minY, tile.y - tileSize * 0.5f);
        maxY = std::max(maxY, tile.y + tileSize * 0.5f);
    }

    m_waterTop = minY; // Cache the unpadded top once, alongside the mesh.

    // Padding allows the shoreline to curve beyond the original tiles.
    const float padding = tileSize * 2.0f;

    minX -= padding;
    minY -= padding;
    maxX += padding;
    maxY += padding;

    m_waterMaskOrigin = {minX, minY};
    m_waterMaskSize = {maxX - minX, maxY - minY};

    const int width = static_cast<int>(
        std::ceil(m_waterMaskSize.x)
    );

    const int height = static_cast<int>(
        std::ceil(m_waterMaskSize.y)
    );

    std::vector<float> mask(width * height, 0.0f);

    // Fill the mask using the generated water tiles.
    for (const Vector2 tile : waterTiles)
    {
        const int left = std::max(
            0,
            static_cast<int>(
                std::floor(tile.x - tileSize * 0.5f - minX)
            )
        );

        const int right = std::min(
            width,
            static_cast<int>(
                std::ceil(tile.x + tileSize * 0.5f - minX)
            )
        );

        const int top = std::max(
            0,
            static_cast<int>(
                std::floor(tile.y - tileSize * 0.5f - minY)
            )
        );

        const int bottom = std::min(
            height,
            static_cast<int>(
                std::ceil(tile.y + tileSize * 0.5f - minY)
            )
        );

        for (int y = top; y < bottom; ++y){
            for (int x = left; x < right; ++x){
                mask[y * width + x] = 1.0f;
            }
        }
    }

    SmoothWaterMask(mask, width, height);
    // Reflection distances must follow the final, smoothed shoreline.
    GenerateWaterDistances(mask, width, height);
    const auto waterDepth = GenerateWaterCliffs(mask, width, height);
    CreateWaterMaskTexture(mask, waterDepth, width, height);
    CreateWaterMesh();
}

void WaterRenderer::SmoothWaterMask(std::vector<float>& mask, int width, int height) {
    std::vector<float> temp(width * height, 0.0f);
    // Gaussian blur smooths the square tile boundaries.
    constexpr int radius = GameConfig::Water::BlurRadius;
    constexpr float sigma = GameConfig::Water::BlurSigma;

    float weights[radius * 2 + 1]{};
    float weightSum = 0.0f;

    for (int i = -radius; i <= radius; ++i)
    {
        const float weight = std::exp(
            -(float)(i * i) / (2.0f * sigma * sigma)
        );

        weights[i + radius] = weight;
        weightSum += weight;
    }

    for (float& weight : weights)
        weight /= weightSum;

    // Horizontal blur.
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float value = 0.0f;

            for (int i = -radius; i <= radius; ++i)
            {
                const int sx = std::clamp(
                    x + i,
                    0,
                    width - 1
                );

                value +=
                    mask[y * width + sx] *
                    weights[i + radius];
            }

            temp[y * width + x] = value;
        }
    }

    // Vertical blur.
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float value = 0.0f;

            for (int i = -radius; i <= radius; ++i)
            {
                const int sy = std::clamp(
                    y + i,
                    0,
                    height - 1
                );

                value +=
                    temp[sy * width + x] *
                    weights[i + radius];
            }

            mask[y * width + x] = value;
        }
    }

}

std::vector<unsigned char> WaterRenderer::GenerateWaterCliffs(const std::vector<float>& mask, int width, int height) {
    // Visual pit experiment: expose the north-facing rock walls inside the
    // smoothed bank. Bake once so the cliff does not add per-frame tile work.
    std::vector<unsigned char> waterDepth(width * height, 255);
    Image cliffSource = LoadImage("assets/cliff2.png");
    if (cliffSource.data) {
        Image cliffLayer = GenImageColor(width, height, BLANK);
        Image submergedLayer = GenImageColor(width, height, BLANK);
        constexpr int wallHeight = GameConfig::Water::CliffWallHeight;
        constexpr int exposedHeight = GameConfig::Water::CliffExposedHeight;
        constexpr int shadowHeight = GameConfig::Water::CliffShadowHeight;
        for (int x = 0; x < width; ++x) {
            int bankY = -1000;
            for (int y = 0; y < height; ++y) {
                const bool wet = mask[y * width + x] >= 0.5f;
                if (!wet) { bankY = -1000; continue; }
                if (bankY == -1000) bankY = y;
                const int drop = y - bankY;
                if (drop >= wallHeight + shadowHeight) continue;
                Color color{};
                if (drop < wallHeight) {
                    // The central face avoids the sprite's transparent side caps.
                    const int sx = std::min(cliffSource.width - 1, 16 + x % 64);
                    const int sy = std::min(cliffSource.height - 1, 9 + drop * 17 / wallHeight);
                    color = GetImageColor(cliffSource, sx, sy);
                    const float shade = 0.94f - 0.28f * drop / wallHeight;
                    color.r = static_cast<unsigned char>(color.r * shade);
                    color.g = static_cast<unsigned char>(color.g * shade);
                    color.b = static_cast<unsigned char>(color.b * shade);
                } else {
                    color = {12, 23, 24, static_cast<unsigned char>(85 * (wallHeight + shadowHeight - drop) / shadowHeight)};
                }
                const int drawY = y - exposedHeight;
                if (drawY < 0) continue;
                if (drop < exposedHeight) {
                    ImageDrawPixel(&cliffLayer, x, drawY, color);
                } else {
                    // Continue the same rock face beneath the surface. The water
                    // shader tints this capture; only the dry rim is drawn on top.
                    ImageDrawPixel(&submergedLayer, x, drawY, color);
                    const float depth = std::clamp(float(drop - exposedHeight) /
                        (wallHeight - exposedHeight), 0.0f, 1.0f);
                    waterDepth[drawY * width + x] = static_cast<unsigned char>(depth * 255);
                }
            }
        }
        // The second sprite in cliff2.png is the thin foreground bank.
        // Follow every south-facing shoreline run, using its centre strip so
        // the end caps do not repeat along the curved lower sides.
        if (cliffSource.width >= 80 && cliffSource.height >= 96) {
            constexpr int lipTop = 80;
            constexpr int lipHeight = 12;
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    if (mask[y * width + x] < 0.5f) continue;
                    if (y + 1 < height && mask[(y + 1) * width + x] >= 0.5f) continue;
                    for (int row = 0; row < lipHeight; ++row) {
                        const int drawY = y - 2 + row;
                        if (drawY < 0 || drawY >= height) continue;
                        const Color color = GetImageColor(cliffSource, 16 + x % 64, lipTop + row);
                        if (color.a == 0) continue;
                        ImageDrawPixel(&cliffLayer, x, drawY, color);
                    }
                }
            }
        }
        if (m_waterCliffTexture.id) UnloadTexture(m_waterCliffTexture);
        m_waterCliffTexture = LoadTextureFromImage(cliffLayer);
        if (m_submergedCliffTexture.id) UnloadTexture(m_submergedCliffTexture);
        m_submergedCliffTexture = LoadTextureFromImage(submergedLayer);
        SetTextureFilter(m_submergedCliffTexture, TEXTURE_FILTER_POINT);
        SetTextureFilter(m_waterCliffTexture, TEXTURE_FILTER_POINT);
        UnloadImage(cliffLayer);
        UnloadImage(submergedLayer);
        UnloadImage(cliffSource);
    }
    return waterDepth;
}

void WaterRenderer::CreateWaterMaskTexture(const std::vector<float>& mask,
    const std::vector<unsigned char>& waterDepth, int width, int height) {
    // Convert the mask into an image.
    std::vector<Color> pixels(width * height);

    for (int i = 0; i < width * height; ++i)
    {
        const unsigned char value =
            static_cast<unsigned char>(
                std::clamp(mask[i], 0.0f, 1.0f) * 255.0f
            );

        // Coverage and visual depth share a texture and the same shoreline.
        pixels[i] = {value, waterDepth[i], 0, 255};
    }

    Image maskImage{};
    maskImage.data = pixels.data();
    maskImage.width = width;
    maskImage.height = height;
    maskImage.mipmaps = 1;
    maskImage.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    m_waterMask = LoadTextureFromImage(maskImage);

    SetTextureFilter(
        m_waterMask,
        TEXTURE_FILTER_BILINEAR
    );

    SetTextureWrap(
        m_waterMask,
        TEXTURE_WRAP_CLAMP
    );

}

void WaterRenderer::CreateWaterMesh() {
    // Create one continuous plane covering the entire pond.
    Mesh waterMesh = GenMeshPlane(
        m_waterMaskSize.x,
        m_waterMaskSize.y,
        1,
        1
    );

    m_waterModel = LoadModelFromMesh(waterMesh);
    m_waterModel.materials[0].shader = m_waterShader;

    m_waterCenter = {
        m_waterMaskOrigin.x + m_waterMaskSize.x * 0.5f,
        0.0f,
        m_waterMaskOrigin.y + m_waterMaskSize.y * 0.5f
    };

    m_waterMaskLocation = GetShaderLocation(
        m_waterShader,
        "waterMask"
    );

    m_waterMaskBoundsLocation = GetShaderLocation(
        m_waterShader,
        "waterMaskBounds"
    );
}

    // Only water within two tiles can contribute to a sprite reflection.
    float WaterRenderer::ReflectionWaterDistance(float feetX, float feetY) const {
        constexpr float maxDistance = World::TileSize * GameConfig::Water::ReflectionRangeTiles;

        const int x = static_cast<int>(std::floor(feetX - m_waterMaskOrigin.x));
        const int y = static_cast<int>(std::floor(feetY - m_waterMaskOrigin.y));

        if (x < 0 || y < 0 ||
            x >= m_waterMaskWidth || y >= m_waterMaskHeight ||
            m_waterDistances.empty()) {
            return maxDistance;
        }

        return std::min(
            m_waterDistances[y * m_waterMaskWidth + x],
            maxDistance
        );
    }

    void WaterRenderer::DrawSpriteReflection(Texture2D texture, Rectangle source, Rectangle destination, float feetY) const
    {
        if (!texture.id) return;
        const float feetX = destination.x + destination.width * 0.5f;
        const float distance = ReflectionWaterDistance(feetX, feetY);
        const float proximity = 1.0f - distance / (World::TileSize * GameConfig::Water::ReflectionRangeTiles);
        if (proximity <= 0 || !texture.id) return;
        const float aboveFeet = std::clamp(feetY - destination.y, 0.0f, destination.height);
        const float length = aboveFeet * GameConfig::Water::ReflectionLengthScale;
        // Reflect each strip about the sprite's feet, not a pond-wide axis.
        for (int row = 0; row < static_cast<int>(std::ceil(length)); ++row) {
            const float height = std::min(1.0f, length - row);
            const float sourceHeight = height / GameConfig::Water::ReflectionLengthScale * source.height / destination.height;
            const float sourceY = source.y + (aboveFeet - (row + height) / GameConfig::Water::ReflectionLengthScale) * source.height / destination.height;
            const unsigned char alpha = static_cast<unsigned char>(GameConfig::Water::ReflectionMaxAlpha * proximity * (1.0f - row / length));
            DrawTexturePro(texture, {source.x, sourceY, source.width, -sourceHeight},
                {destination.x, feetY + row, destination.width, height}, {0,0}, 0, {255,255,255,alpha});
        }
    }

    void WaterRenderer::DrawWaterReflections(const World& world, const Tileset& tileset) const {
        const Chunk& chunk = world.GetChunk();
        constexpr float tileSize = World::TileSize;

        // Reflection eligibility uses sprite feet, not the canopy's top-left.
        // Invert the same anchor calculation so even large sprites are retained.
        constexpr float reach = tileSize * GameConfig::Water::ReflectionRangeTiles;
        constexpr float footOffsetX = tileSize * 0.5f + GameConfig::Trees::AnchorOffset.x;
        constexpr float footOffsetY = tileSize + GameConfig::Trees::AnchorOffset.y;
        const int left = std::clamp(int(std::floor((m_waterMaskOrigin.x - reach - footOffsetX) / tileSize)), 0, Chunk::CHUNK_WIDTH);
        const int top = std::clamp(int(std::floor((m_waterMaskOrigin.y - reach - footOffsetY) / tileSize)), 0, Chunk::CHUNK_HEIGHT);
        const int right = std::clamp(int(std::ceil((m_waterMaskOrigin.x + m_waterMaskSize.x + reach - footOffsetX) / tileSize)) + 1, 0, Chunk::CHUNK_WIDTH);
        const int bottom = std::clamp(int(std::ceil((m_waterMaskOrigin.y + m_waterMaskSize.y + reach - footOffsetY) / tileSize)) + 1, 0, Chunk::CHUNK_HEIGHT);
        for (int y = top; y < bottom; ++y) {
            for (int x = left; x < right; ++x) {
                const Tile tile = chunk.GetTile(x, y);

                if (tile.type != TileType::Tree)
                    continue;

                const Rectangle source = tileset.GetTree(tile.metadata);
                const float width = source.width * GameConfig::Trees::RenderScale;
                const float height = source.height * GameConfig::Trees::RenderScale;
                const float feet = (y + 1) * tileSize + GameConfig::Trees::AnchorOffset.y;

                DrawSpriteReflection(
                    tileset.GetTreeAtlasTexture(),
                    source,
                    {
                        x * tileSize + (tileSize - width) * 0.5f + GameConfig::Trees::AnchorOffset.x,
                        feet - height,
                        width,
                        height
                    },
                    feet
                );
            }
        }

        for (const WorldObject& object : world.GetObjects()) {
            DrawSpriteReflection(
                object.texture,
                {0, 0, float(object.texture.width), float(object.texture.height)},
                {
                    object.position.x,
                    object.position.y,
                    float(object.texture.width),
                    float(object.texture.height)
                },
                object.position.y + object.texture.height
            );
        }
    }

    void WaterRenderer::GenerateWaterDistances(const std::vector<float>& mask, int width, int height) 
    {
        m_waterMaskWidth = width;
        m_waterMaskHeight = height;

        constexpr float infinity = 1e9f;
        constexpr float diagonal = 1.41421356f;

        m_waterDistances.assign(width * height, infinity);

        // Water begins where the smoothed mask reaches 50% coverage.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int index = y * width + x;

                if (mask[index] >= 0.5f) {
                    m_waterDistances[index] = 0.0f;
                }
            }
        }

        // First pass: top-left to bottom-right.
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float& distance = m_waterDistances[y * width + x];

                if (x > 0)
                    distance = std::min(distance, m_waterDistances[y * width + x - 1] + 1.0f);

                if (y > 0)
                    distance = std::min(distance, m_waterDistances[(y - 1) * width + x] + 1.0f);

                if (x > 0 && y > 0)
                    distance = std::min(distance, m_waterDistances[(y - 1) * width + x - 1] + diagonal);

                if (x + 1 < width && y > 0)
                    distance = std::min(distance, m_waterDistances[(y - 1) * width + x + 1] + diagonal);
            }
        }

        // Second pass: bottom-right to top-left.
        for (int y = height - 1; y >= 0; --y) {
            for (int x = width - 1; x >= 0; --x) {
                float& distance = m_waterDistances[y * width + x];

                if (x + 1 < width)
                    distance = std::min(distance, m_waterDistances[y * width + x + 1] + 1.0f);

                if (y + 1 < height)
                    distance = std::min(distance, m_waterDistances[(y + 1) * width + x] + 1.0f);

                if (x + 1 < width && y + 1 < height)
                    distance = std::min(distance, m_waterDistances[(y + 1) * width + x + 1] + diagonal);

                if (x > 0 && y + 1 < height)
                    distance = std::min(distance, m_waterDistances[(y + 1) * width + x - 1] + diagonal);
            }
        }
    }
}