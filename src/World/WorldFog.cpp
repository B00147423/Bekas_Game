#include "World.h"

#include <algorithm>
#include <cmath>
#include <raymath.h>

namespace World
{
    World::~World()
    {
        ClearColliders();
        if (m_explorationTexture.id) UnloadTexture(m_explorationTexture);
    }

    void World::GenerateFogRegions()
    {
        static_assert(GameConfig::Fog::MaxRegionDiameter >= 16.0f);
        static_assert(GameConfig::Fog::RegionOverlap > 0.0f && GameConfig::Fog::RegionOverlap <= 1.0f);
        static_assert(GameConfig::Fog::MaxRegionExtension >= 0.0f);
        // Keep a bounded exploration window. Revealed texels persist when it moves.
        constexpr float windowSize=128*TileSize;
        const float worldWidth = std::min(Chunk::CHUNK_WIDTH * TileSize,windowSize);
        const float worldHeight = std::min(Chunk::CHUNK_HEIGHT * TileSize,windowSize);
        const float step=32*TileSize;
        m_fogOrigin={std::clamp(std::floor((m_activePosition.x-worldWidth*0.5f)/step)*step,
                               0.0f,Chunk::CHUNK_WIDTH*TileSize-worldWidth),
                     std::clamp(std::floor((m_activePosition.y-worldHeight*0.5f)/step)*step,
                               0.0f,Chunk::CHUNK_HEIGHT*TileSize-worldHeight)};
        m_fogWidth = static_cast<int>(std::ceil(worldWidth / FogPixelSize));
        m_fogHeight = static_cast<int>(std::ceil(worldHeight / FogPixelSize));
        const float reach = GameConfig::Fog::MaxRegionDiameter * 0.5f;
        const int columns = static_cast<int>(std::ceil(worldWidth / (reach * 0.72f)));
        const int rows = static_cast<int>(std::ceil(worldHeight / (reach * 0.72f)));
        const Vector2 spacing{worldWidth / columns, worldHeight / rows};
        
        const auto random = [&](uint32_t index) {
            uint32_t value = index + uint32_t(m_seed) + uint32_t(m_seed >> 32) * 0x9e3779b9u;
            value ^= value >> 16;
            value *= 0x7feb352du;
            value ^= value >> 15;
            value *= 0x846ca68bu;
            value ^= value >> 16;
            return float(value >> 8) / 16777216.0f;
        };
        
        std::vector<Vector2> seeds;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < columns; ++x) {
                const uint32_t index = y * columns + x;
                seeds.push_back({(x + 0.5f + 0.60f * (random(2 * index) - 0.5f)) * spacing.x,
                                 (y + 0.5f + 0.60f * (random(2 * index + 1) - 0.5f)) * spacing.y});
            }
        }

        // Every region lies inside a radius-limited disk around its seed.
        // Reserving the texel diagonal includes the texel corners in the size bound.
        const float eligible = reach - FogPixelSize * std::sqrt(0.5f);
        std::vector<int> owners(m_fogWidth * m_fogHeight);
        for (int y = 0; y < m_fogHeight; ++y) {
            for (int x = 0; x < m_fogWidth; ++x) {
                const Vector2 point{(x + 0.5f) * FogPixelSize, (y + 0.5f) * FogPixelSize};
                const int column = std::min(int(point.x / spacing.x), columns - 1);
                const int row = std::min(int(point.y / spacing.y), rows - 1);
                int winner = row * columns + column;
                float best = Vector2DistanceSqr(point, seeds[winner]);
                for (int sy = std::max(0, row - 2); sy <= std::min(rows - 1, row + 2); ++sy) {
                    for (int sx = std::max(0, column - 2); sx <= std::min(columns - 1, column + 2); ++sx) {
                        const int index = sy * columns + sx;
                        if (Vector2DistanceSqr(point, seeds[index]) > eligible * eligible) continue;
                        const float score = Vector2DistanceSqr(point, seeds[index]);
                        if (score < best) { best = score; winner = index; }
                    }
                }
                owners[y * m_fogWidth + x] = winner;
            }
        }

        m_fogRegions.clear();
        for (int index = 0; index < int(owners.size()); ++index) {
            const int owner = owners[index];
            if (owner < 0) continue;
            const int x = index % m_fogWidth, y = index / m_fogWidth;
            auto& region = m_fogRegions.emplace_back(FogRegion{{index}, false, x, y, x, y});
            owners[index] = -1;
            const auto visit = [&](int next) {
                if (owners[next] == owner) { owners[next] = -1; region.pixels.push_back(next); }
            };
            for (size_t i = 0; i < region.pixels.size(); ++i) {
                const int pixel = region.pixels[i];
                const int px = pixel % m_fogWidth, py = pixel / m_fogWidth;
                region.left = std::min(region.left, px);
                region.top = std::min(region.top, py);
                region.right = std::max(region.right, px);
                region.bottom = std::max(region.bottom, py);
                if (px > 0) visit(pixel - 1);
                if (px + 1 < m_fogWidth) visit(pixel + 1);
                if (py > 0) visit(pixel - m_fogWidth);
                if (py + 1 < m_fogHeight) visit(pixel + m_fogWidth);
            }
        }
        m_explorationPixels.assign(owners.size(), 0);
        const int originX=int(m_fogOrigin.x/FogPixelSize),originY=int(m_fogOrigin.y/FogPixelSize);
        for(auto& region:m_fogRegions) {
            bool allSeen=true;
            for(int pixel:region.pixels) {
                const uint64_t key=(uint64_t(originY+pixel/m_fogWidth)<<32)|uint32_t(originX+pixel%m_fogWidth);
                const bool seen=m_exploredFogPixels.contains(key);
                m_explorationPixels[pixel]=seen?255:0;allSeen &= seen;
            }
            region.discovered=allSeen;
        }
        if (m_explorationTexture.id) UnloadTexture(m_explorationTexture);
        const Image image{m_explorationPixels.data(), m_fogWidth, m_fogHeight, 1,
                          PIXELFORMAT_UNCOMPRESSED_GRAYSCALE};
        m_explorationTexture = LoadTextureFromImage(image);
        m_lastFogRadius = -1.0f;
    }

    void World::UpdateFog(Vector2 lightPosition, float lightRadius){

    const float radius = std::min(lightRadius, GameConfig::Fog::DiscoveryRadius) + GameConfig::Fog::DiscoveryLead;
        if(!std::isfinite(lightPosition.x) || !std::isfinite(lightPosition.y)) return;
        const float margin=radius+GameConfig::Fog::MaxRegionDiameter;
        const bool shiftX=(lightPosition.x<m_fogOrigin.x+margin && m_fogOrigin.x>0) ||
            (lightPosition.x>m_fogOrigin.x+m_fogWidth*FogPixelSize-margin && m_fogOrigin.x+m_fogWidth*FogPixelSize<Chunk::CHUNK_WIDTH*TileSize);
        const bool shiftY=(lightPosition.y<m_fogOrigin.y+margin && m_fogOrigin.y>0) ||
            (lightPosition.y>m_fogOrigin.y+m_fogHeight*FogPixelSize-margin && m_fogOrigin.y+m_fogHeight*FogPixelSize<Chunk::CHUNK_HEIGHT*TileSize);
        if(shiftX || shiftY) { m_activePosition=lightPosition; GenerateFogRegions(); }
        lightPosition.x-=m_fogOrigin.x;lightPosition.y-=m_fogOrigin.y;

        if (!m_explorationTexture.id || !std::isfinite(radius) || radius <= 0.0f ||
            !std::isfinite(lightPosition.x) || !std::isfinite(lightPosition.y)) return;
        if (radius == m_lastFogRadius && lightPosition.x == m_lastFogPosition.x &&
            lightPosition.y == m_lastFogPosition.y) return;
        m_lastFogRadius = radius;
        m_lastFogPosition = lightPosition;

        const float radiusSquared = radius * radius;
        const float limit = radius + GameConfig::Fog::MaxRegionExtension;
        const auto farthestSquared = [&](float left, float top, float right, float bottom) {
            const float dx = std::max(std::abs(left - lightPosition.x), std::abs(right - lightPosition.x));
            const float dy = std::max(std::abs(top - lightPosition.y), std::abs(bottom - lightPosition.y));
            return dx * dx + dy * dy;
        };
        int left = m_fogWidth, top = m_fogHeight, right = -1, bottom = -1;
        for (auto& region : m_fogRegions) {
            if (region.discovered) continue;
            const float x0 = region.left * FogPixelSize, y0 = region.top * FogPixelSize;
            const float x1 = (region.right + 1) * FogPixelSize, y1 = (region.bottom + 1) * FogPixelSize;
            const Vector2 nearest{std::clamp(lightPosition.x, x0, x1), std::clamp(lightPosition.y, y0, y1)};
            if (Vector2DistanceSqr(lightPosition, nearest) >= radiusSquared) continue;
            if (farthestSquared(x0, y0, x1, y1) > radiusSquared) {
                int covered = 0;
                bool tooFar = false;
                for (int pixel : region.pixels) {
                    const float x = (pixel % m_fogWidth) * FogPixelSize;
                    const float y = (pixel / m_fogWidth) * FogPixelSize;
                    const float farthest = farthestSquared(x, y, x + FogPixelSize, y + FogPixelSize);
                    if (farthest > limit * limit) { tooFar = true; break; }
                    if (farthest <= radiusSquared) ++covered;
                }
                if (tooFar || covered < GameConfig::Fog::RegionOverlap * region.pixels.size()) continue;
            }
            region.discovered = true;
            for (int pixel : region.pixels) {
                m_explorationPixels[pixel] = 255;
                const uint64_t key=(uint64_t(int(m_fogOrigin.y/FogPixelSize)+pixel/m_fogWidth)<<32)|
                    uint32_t(int(m_fogOrigin.x/FogPixelSize)+pixel%m_fogWidth);
                m_exploredFogPixels.insert(key);
            }
            left = std::min(left, region.left);
            top = std::min(top, region.top);
            right = std::max(right, region.right);
            bottom = std::max(bottom, region.bottom);
        }
        if (right < left) return;
        m_fogUpload.clear();
        for (int y = top; y <= bottom; ++y) {
            const auto row = m_explorationPixels.begin() + y * m_fogWidth;
            m_fogUpload.insert(m_fogUpload.end(), row + left, row + right + 1);
        }
        UpdateTextureRec(m_explorationTexture,
            {float(left), float(top), float(right - left + 1), float(bottom - top + 1)}, m_fogUpload.data());
    }
}
