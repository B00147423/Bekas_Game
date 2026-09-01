#pragma once

#include <cstdint>

namespace World
{
    enum TileType
    {
        Air,
        Grass,
        Dirt,
        Stone,
        Water,
        Tree,
        CoalOre,
        IronOre,
    };

    enum BiomeType
    {
        Plains,
        Forest,
    };

    struct Tile
    {
        TileType type = TileType::Air;
        BiomeType biome = BiomeType::Plains;

        bool solid = false;
        uint8_t metadata = 0;
    };

}