#pragma once
#include <raylib.h>
#include "Lighting/ShadowStyle.h"
#include <array>
// World distances are pixels; collider sizes are full widths/heights, not Box2D half extents.
namespace GameConfig
{
    // Player
    namespace Player
    {
        inline constexpr Vector2 SpriteSize = {32.0f, 32.0f}; // Rendered width and height.
        inline constexpr Vector2 ColliderSize = {10.0f, 10.0f}; // Independent collision width and height.
        inline constexpr Vector2 BodyOffset = {32.0f * 0.5f, 32.0f * 0.65f}; // Collider center from sprite top-left; also camera/light/feet anchor. Original placement, independent of rendering size.
        inline constexpr Vector2 SpawnPosition = {184.0f, 184.0f};// Sprite top-left at spawn.
        inline constexpr int FrameWidth = 32; // Source sprite-sheet frame width in texels.
        inline constexpr int FrameHeight = 32; // Source sprite-sheet frame height in texels.
        inline constexpr int FramesPerRow = 29; // Source sheet layout.
        inline constexpr int IdleFirstFrame = 0;
        inline constexpr int IdleLastFrame = 1;
        inline constexpr float IdleFrameDuration = 0.15f; // Seconds per idle frame.
        inline constexpr int WalkFirstFrame = 2;
        inline constexpr int WalkLastFrame = 4;
        inline constexpr float WalkFrameDuration = 0.10f; // Seconds per walking frame.
        inline constexpr float Speed = 150.0f; // World pixels per second.
        inline constexpr char TexturePath[] = "assets/human.png";


    }

    // Trees
    namespace Trees
    {
        inline constexpr float RenderScale = 0.5f; // Uniform 2:1 reduction; preserve sprite proportions.
        inline constexpr Rectangle SpriteSource = {116.0f, 25.0f, 24.0f, 39.0f}; // Atlas x/y and source width/height.
            inline constexpr std::array<Rectangle, 11> SpriteSourcesTree = {{
                // 8 pine trees
                {   0, 144, 48, 96 },
                {  48, 144, 48, 96 },
                {  96, 144, 48, 96 },
                { 144, 144, 48, 96 },
                { 192, 144, 48, 96 },
                { 240, 144, 48, 96 },
                { 288, 144, 48, 96 },
                { 336, 144, 48, 96 },

                // 3 round trees
                { 384, 144, 96, 96 },
                { 480, 144, 96, 96 },
                { 576, 144, 96, 96 }
            }};

        inline constexpr Vector2 AnchorOffset = {0.0f, 0.0f}; // Sprite bottom-center offset from tile bottom-center; also moves collider and shadow anchors.
        // Trunk footprint width/height in world pixels, matching SpriteSourcesTree order.
        // Starting values: tune each entry to its sprite's trunk base.
        inline constexpr std::array<Vector2, 11> ColliderSizes = {{
            {8, 6},   // Pine 0
            {8, 6},   // Pine 1
            {10, 7},  // Pine 2
            {10, 7},  // Pine 3
            {8, 6},   // Pine 4
            {10, 7},  // Pine 5
            {12, 8},  // Pine 6
            {12, 8},  // Pine 7
            {14, 9},  // Round tree 8
            {16, 10}, // Round tree 9
            {18, 12}  // Round tree 10
        }};
        static_assert(ColliderSizes.size() == SpriteSourcesTree.size());
        inline constexpr Vector2 ColliderOffset = {0.0f, 0.0f}; // Collision center adjustment from bottom-centered stump at tile feet.
        inline constexpr Lighting::ShadowStyle Shadow{
            Lighting::ShadowMode::Footprint,
            {14, 9},  // Footprint width and height
            {0, -1},  // Shadow position offset
            24,       // Projected shadow length (was 16)
            28,       // Contact shadow size parameter
            0.9f,    // Projected shadow opacity (was 0.45)
            0.9f     // Contact shadow opacity (was 0.75)
        };
    }

    // Tent
    namespace Tent
    {
        inline constexpr char TexturePath[] = "assets/tent.png";
        inline constexpr Vector2 SpawnOffset = {48.0f, 0.0f}; // Sprite top-left offset from player spawn.
        inline constexpr Vector2 ColliderSize = {20.0f, 20.0f}; // Preserved collision width/height in world pixels; historical approximate footprint, not derived from texture size.
        inline constexpr Vector2 ColliderOffset = {0.0f, -5.0f}; // Collision center adjustment from bottom-centered footprint of the rendered sprite.
        inline constexpr Vector2 ShadowOffset = {0.0f, -5.0f}; // Base center from sprite bottom-center.
        inline constexpr Lighting::ShadowStyle Shadow{
            Lighting::ShadowMode::Footprint,
            {36, 16},
            ShadowOffset,
            28,       // Longer projected shadow
            36,
            0.6f, // Projected shadow opacity
            0.6f  // Contact shadow opacity
        };
        inline constexpr Rectangle LegacyAtlasSource = {386.0f, 676.0f, 45.0f, 42.0f}; // Existing GetTent() atlas rectangle; not used for tent.png drawing.

    }

    // World
    namespace World
    {
        inline constexpr float TileSize = 16.0f; // World grid/render spacing; does not resize atlas source cells.
        inline constexpr float AtlasTileSize = 16.0f; // Source texture cell width/height in texels.
        inline constexpr Vector2 GrassAtlasOrigin = {608.0f, 816.0f}; // Top-left source texel of grass variants.
        inline constexpr int GrassVariantsPerSide = 8; // Rows and columns in the grass source block.
        inline constexpr int RareGrassChancePercent = 6;
        inline constexpr int SpawnClearRadius = 7; // Tree clearing radius in tiles.
        inline constexpr char GrassTexturePath[] = "assets/grass_top_layer.png";
        inline constexpr char FoliageTexturePath[] = "assets/Trees_Flowers.png";
        inline constexpr char ForestTexturePath[] = "assets/47Tiles_grass.png";
        inline constexpr Color DaylightTint = {255, 240, 170, 255}; // F3 rendering tint.

        inline constexpr Vector3 LightColor = {0.6f, 0.74f, 0.21f};
        inline constexpr float LightRadius = 90.0f;
        inline constexpr float LightIntensity = 1.0f;
        inline constexpr float Ambient = 0.30f;

#ifdef BEKAS_WORLD_WIDTH
        inline constexpr int ChunkWidth = BEKAS_WORLD_WIDTH;
#else
        inline constexpr int ChunkWidth = 2048;
#endif
#ifdef BEKAS_WORLD_HEIGHT
        inline constexpr int ChunkHeight = BEKAS_WORLD_HEIGHT;
#else
        inline constexpr int ChunkHeight = 2048;
#endif
        inline constexpr int StartingTreeCount = 12;
        
    }

    namespace Water
    {
        inline constexpr float CameraHeight = 500.0f;

        inline constexpr int BlurRadius = 12;
        inline constexpr float BlurSigma = 6.0f;

        inline constexpr int CliffWallHeight = 40;
        inline constexpr int CliffExposedHeight = 8;
        inline constexpr int CliffShadowHeight = 5;

        inline constexpr float ReflectionRangeTiles = 2.0f;
        inline constexpr float ReflectionLengthScale = 0.7f;
        inline constexpr float ReflectionMaxAlpha = 180.0f;

        inline constexpr char SurfaceVertexShaderPath[] = "src/shaders/water3d.vs";
        inline constexpr char SurfaceFragmentShaderPath[] = "src/shaders/water3d.fs";
        // World-space capture extends this far beyond each edge of the chunk.
        inline constexpr int CapturePadding = 256;
        inline constexpr Vector2 CaptureOrigin = {-float(CapturePadding), -float(CapturePadding)};
        inline constexpr int CaptureWidth = static_cast<int>(World::ChunkWidth * World::TileSize) + 2 * CapturePadding;
        inline constexpr int CaptureHeight = static_cast<int>(World::ChunkHeight * World::TileSize) + 2 * CapturePadding;
    }

    namespace Shadows
    {
        inline constexpr float Softness = 10.0f; // Emitter disk radius in world pixels; zero = hard.
        inline constexpr float Opacity = 0.90f; // 0 = no shadow, 1 = fully blocked direct light.
        inline constexpr int Samples = 8; // 4, 8 or 16; higher values reduce penumbra banding.
    }

    namespace Fog
    {
        inline constexpr float RegionOverlap = 0.25f;
        inline constexpr float MaxRegionDiameter = 48.0f;
        inline constexpr float MaxRegionExtension = 12.0f;

        inline constexpr float DiscoveryRadius = World::LightRadius;
        inline constexpr float ExploredDarkness = 0.80f;

        inline constexpr float CornerRadius = 8.0f;
        inline constexpr float BoundaryFeather = 12.0f;

        inline constexpr float DiscoveryLead = 32.0f;
    }
}
