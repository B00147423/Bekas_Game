#include "World.h"
#include "VisibleTiles.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace World
{
    void World::DrawDebugGrid(Rectangle visibleBounds) const{
        const auto bounds=VisibleTiles(visibleBounds);
        for (int y = bounds.top; y < bounds.bottom; ++y){
            for (int x = bounds.left; x < bounds.right; ++x){
                int cost = m_flowField.GetCost(x, y);

                if (cost >= 0){
                    DrawText(
                        TextFormat("%d", cost),
                        x * TileSize + 2,
                        y * TileSize + 2,
                        8,
                        DARKBLUE
                    );
                }
            }
        }
    }

    // flow FIELD DEBUG DRAWING
    void World::DrawFlowField(Vector2 worldTopLeft, Vector2 worldBottomRight) const{
        m_flowField.DrawFlowField(
            TileSize,
            worldTopLeft,
            worldBottomRight
        );
    }

    void World::SetFlowFieldTarget(Vector2 worldPosition){
        const int targetX =static_cast<int>(std::floor(worldPosition.x / TileSize));

        const int targetY = static_cast<int>(std::floor(worldPosition.y / TileSize));

        if (targetX < 0 || targetX >= Chunk::CHUNK_WIDTH ||
            targetY < 0 || targetY >= Chunk::CHUNK_HEIGHT)
        {
            return;
        }

        const Tile tile = m_chunk.GetTile(targetX, targetY);

        if (tile.solid)
            return;

        m_flowTarget = worldPosition;
        m_hasFlowTarget = true;
        m_flowField.Generate(
            m_chunk,
            targetX,
            targetY,
            int(m_activePosition.x / TileSize), int(m_activePosition.y / TileSize)
        );
    }

    bool World::IsInFlowTargetTile(Vector2 worldPosition) const
    {
        return m_hasFlowTarget &&
            std::floor(worldPosition.x / TileSize) == std::floor(m_flowTarget.x / TileSize) &&
            std::floor(worldPosition.y / TileSize) == std::floor(m_flowTarget.y / TileSize);
    }

    Vector2 World::GetFlowDirection(Vector2 worldPosition, float maxStep) const
    {
        int tileX = static_cast<int>(worldPosition.x / TileSize);
        int tileY = static_cast<int>(worldPosition.y / TileSize);

        if (tileX < 0 || tileX >= Chunk::CHUNK_WIDTH ||
            tileY < 0 || tileY >= Chunk::CHUNK_HEIGHT)
        {
            return { 0.0f, 0.0f };
        }

        if (IsInFlowTargetTile(worldPosition)) {
            const Vector2 delta = {m_flowTarget.x - worldPosition.x, m_flowTarget.y - worldPosition.y};
            const float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (distance <= 0.01f) return {0.0f, 0.0f};
            // Scale the final frame's movement to land on the click without overshooting.
            const float divisor = std::max(distance, maxStep);
            return {delta.x / divisor, delta.y / divisor};
        }

        Vector2 tileDir = m_flowField.GetDirection(tileX, tileY);

        // Unreachable tile: step toward any cheaper neighbor
        if (tileDir.x == 0.0f && tileDir.y == 0.0f &&
            m_flowField.GetCost(tileX, tileY) < 0)
        {
            const int dirs[4][2] = {
                { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }
            };

            int bestCost = INT_MAX;
            for (const auto& d : dirs)
            {
                const int nx = tileX + d[0];
                const int ny = tileY + d[1];
                if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                    ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
                {
                    continue;
                }

                const int cost = m_flowField.GetCost(nx, ny);
                if (cost >= 0 && cost < bestCost)
                {
                    bestCost = cost;
                    tileDir = {
                        static_cast<float>(d[0]),
                        static_cast<float>(d[1])
                    };
                }
            }
        }

        if (tileDir.x == 0.0f && tileDir.y == 0.0f)
            return { 0.0f, 0.0f };

        // Steer toward the center of the next tile — reduces getting
        // hung up on stump corners while only using cardinal tile steps.
        const float nextCenterX =
            (static_cast<float>(tileX) + tileDir.x) * TileSize + TileSize * 0.5f;
        const float nextCenterY =
            (static_cast<float>(tileY) + tileDir.y) * TileSize + TileSize * 0.5f;

        Vector2 steer = {
            nextCenterX - worldPosition.x,
            nextCenterY - worldPosition.y
        };

        const float lenSq = steer.x * steer.x + steer.y * steer.y;
        if (lenSq < 0.0001f)
            return tileDir;

        const float invLen = 1.0f / sqrtf(lenSq);
        return { steer.x * invLen, steer.y * invLen };
    }

    Vector2 World::GetSlideDirection(Vector2 worldPosition, Vector2 blockedDir) const
    {
        int tileX = static_cast<int>(worldPosition.x / TileSize);
        int tileY = static_cast<int>(worldPosition.y / TileSize);

        if (tileX < 0 || tileX >= Chunk::CHUNK_WIDTH ||
            tileY < 0 || tileY >= Chunk::CHUNK_HEIGHT)
        {
            return { 0.0f, 0.0f };
        }

        // Prefer side steps that still go downhill on the cost field
        const Vector2 candidates[4] = {
            { -blockedDir.y, blockedDir.x },
            { blockedDir.y, -blockedDir.x },
            { -blockedDir.x, -blockedDir.y },
            blockedDir
        };

        int bestCost = INT_MAX;
        Vector2 best = { 0.0f, 0.0f };

        for (const Vector2& c : candidates) {
            // Quantize to tile step
            const int dx = (c.x > 0.3f) ? 1 : (c.x < -0.3f ? -1 : 0);
            const int dy = (c.y > 0.3f) ? 1 : (c.y < -0.3f ? -1 : 0);
            if (dx == 0 && dy == 0)
                continue;

            const int nx = tileX + dx;
            const int ny = tileY + dy;
            if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
            {
                continue;
            }

            const int cost = m_flowField.GetCost(nx, ny);
            if (cost >= 0 && cost < bestCost)
            {
                bestCost = cost;
                const float nextCenterX = static_cast<float>(nx) * TileSize + TileSize * 0.5f;
                const float nextCenterY = static_cast<float>(ny) * TileSize + TileSize * 0.5f;
                Vector2 steer = {
                    nextCenterX - worldPosition.x,
                    nextCenterY - worldPosition.y
                };
                const float lenSq = steer.x * steer.x + steer.y * steer.y;
                if (lenSq > 0.0001f)
                {
                    const float invLen = 1.0f / sqrtf(lenSq);
                    best = { steer.x * invLen, steer.y * invLen };
                }
            }
        }
        return best;
    }
}
