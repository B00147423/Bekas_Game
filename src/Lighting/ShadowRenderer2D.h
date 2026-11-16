
#pragma once

#include <raylib.h>
#include <map>
#include <tuple>
#include <vector>
#include "ShadowStyle.h"

namespace Lighting
{
    class ShadowRenderer2D
    {
    public:
        // Call at asset load time, before releasing the original CPU image.
        void CacheShape(Texture2D texture, const Image& image, Rectangle source = {});
        ShadowRenderer2D() = default;
        ~ShadowRenderer2D();
        ShadowRenderer2D(const ShadowRenderer2D&) = delete;
        ShadowRenderer2D& operator=(const ShadowRenderer2D&) = delete;
        void Clear();
        // Collect once per light. Cached local edges + per-caster transforms are
        // replayed for every area-light sample without CPU extrusion or uploads.
        bool BeginCollection();
        void EndCollection();
        void DrawBatch(Vector2 lightPosition, float shadowLength, Color maskColor);
        struct Stats { unsigned int casters=0, drawCalls=0, uploadBytes=0; };
        const Stats& GetStats() const { return m_stats; }
        void DrawCaster(Texture2D texture, Rectangle source, Rectangle destination,
            const ShadowStyle& style, Vector2 lightPosition, float shadowLength,
            Color maskColor = BLACK);
        void Draw(
            Texture2D texture,
            Rectangle source,
            Rectangle destination,
            Vector2 lightPosition,
            float shadowLength,
            Color maskColor = BLACK
        );

    private:
        struct Edge
        {
            Vector2 a;
            Vector2 b;
        };

        using ShapeKey = std::tuple<unsigned int, int, int, int, int>;

        struct Shape {
            std::vector<Edge> edges;
            unsigned int vao=0, vertices=0, instances=0;
            int capacity=0;
            std::vector<Vector4> queued, uploaded;
        };
        std::map<ShapeKey, Shape> m_cache;
        using FootprintKey = std::tuple<float, float, float, float>;
        std::map<FootprintKey, Shape> m_footprints;
        Shader m_shader{};
        int m_mvp=-1, m_light=-1, m_length=-1, m_color=-1, m_sourceSize=-1;
        int m_propMode=-1, m_propSettings=-1, m_referenceDest=-1;
        bool m_collecting=false;
        bool m_shaderAttempted=false;
        Stats m_stats{};
        void CreateGpuShape(Shape& shape);
        bool EnsureShader();
        void UploadInstances(Shape& shape);
        void DrawFootprint(Rectangle base, const ShadowStyle& style, Vector2 light, Color color);
        static bool ContainsLight(const Shape& shape, Vector4 destination,
            Vector2 sourceSize, Vector2 lightPosition);

        const std::vector<Edge>& GetEdges(
            Texture2D texture,
            Rectangle source
        );

        std::vector<Edge> GenerateEdges(
            const Image& image,
            Rectangle source
        );
    };
}
