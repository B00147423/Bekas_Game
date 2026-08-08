#include "Tileset.h"

namespace World
{
    namespace
    {
        Texture2D LoadTextureKeyBlack(const char* path)
        {
            Image image = LoadImage(path);
            if (image.data == nullptr)
                return Texture2D{};

            // Trees_Flowers.png uses solid black as empty space
            ImageColorReplace(&image, BLACK, BLANK);
            Texture2D texture = LoadTextureFromImage(image);
            UnloadImage(image);
            return texture;
        }
    }

    Tileset::~Tileset()
    {
        if (m_grassTexture.id != 0)
            UnloadTexture(m_grassTexture);
        if (m_foliageTexture.id != 0)
            UnloadTexture(m_foliageTexture);
    }

    bool Tileset::Load(const std::string& grassPath, const std::string& foliagePath)
    {
        m_grassTexture = LoadTexture(grassPath.c_str());
        m_foliageTexture = LoadTextureKeyBlack(foliagePath.c_str());

        if (m_grassTexture.id == 0 || m_foliageTexture.id == 0)
            return false;

        // 16x16 grass tiles in region (608, 880, 48, 64) — 3 wide x 4 tall
        m_grassVariants.clear();

        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 8; col++)
            {
                m_grassVariants.push_back(Rectangle{
                    608.0f + static_cast<float>(col) * 16.0f,
                    816.0f + static_cast<float>(row) * 16.0f,
                    16.0f,
                    16.0f
                });
            }
        }

        // Trees_Flowers.png — pine / evergreen
        m_pineTree = { 116.0f, 25.0f, 24.0f, 39.0f };

        SetTextureFilter(m_grassTexture, TEXTURE_FILTER_POINT);
        SetTextureFilter(m_foliageTexture, TEXTURE_FILTER_POINT);

        return true;
    }

    const Texture2D& Tileset::GetGrassTexture() const
    {
        return m_grassTexture;
    }

    const Texture2D& Tileset::GetFoliageTexture() const
    {
        return m_foliageTexture;
    }

    Rectangle Tileset::GetGrass(int variant) const
    {
        if (m_grassVariants.empty())
            return Rectangle{};

        const int count = static_cast<int>(m_grassVariants.size());
        const int index = ((variant % count) + count) % count;
        return m_grassVariants[index];
    }

    Rectangle Tileset::GetPineTree() const
    {
        return m_pineTree;
    }
}
