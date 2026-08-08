// #include "HeightMapGenerator.h"

// namespace World::Generation
// {
//     HeightMapGenerator::HeightMapGenerator(uint64_t seed)
//         : m_noise(seed)
//     {
//     }

//     std::vector<int> HeightMapGenerator::GenerateHeightMap(int width)
//     {
//         std::vector<int> heightMap;
//         heightMap.reserve(width);

//         for (int x = 0; x < width; x++)
//         {
//             float noiseValue = m_noise.FractalBrownianMotion(
//                 x * m_frequency,
//                 0.0f,
//                 5,
//                 0.5f
//             );

//             float normalized = (noiseValue + 1.0f) / 2.0f;

//             int height =
//                 m_baseHeight +
//                 static_cast<int>(normalized * m_amplitude);

//             heightMap.push_back(height);
//         }

//         return heightMap;
//     }


// }
