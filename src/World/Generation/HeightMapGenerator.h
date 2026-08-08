// #pragma once

// #include <cstdint>
// #include <vector>
// #include "NoiseGenerator.h"

// namespace World::Generation
// {
//     class HeightMapGenerator
//     {
//     public:
//         HeightMapGenerator(uint64_t seed);

//         std::vector<int> GenerateHeightMap(int width);

//     private:
//         void SmoothHeightMap(std::vector<int>& heightMap, int passes);

//         NoiseGenerator m_noise;

//         float m_frequency = 0.04f;
//         int m_baseHeight = 15;
//         int m_amplitude = 25;
//     };
// }
