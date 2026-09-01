#include "Tileset.h"

#include <unordered_set>

namespace World{
    struct MaskCell{
        uint8_t mask;
        int column;
        int row;
    };

    constexpr MaskCell kForestMaskCells[] ={
        // Isolated
        {   0,  0, 0 },

        // End caps
        {   1,  1, 0 },
        {   4,  2, 0 },
        {  16,  3, 0 },
        {  64,  4, 0 },

        // Straight pipes
        {  17,  5, 0 },
        {  68,  6, 0 },

        // L corners
        {   5,  0, 1 },
        {   7,  1, 1 },
        {  20,  2, 1 },
        {  28,  3, 1 },
        {  65,  4, 1 },
        {  80,  5, 1 },
        { 112,  6, 1 },
        { 193,  7, 1 },

        // T shapes — row 1
        {  21,  8, 1 },
        {  23,  9, 1 },
        {  29, 10, 1 },
        {  31, 11, 1 },

        // T shapes — row 2
        {  69,  0, 2 },
        {  71,  1, 2 },
        {  81,  2, 2 },
        {  84,  3, 2 },
        {  92,  4, 2 },
        { 113,  5, 2 },
        { 116,  6, 2 },
        { 124,  7, 2 },
        { 197,  8, 2 },
        { 199,  9, 2 },
        { 209, 10, 2 },
        { 241, 11, 2 },

        // Surrounded / centre — row 3
        {  85,  0, 3 },
        {  87,  1, 3 },
        {  93,  2, 3 },
        {  95,  3, 3 },
        { 117,  4, 3 },
        { 119,  5, 3 },
        { 125,  6, 3 },
        { 127,  7, 3 },
        { 213,  8, 3 },

        // Surrounded / centre — row 4
        { 215,  0, 4 },
        { 221,  1, 4 },
        { 223,  2, 4 },
        { 245,  3, 4 },
        { 247,  4, 4 },
        { 253,  5, 4 },
        { 255,  6, 4 }
    };
    namespace{
        Texture2D LoadTextureKeyBlack(const char* path) {
            Image image = LoadImage(path);
            if (image.data == nullptr)
                return Texture2D{};

            // Trees_Flowers.png uses solid black as empty space
            ImageColorReplace(&image, BLACK, BLANK);
            Texture2D texture = LoadTextureFromImage(image);
            UnloadImage(image);
            return texture;
        }

        // 8x8 block at (608,816) in grass_top_layer.png — flowers / leafy props
        bool IsRareGrassIndex(int index) {
            static const std::unordered_set<int> kRare = {
                8,   // pink flower
                18,  // leaf cluster
                26,  // white daisy
                27,  // leaves
                28,  // leaves / purple
                35,  // small pink / daisy
                36,  // red flower
                63   // scattered leaves
            };
            return kRare.find(index) != kRare.end();
        }
    }

    Tileset::~Tileset(){
        if (m_grassTexture.id != 0)
            UnloadTexture(m_grassTexture);
        if (m_foliageTexture.id != 0)
            UnloadTexture(m_foliageTexture);
        if (m_forestTexture.id != 0)
        {
            UnloadTexture(m_forestTexture);
        }
    }

    bool Tileset::Load(const std::string& grassPath, const std::string& foliagePath, const std::string& forestPath){
        m_grassTexture = LoadTexture(grassPath.c_str());
        m_foliageTexture = LoadTextureKeyBlack(foliagePath.c_str());
        m_forestTexture = LoadTexture(forestPath.c_str());
        
        TraceLog(
            LOG_WARNING,
            "FOREST: id=%u width=%d height=%d",
            m_forestTexture.id,
            m_forestTexture.width,
            m_forestTexture.height
        );

        if (m_foliageTexture.id == 0 || m_forestTexture.id == 0)
            return false;


        std::fill(
            m_forestByMask.begin(),
            m_forestByMask.end(),
            Rectangle{0.0f, 0.0f, 16.0f, 16.0f}
        );

        for(const MaskCell& cell : kForestMaskCells){
            m_forestByMask[cell.mask] = {
                static_cast<float>(cell.column * 16),
                static_cast<float>(cell.row * 16),
                16.0f,
                16.0f
            };
        }
        m_grassCommon.clear();
        m_grassRare.clear();

        for (int row = 0; row < 8; row++){
            for (int col = 0; col < 8; col++){
                const int index = row * 8 + col;
                const Rectangle rect = {
                    608.0f + static_cast<float>(col) * 16.0f,
                    816.0f + static_cast<float>(row) * 16.0f,
                    16.0f,
                    16.0f
                };

                if (IsRareGrassIndex(index))
                    m_grassRare.push_back(rect);
                else
                    m_grassCommon.push_back(rect);
            }
        }

        // Trees_Flowers.png — pine / evergreen
        m_pineTree = { 116.0f, 25.0f, 24.0f, 39.0f };

        SetTextureFilter(m_grassTexture, TEXTURE_FILTER_POINT);
        SetTextureFilter(m_foliageTexture, TEXTURE_FILTER_POINT);
        if (m_forestTexture.id != 0)
        {
            SetTextureFilter(
                m_forestTexture,
                TEXTURE_FILTER_POINT
            );
        }
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

    const Texture2D& Tileset::GetForestTexture() const
    {
        return m_forestTexture;
    }

    Rectangle Tileset::GetForest(uint8_t mask) const
    {
        return m_forestByMask[mask];
    }

    Rectangle Tileset::GetGrass(int variant) const{
        if (m_grassCommon.empty())
            return Rectangle{};

        // Stable per-tile: low chance of flower/plant, else plain grass
        constexpr int kRareChancePercent = 6;
        const int roll = ((variant % 100) + 100) % 100;

        if (!m_grassRare.empty() && roll < kRareChancePercent)
        {
            const int count = static_cast<int>(m_grassRare.size());
            const int index = ((variant / 100) % count + count) % count;
            return m_grassRare[index];
        }

        const int count = static_cast<int>(m_grassCommon.size());
        const int index = ((variant % count) + count) % count;
        return m_grassCommon[index];
    }

    Rectangle Tileset::GetPineTree() const
    {
        return m_pineTree;
    }
}
