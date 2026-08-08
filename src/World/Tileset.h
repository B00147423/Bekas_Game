#pragma once

#include <raylib.h>
#include <string>
#include <vector>

namespace World
{
    class Tileset
    {
    public:
        Tileset() = default;
        ~Tileset();

        Tileset(const Tileset&) = delete;
        Tileset& operator=(const Tileset&) = delete;

        bool Load(const std::string& grassPath, const std::string& foliagePath);

        // Picks one of the grass variants (wraps with %)
        Rectangle GetGrass(int variant) const;
        Rectangle GetPineTree() const;

        const Texture2D& GetGrassTexture() const;
        const Texture2D& GetFoliageTexture() const;

    private:
        Texture2D m_grassTexture{};
        Texture2D m_foliageTexture{};

        std::vector<Rectangle> m_grassVariants;
        Rectangle m_pineTree{};
    };
}
