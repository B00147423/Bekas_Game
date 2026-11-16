#pragma once
#include "Tile.h"
#include "GameConfig.h"
#include <array>
#include <functional>
#include <map>
#include <set>
#include <vector>

namespace World
{
    class Chunk
    {
    public:
        Chunk();

        Tile GetTile(int x, int y) const;
        void SetTile(int x, int y, const Tile& tile);

        // Large worlds allocate and generate only the 32x32 pages they visit.
        void SetGenerator(std::function<Tile(int,int)> generator);
        std::vector<Vector2> GetWaterTiles() const;
        size_t LoadedPageCount() const { return m_pages.size(); }
        static constexpr int PageSize = 32;
        static constexpr int CHUNK_WIDTH = GameConfig::World::ChunkWidth;
        static constexpr int CHUNK_HEIGHT = GameConfig::World::ChunkHeight;
    private:

        using Page = std::array<Tile, PageSize * PageSize>;
        Page& GetPage(int x, int y) const;
        mutable std::map<int, Page> m_pages;
        std::function<Tile(int,int)> m_generator;
        std::set<int> m_waterTiles;

    };
}
