#pragma once
#include <random>
#include <algorithm>
#include <cstdint>
#include <array>
#include <cmath>
#include "raylib.h"

namespace World::Generation
{
    class NoiseGenerator
    {
    public:
        NoiseGenerator(uint64_t seed);

        // NEW: Replaces PerlinNoise
        float SimplexNoise2D(float x, float y);
        float FractalBrownianMotion(float x, float y, int octaves, float persistence);
        float  Gradient(int hash, float x, float y);
        
       
    private:

        
        uint64_t m_seed;

    // KEEP: Exactly as it was


        void GeneratePermutation();


        std::array<int, 512> m_permutation;
    };
}
