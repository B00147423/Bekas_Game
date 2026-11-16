#pragma once
#include <raylib.h>
#include "Lighting/ShadowRenderer2D.h"
#include "GameConfig.h"
#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
namespace World
{
    class Tileset
    {
    public:
        Tileset() = default;
        ~Tileset();

        Tileset(const Tileset&) = delete;
        Tileset& operator=(const Tileset&) = delete;

        bool Load(const std::string& grassPath, const std::string& foliagePat, const std::string& forestPath, Lighting::ShadowRenderer2D& shadows);

        // Weighted: mostly plain grass, rare flowers/plants
        Rectangle GetGrass(int variant) const;
        Rectangle GetPineTree() const;
        Rectangle GetForest(uint8_t mask) const;
        Rectangle GetTree(int index) const;
        const Texture2D& GetForestTexture() const;
        const Texture2D& GetGrassTexture() const;
        const Texture2D& GetFoliageTexture() const;
        const Texture2D& GetSettlementTexture() const;
        const Texture2D& GetTreeAtlasTexture() const;   
  
    private:
        Texture2D m_grassTexture{};
        Texture2D m_foliageTexture{};
        Texture2D m_forestTexture{};
        Texture2D m_settlementTexture{};
        Texture2D m_treeAtlasTexture{};
        std::array<Rectangle, 256> m_forestByMask{};
        std::vector<Rectangle> m_grassCommon;
        std::vector<Rectangle> m_grassRare;
        Rectangle m_pineTree{};
        
    };
}
