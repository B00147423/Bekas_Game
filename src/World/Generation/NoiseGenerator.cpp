#include "NoiseGenerator.h"

namespace World::Generation
{
    namespace
    {
        inline float GradientHash(int hash, float x, float y)
        {
            int h = hash & 3;
            float gx = (h == 0 || h == 3) ? 1.0f : -1.0f;
            float gy = (h == 0 || h == 1) ? 1.0f : -1.0f;
            return gx * x + gy * y;
        }
    }

    NoiseGenerator::NoiseGenerator(uint64_t seed)
        : m_seed(seed)
    {
        GeneratePermutation(); // KEEP
    }
    
    // KEEP: Exactly as it was
    float NoiseGenerator::Gradient(int hash, float x, float y)
    {
        return GradientHash(hash, x, y);
    }

    // NEW: Replaces PerlinNoise
    float NoiseGenerator::SimplexNoise2D(float x, float y)
    {
        const float F2 = 0.366025403f;
        const float G2 = 0.211324865f;

        float s = (x + y) * F2;
        int i = static_cast<int>(floorf(x + s));
        int j = static_cast<int>(floorf(y + s));

        float t = (i + j) * G2;
        float X0 = i - t;
        float Y0 = j - t;
        float x0 = x - X0;
        float y0 = y - Y0;

        int i1, j1; 
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else         { i1 = 0; j1 = 1; }

        float x1 = x0 - i1 + G2;
        float y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2;
        float y2 = y0 - 1.0f + 2.0f * G2;

        int ii = i & 255;
        int jj = j & 255;

        int gi0 = m_permutation[ii + m_permutation[jj]];
        int gi1 = m_permutation[ii + i1 + m_permutation[jj + j1]];
        int gi2 = m_permutation[ii + 1 + m_permutation[jj + 1]];

        float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;

        float t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 >= 0.0f) {
            t0 *= t0;
            n0 = t0 * t0 * Gradient(gi0, x0, y0);
        }

        float t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 >= 0.0f) {
            t1 *= t1;
            n1 = t1 * t1 * Gradient(gi1, x1, y1);
        }

        float t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 >= 0.0f) {
            t2 *= t2;
            n2 = t2 * t2 * Gradient(gi2, x2, y2);
        }

        return 70.0f * (n0 + n1 + n2);
    }


    // KEEP: Exactly as it was
    void NoiseGenerator::GeneratePermutation()
    {
        std::array<int, 256> values;
        for(int i = 0; i < 256; i++) { values[i] = i; }

        std::mt19937 rng(m_seed);
        std::shuffle(values.begin(), values.end(), rng);

        for(int i = 0; i < 512; i++) {
            m_permutation[i] = values[i % 256];
        }
    }

    // UPDATED: Now calls SimplexNoise2D
    float NoiseGenerator::FractalBrownianMotion(float x, float y, int octaves, float persistence)
    {
        float total = 0;
        float frequency = 1;
        float amplitude = 1;
        float maxValue = 0;

        for(int i = 0; i < octaves; i++)
        {
            // Swapped PerlinNoise for SimplexNoise2D here:
            total += SimplexNoise2D(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2;
        }

        return total / maxValue;
    }
}
