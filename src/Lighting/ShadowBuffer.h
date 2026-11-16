#pragma once

#include <raylib.h>
#include <array>

namespace Lighting
{
    class ShadowBuffer
    {
    public:
        ShadowBuffer();
        ~ShadowBuffer();
        ShadowBuffer(const ShadowBuffer&) = delete;
        ShadowBuffer& operator=(const ShadowBuffer&) = delete;

        void Init(int width, int height);
        void Resize(int width, int height, int viewWidth, int viewHeight);
        void BeginScissor(Rectangle logicalBounds) const;
        void EndScissor() const;
        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        void Begin();
        void End();
        void Unload();

        Texture2D GetTexture() const;

        // RGBA channels hold independent binary visibility samples. MIN blending
        // unions all occluders per sample, so overlaps never darken twice.
        bool PrepareSamples(int count);
        void BeginSample(int index);
        void EndSample();
        void Resolve(int count, float opacity, Rectangle screenBounds, float smoothingRadius = 4.0f);
        static Color SampleColor(int index);
        static Vector2 SampleOffset(int index, int count, float radius);


    private:
        RenderTexture2D m_renderTexture{};

        std::array<RenderTexture2D, 4> m_samples{};
        Shader m_resolveShader{};
        int m_countLocation = -1;
        int m_opacityLocation = -1;
        int m_smoothingLocation = -1;
        std::array<int, 3> m_textureLocations{};
        int m_width = 0;
        int m_height = 0;
        int m_viewWidth = 0, m_viewHeight = 0;
        void ReleaseTargets();
        void LogicalProjection() const;
        static RenderTexture2D MakeTarget(int width, int height);
    };
}