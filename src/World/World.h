#pragma once

#include <cstdint>
#include "GameConfig.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "Rendering/Tileset.h"
#include "Generation/WorldGenerator.h"
#include "Chunk.h"
#include "Navigation/FlowField.h"
#include "Physics/PhysicsWorld.h"
#include "Lighting/ShadowStyle.h"
#include "Lighting/ShadowRenderer2D.h"
#include "rlgl.h"
namespace World
{


    struct FogRegion
    {
        std::vector<int> pixels; // Pixel indices in the exploration mask.
        bool discovered = false;
        int left = 0, top = 0, right = 0, bottom = 0;
    };

    struct WorldObject
    {
        Texture2D texture{};
        Vector2 position{};
        Lighting::ShadowStyle shadow{};
        bool hasCollision = true;
        Vector2 colliderSize{};
        Vector2 colliderOffset{};
    };

// Implementations of this class are grouped by responsibility:
// World.cpp: construction, generation, and state coordination.
// WorldRendering.cpp: terrain masks and ground/tree/object drawing.
// WorldShadows.cpp: occluders and shadow-caster submission.
// WorldNavigation.cpp: targets, steering, and navigation debug drawing.
// WorldPhysics.cpp: colliders, placement checks, and object insertion.
// WorldFog.cpp: exploration state/texture updates and destruction.
class World
    {
    public:
        World(uint64_t seed, Physics::PhysicsWorld& physicsWorld);

        ~World();
        World(const World&) = delete;
        World& operator=(const World&) = delete;

        void Generate(Vector2 playerPosition);
        // Removes this world's bodies only; retains tiles and placed objects.
        void ClearColliders();
        void UpdateActiveArea(Vector2 position);
        void ClearSpawnArea(int centerTileX, int centerTileY, int radius);

        void DrawGround(const Tileset& tileset, Rectangle visibleBounds = {
            0, 0, Chunk::CHUNK_WIDTH * GameConfig::World::TileSize,
            Chunk::CHUNK_HEIGHT * GameConfig::World::TileSize}) const;
        std::vector<Vector2> GetWaterTiles() const;

        bool isForest(int tileX, int tileY) const;
        uint8_t GetForestMask(int tileX, int tileY) const;
        void DrawTrees(const Tileset& tileset, float playerFeetY, bool behindPlayer, Color tint = WHITE,
            Rectangle visibleBounds = {0,0,Chunk::CHUNK_WIDTH * GameConfig::World::TileSize,
                Chunk::CHUNK_HEIGHT * GameConfig::World::TileSize}) const;
        void DrawAutoShadows(const Tileset& tileset, Lighting::ShadowRenderer2D& renderer, Vector2 lightPosition, float shadowLength, Rectangle visibleBounds, float lightRadius, Color maskColor = BLACK) const;
        void DrawOccluders() const;
        void DrawDebugGrid(Rectangle visibleBounds) const;
        // Explicit full collider dimensions; decorations pass {} with hasCollision=false. Position is texture top-left.
        void AddObject(Texture2D texture, Vector2 position, Vector2 colliderSize, Vector2 colliderOffset = {}, Lighting::ShadowStyle shadow = {}, bool hasCollision = true);
        void DrawObjects(float playerFeetY,bool behindPlayer, Vector2 mouseWorld, Shader hoverShader) const;
        bool CanPlaceObject(Vector2 position, Vector2 size) const;
        void Draw3D(const Tileset& tileset, const Camera3D& camera) const;

        void DrawFlowField(Vector2 worldTopLeft, Vector2 worldBottomRight) const;
        void SetFlowFieldTarget(Vector2 worldPosition);
        void UpdateFog(Vector2 lightPosition, float lightRadius);
        //void GenerateStartingForest(int centerX, int centerY);
        // Steering direction in world space (toward next tile center / escape)
        Vector2 GetFlowDirection(Vector2 worldPosition, float maxStep = 0.0f) const;
        bool IsInFlowTargetTile(Vector2 worldPosition) const;



        bool IsTreeAt(Vector2 worldPosition) const;
        bool RemoveTree(Vector2 worldPosition);

        bool FindTreeApproachPoint( Vector2 treePosition, Vector2 playerPosition, Vector2& approachPoint) const;
        static constexpr float TileSize = GameConfig::World::TileSize;

        // When blocked by physics, try a side step along the field
        Vector2                                 GetSlideDirection(Vector2 worldPosition, Vector2 blockedDir) const;
        const Chunk&                            GetChunk() const;
        const std::vector<WorldObject>&         GetObjects() const;
        const Texture2D&                        GetExplorationTexture() const;
        Rectangle GetExplorationBounds() const {
            return {m_fogOrigin.x,m_fogOrigin.y,m_fogWidth*FogPixelSize,m_fogHeight*FogPixelSize};
        }
    private:

        void GenerateFogRegions();
        //void BuildConnectedFogRegions(std::vector<int> owners);
        //bool CanDiscoverFogRegion(const FogRegion& region, Vector2 lightPosition, float radius) const;
        //void UploadFogChanges(int left, int top, int right, int bottom);
        static constexpr float FogPixelSize = 2.0f;
        int m_fogWidth = 0, m_fogHeight = 0;
        Vector2 m_fogOrigin{};
        std::unordered_set<uint64_t> m_exploredFogPixels;
        Vector2 m_lastFogPosition{};
        float m_lastFogRadius = -1.0f;
        std::vector<unsigned char> m_fogUpload;

        //fog of war

        std::vector<FogRegion> m_fogRegions;
        std::vector<unsigned char> m_explorationPixels;
        Texture2D m_explorationTexture{};

        void DrawGrassTile(const Tileset& tileset, int tileX, int tileY, uint8_t metadata, const Rectangle& dest, Color tint) const;
        void CreateTreeColliders();
        void CreateObjectCollider(const WorldObject& object);
        Physics::PhysicsWorld& m_physicsWorld;
        // Cached while PhysicsWorld is alive: check before accessing its reference
        // during cleanup. PhysicsWorld owns and destroys this Box2D simulation.
        b2WorldId m_physicsWorldId{};
        std::vector<b2BodyId> m_colliders;
        std::unordered_map<int,b2BodyId> m_treeColliders;
        Vector2 m_activePosition{};
        int m_activeBucketX = -10000, m_activeBucketY = -10000;

        std::vector<WorldObject> m_objects;

        
        FlowField m_flowField;
        Vector2 m_flowTarget{};
        bool m_hasFlowTarget = false;
        uint64_t m_seed;

        Generation::WorldGenerator m_worldGenerator;
        Chunk m_chunk;
    };
}
