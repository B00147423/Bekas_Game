#include "ShadowBuffer.h"
#include <rlgl.h>
#include <algorithm>
#include <cmath>

namespace Lighting
{
    ShadowBuffer::ShadowBuffer(){}

    ShadowBuffer::~ShadowBuffer()
    {
        Unload();
    }

    void ShadowBuffer::Init(int width, int height)
    {
        Unload();

        m_width = m_viewWidth = width;
        m_height = m_viewHeight = height;

        m_renderTexture = MakeTarget(width, height);
        m_resolveShader = LoadShader(nullptr, "src/shaders/shadow_resolve.fs");
        if (!IsShaderValid(m_resolveShader)) {
            TraceLog(LOG_WARNING, "Shadow resolve shader unavailable; using hard shadows");
            m_resolveShader = {};
            return;
        }
        m_countLocation = GetShaderLocation(m_resolveShader, "sampleCount");
        m_opacityLocation = GetShaderLocation(m_resolveShader, "opacity");
        m_smoothingLocation = GetShaderLocation(m_resolveShader, "smoothingRadius");
        if (m_countLocation < 0 || m_opacityLocation < 0) {
            TraceLog(LOG_WARNING, "Invalid shadow resolve uniforms; using hard shadows");
            UnloadShader(m_resolveShader);
            m_resolveShader = {};
            return;
        }
        m_textureLocations = {GetShaderLocation(m_resolveShader, "samples1"),
            GetShaderLocation(m_resolveShader, "samples2"), GetShaderLocation(m_resolveShader, "samples3")};
    }

    void ShadowBuffer::Begin()
    {
        BeginTextureMode(m_renderTexture);
        ClearBackground(WHITE);
        LogicalProjection();
    }

    void ShadowBuffer::End()
    {
        EndTextureMode();
    }

    RenderTexture2D ShadowBuffer::MakeTarget(int width,int height) {
        RenderTexture2D result{};
        result.id=rlLoadFramebuffer();
        if(!result.id) return {};
        result.texture={rlLoadTexture(nullptr,width,height,RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,1),
            width,height,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        rlEnableFramebuffer(result.id);
        rlFramebufferAttach(result.id,result.texture.id,RL_ATTACHMENT_COLOR_CHANNEL0,RL_ATTACHMENT_TEXTURE2D,0);
        bool valid=rlFramebufferComplete(result.id);
        rlDisableFramebuffer();
        if(!valid) { UnloadRenderTexture(result); return {}; }
        SetTextureFilter(result.texture,TEXTURE_FILTER_POINT);
        SetTextureWrap(result.texture,TEXTURE_WRAP_CLAMP);
        return result; // Shadows are 2D: no unused depth attachment.
    }

    void ShadowBuffer::ReleaseTargets() {
        if(m_renderTexture.id) UnloadRenderTexture(m_renderTexture);
        m_renderTexture={};
        for(auto& sample:m_samples) {
            if(sample.id) UnloadRenderTexture(sample);
            sample={};
        }
    }

    void ShadowBuffer::Resize(int width,int height,int viewWidth,int viewHeight) {
        if(width<=0 || height<=0 || viewWidth<=0 || viewHeight<=0) return;
        if(!m_resolveShader.id && !m_renderTexture.id) Init(width,height);
        if(width!=m_width || height!=m_height) {
            ReleaseTargets();
            m_width=width; m_height=height;
            m_renderTexture=MakeTarget(width,height);
        }
        m_viewWidth=viewWidth; m_viewHeight=viewHeight;
    }

    void ShadowBuffer::LogicalProjection() const {
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlOrtho(0,m_viewWidth,m_viewHeight,0,0,1);
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
    }

    void ShadowBuffer::BeginScissor(Rectangle bounds) const {
        const float sx=(float)m_width/m_viewWidth, sy=(float)m_height/m_viewHeight;
        int left=std::clamp((int)std::floor(bounds.x*sx),0,m_width);
        int top=std::clamp((int)std::floor(bounds.y*sy),0,m_height);
        int right=std::clamp((int)std::ceil((bounds.x+bounds.width)*sx),0,m_width);
        int bottom=std::clamp((int)std::ceil((bounds.y+bounds.height)*sy),0,m_height);
        rlDrawRenderBatchActive();
        rlEnableScissorTest();
        rlScissor(left,m_height-bottom,right-left,bottom-top);
    }

    void ShadowBuffer::EndScissor() const {
        rlDrawRenderBatchActive();
        rlDisableScissorTest();
    }

    void ShadowBuffer::Unload() {
        ReleaseTargets();
        if(m_resolveShader.id) UnloadShader(m_resolveShader);
        m_resolveShader={};
        m_width=m_height=m_viewWidth=m_viewHeight=0;
    }

    bool ShadowBuffer::PrepareSamples(int count)
    {
        if (!m_resolveShader.id || (count != 1 && count != 4 && count != 8 && count != 16)) return false;
        // Release higher quality targets when the player lowers sample count.
        for(int i=(count+3)/4;i<4;++i) {
            if(m_samples[i].id) UnloadRenderTexture(m_samples[i]);
            m_samples[i]={};
        }
        for (int i = 0; i < (count + 3) / 4; ++i) {
            if (!m_samples[i].id) m_samples[i] = MakeTarget(m_width, m_height);
            if (!m_samples[i].id || !m_samples[i].texture.id) return false;
        }
        return true;
    }

    Color ShadowBuffer::SampleColor(int index)
    {
        Color color = WHITE;
        switch (index % 4) {
            case 0: color.r = 0; break;
            case 1: color.g = 0; break;
            case 2: color.b = 0; break;
            case 3: color.a = 0; break;
        }
        return color;
    }

    Vector2 ShadowBuffer::SampleOffset(int index, int count, float radius)
    {
        if (count <= 1 || radius <= 0) return {};
        // Equal-area radial strata with antipodal pairs: stable, centered disk
        // samples in world space. No frame jitter or temporal history required.
        const int pair = index / 2;
        const float r = radius * std::sqrt((pair + 0.5f) / (count / 2));
        const float angle = pair * 2.39996323f + (index % 2) * PI;
        return {r * std::cos(angle), r * std::sin(angle)};
    }

    void ShadowBuffer::BeginSample(int index)
    {
        BeginTextureMode(m_samples[index / 4]);
        if (index % 4 == 0) ClearBackground(WHITE);
        LogicalProjection();
        rlSetBlendFactors(RL_ONE, RL_ONE, RL_MIN);
        BeginBlendMode(BLEND_CUSTOM);
    }

    void ShadowBuffer::EndSample()
    {
        EndBlendMode();
        EndTextureMode();
    }

    void ShadowBuffer::Resolve(int count, float opacity, Rectangle screenBounds, float smoothingRadius)
    {
        opacity = std::clamp(opacity, 0.0f, 1.0f);
        Begin();
        BeginScissor(screenBounds);
        BeginShaderMode(m_resolveShader);
        SetShaderValue(m_resolveShader, m_countLocation, &count, SHADER_UNIFORM_INT);
        SetShaderValue(m_resolveShader, m_opacityLocation, &opacity, SHADER_UNIFORM_FLOAT);
        smoothingRadius=std::max(0.0f,smoothingRadius);
        SetShaderValue(m_resolveShader, m_smoothingLocation, &smoothingRadius, SHADER_UNIFORM_FLOAT);
        for (int i = 1; i < (count + 3) / 4; ++i)
            SetShaderValueTexture(m_resolveShader, m_textureLocations[i-1], m_samples[i].texture);
        DrawTexturePro(m_samples[0].texture, {0, 0, (float)m_width, -(float)m_height},
            {0,0,(float)m_viewWidth,(float)m_viewHeight},{0,0},0,WHITE);
        EndShaderMode();
        EndScissor();
        End();
    }

    Texture2D ShadowBuffer::GetTexture() const
    {
        return m_renderTexture.texture;
    }
}