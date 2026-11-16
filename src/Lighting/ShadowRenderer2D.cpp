#include "ShadowRenderer2D.h"

#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <utility>
#include <array>
// Do not include <GL/gl.h> here: on Windows it imports Win32 declarations
// named Rectangle, CloseWindow and ShowCursor that conflict with raylib.
// The modern GL entry points below are loaded using rlGetProcAddress().

namespace {
using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLfloat = float;
using GLboolean = unsigned char;

#if defined(_WIN32)
#define SHADOW_GL_CALL __stdcall
#else
#define SHADOW_GL_CALL
#endif

constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
constexpr GLenum GL_STATIC_DRAW = 0x88E4;
constexpr GLenum GL_DYNAMIC_DRAW = 0x88E8;
constexpr GLenum GL_FLOAT = 0x1406;
constexpr GLenum GL_TRIANGLES = 0x0004;
constexpr GLboolean GL_FALSE = 0;

// Modern OpenGL entry points are resolved through raylib's active context.
// This avoids depending on a second GL loader such as GLEW/GLAD.
using GenVertexArraysFn = void (SHADOW_GL_CALL *)(GLsizei, GLuint*);
using BindVertexArrayFn = void (SHADOW_GL_CALL *)(GLuint);
using DeleteVertexArraysFn = void (SHADOW_GL_CALL *)(GLsizei, const GLuint*);
using GenBuffersFn = void (SHADOW_GL_CALL *)(GLsizei, GLuint*);
using BindBufferFn = void (SHADOW_GL_CALL *)(GLenum, GLuint);
using BufferDataFn = void (SHADOW_GL_CALL *)(GLenum, std::ptrdiff_t, const void*, GLenum);
using BufferSubDataFn = void (SHADOW_GL_CALL *)(GLenum, std::ptrdiff_t, std::ptrdiff_t, const void*);
using DeleteBuffersFn = void (SHADOW_GL_CALL *)(GLsizei, const GLuint*);
using EnableVertexAttribArrayFn = void (SHADOW_GL_CALL *)(GLuint);
using VertexAttribPointerFn = void (SHADOW_GL_CALL *)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
using VertexAttribDivisorFn = void (SHADOW_GL_CALL *)(GLuint, GLuint);
using DrawArraysInstancedFn = void (SHADOW_GL_CALL *)(GLenum, GLint, GLsizei, GLsizei);
using UseProgramFn = void (SHADOW_GL_CALL *)(GLuint);
using UniformMatrix4fvFn = void (SHADOW_GL_CALL *)(GLint, GLsizei, GLboolean, const GLfloat*);
using Uniform2fFn = void (SHADOW_GL_CALL *)(GLint, GLfloat, GLfloat);
using Uniform1fFn = void (SHADOW_GL_CALL *)(GLint, GLfloat);
using Uniform4fFn = void (SHADOW_GL_CALL *)(GLint, GLfloat, GLfloat, GLfloat, GLfloat);

struct Api {
    GenVertexArraysFn genVAO{};
    BindVertexArrayFn bindVAO{};
    DeleteVertexArraysFn deleteVAO{};
    GenBuffersFn genBuffers{};
    BindBufferFn bindBuffer{};
    BufferDataFn bufferData{};
    BufferSubDataFn bufferSubData{};
    DeleteBuffersFn deleteBuffers{};
    EnableVertexAttribArrayFn enableAttrib{};
    VertexAttribPointerFn attribPointer{};
    VertexAttribDivisorFn attribDivisor{};
    DrawArraysInstancedFn drawInstanced{};
    UseProgramFn useProgram{};
    UniformMatrix4fvFn uniformMatrix{};
    Uniform2fFn uniform2{};
    Uniform1fFn uniform1{};
    Uniform4fFn uniform4{};
    bool ready = false;

    template<class T> static T load(const char* name) {
        return reinterpret_cast<T>(rlGetProcAddress(name));
    }
    bool init() {
        genVAO=load<GenVertexArraysFn>("glGenVertexArrays");
        bindVAO=load<BindVertexArrayFn>("glBindVertexArray");
        deleteVAO=load<DeleteVertexArraysFn>("glDeleteVertexArrays");
        genBuffers=load<GenBuffersFn>("glGenBuffers");
        bindBuffer=load<BindBufferFn>("glBindBuffer");
        bufferData=load<BufferDataFn>("glBufferData");
        bufferSubData=load<BufferSubDataFn>("glBufferSubData");
        deleteBuffers=load<DeleteBuffersFn>("glDeleteBuffers");
        enableAttrib=load<EnableVertexAttribArrayFn>("glEnableVertexAttribArray");
        attribPointer=load<VertexAttribPointerFn>("glVertexAttribPointer");
        attribDivisor=load<VertexAttribDivisorFn>("glVertexAttribDivisor");
        drawInstanced=load<DrawArraysInstancedFn>("glDrawArraysInstanced");
        useProgram=load<UseProgramFn>("glUseProgram");
        uniformMatrix=load<UniformMatrix4fvFn>("glUniformMatrix4fv");
        uniform2=load<Uniform2fFn>("glUniform2f");
        uniform1=load<Uniform1fFn>("glUniform1f");
        uniform4=load<Uniform4fFn>("glUniform4f");
        ready=genVAO && bindVAO && deleteVAO && genBuffers && bindBuffer &&
            bufferData && bufferSubData && deleteBuffers && enableAttrib &&
            attribPointer && attribDivisor && drawInstanced && useProgram &&
            uniformMatrix && uniform2 && uniform1 && uniform4;
        return ready;
    }
};
Api& glapi() { static Api api; return api; }

// Six vertices per 2D edge = two shadow-volume triangles.
// Local endpoints and a vertex index are streamed once at asset-load time.
struct Vertex { float ax, ay, bx, by, corner; };



// The CPU reference clips homogeneous shadow quads before dividing by w.
// This avoids enormous world coordinates for lights almost on an edge.
struct ClipPoint { double x,y,w; };
Rectangle viewBounds() {
    Matrix inverse=MatrixInvert(MatrixMultiply(rlGetMatrixModelview(),rlGetMatrixProjection()));
    Vector3 p=Vector3Transform({-1,-1,0},inverse);
    float left=p.x,right=p.x,top=p.y,bottom=p.y;
    for(Vector3 corner : {Vector3{1,-1,0},Vector3{-1,1,0},Vector3{1,1,0}}) {
        p=Vector3Transform(corner,inverse);
        left=std::min(left,p.x);right=std::max(right,p.x);
        top=std::min(top,p.y);bottom=std::max(bottom,p.y);
    }
    return {left,top,right-left,bottom-top};
}
void drawClippedQuad(const std::array<ClipPoint,4>& quad,Rectangle bounds,Color color) {
    std::array<ClipPoint,12> input{},output{};
    std::copy(quad.begin(),quad.end(),input.begin());int count=4;
    auto plane=[&](ClipPoint p,int side) {
        switch(side) {
            case 0:return p.x-bounds.x*p.w;
            case 1:return (bounds.x+bounds.width)*p.w-p.x;
            case 2:return p.y-bounds.y*p.w;
            default:return (bounds.y+bounds.height)*p.w-p.y;
        }
    };
    for(int side=0;side<4 && count>0;++side) {
        int n=0;ClipPoint prev=input[count-1];double fp=plane(prev,side);
        for(int i=0;i<count;++i) {
            ClipPoint next=input[i];double fn=plane(next,side);
            if((fp>=0)!=(fn>=0)) {
                double t=fp/(fp-fn);
                output[n++]={prev.x+(next.x-prev.x)*t,prev.y+(next.y-prev.y)*t,prev.w+(next.w-prev.w)*t};
            }
            if(fn>=0) output[n++]=next;
            prev=next;fp=fn;
        }
        count=n;input=output;
    }
    auto point=[](ClipPoint p){return Vector2{float(p.x/p.w),float(p.y/p.w)};};
    if(count<3) return;
    for(int i=1;i<count-1;++i) {
        Vector2 a=point(input[0]),b=point(input[i]),c=point(input[i+1]);
        float cross=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
        if(std::abs(cross)<0.00001f) continue;
        if(cross>0) std::swap(b,c);
        DrawTriangle(a,b,c,color);
    }
}

bool equalTransforms(const std::vector<Vector4>& a,const std::vector<Vector4>& b) {
    if (a.size()!=b.size()) return false;
    for (size_t i=0;i<a.size();++i) {
        if (a[i].x!=b[i].x || a[i].y!=b[i].y || a[i].z!=b[i].z || a[i].w!=b[i].w) return false;
    }
    return true;
}
}

namespace Lighting {
ShadowRenderer2D::~ShadowRenderer2D() { Clear(); }
void ShadowRenderer2D::Clear() {
    auto& g=glapi();
    if (g.ready) for (auto& entry:m_cache) {
        auto& s=entry.second;
        if (s.vao) g.deleteVAO(1,&s.vao);
        if (s.vertices) g.deleteBuffers(1,&s.vertices);
        if (s.instances) g.deleteBuffers(1,&s.instances);
    }
    if (g.ready) for (auto& [key,s]:m_footprints) {
        if (s.vao) g.deleteVAO(1,&s.vao);
        if (s.vertices) g.deleteBuffers(1,&s.vertices);
        if (s.instances) g.deleteBuffers(1,&s.instances);
    }
    m_footprints.clear();
    m_cache.clear();
    if (m_shader.id) UnloadShader(m_shader);
    m_shader={}; m_shaderAttempted=false; m_collecting=false; m_stats={};
}

const std::vector<ShadowRenderer2D::Edge>&
ShadowRenderer2D::GetEdges(Texture2D texture, Rectangle source) {
    ShapeKey key{texture.id,int(source.x),int(source.y),int(source.width),int(source.height)};
    auto it=m_cache.find(key);
    if (it!=m_cache.end()) return it->second.edges;
    static const std::vector<Edge> empty;
    return empty;
}

void ShadowRenderer2D::CacheShape(Texture2D texture,const Image& image, Rectangle source) {
    if (!texture.id || !image.data) return;
    if (source.width==0 || source.height==0) source={0,0,float(image.width),float(image.height)};
    ShapeKey key{texture.id,int(source.x),int(source.y),int(source.width),int(source.height)};
    auto& g=glapi();
    auto& s=m_cache[key];
    if (g.ready) {
        if (s.vao) g.deleteVAO(1,&s.vao);
        if (s.vertices) g.deleteBuffers(1,&s.vertices);
        if (s.instances) g.deleteBuffers(1,&s.instances);
    }
    s=Shape{};
    s.edges=GenerateEdges(image,source);
}

std::vector<ShadowRenderer2D::Edge> ShadowRenderer2D::GenerateEdges(const Image& image,Rectangle source) {
    std::vector<Edge> edges;
    int sx=int(source.x), sy=int(source.y), w=int(source.width), h=int(source.height);
    if (!image.data || sx<0 || sy<0 || w<=0 || h<=0 || sx+w>image.width || sy+h>image.height) return edges;
    Color* pixels=LoadImageColors(image);
    if (!pixels) return edges;
    auto solid=[&](int x,int y) {
        return x>=0 && x<w && y>=0 && y<h && pixels[(sy+y)*image.width+sx+x].a>=128;
    };
    for (int y=0;y<=h;++y) for (int side=0;side<2;++side) {
        auto boundary=[&](int x) { return side==0 ? solid(x,y)&&!solid(x,y-1) : solid(x,y-1)&&!solid(x,y); };
        for (int x=0;x<w;) {
            if (!boundary(x)) {++x;continue;}
            int start=x;
            while (x<w && boundary(x)) ++x;
            if (side==0) edges.push_back({{float(start),float(y)},{float(x),float(y)}});
            else edges.push_back({{float(x),float(y)},{float(start),float(y)}});
        }
    }
    for (int x=0;x<=w;++x) for (int side=0;side<2;++side) {
        auto boundary=[&](int y) { return side==0 ? solid(x,y)&&!solid(x-1,y) : solid(x-1,y)&&!solid(x,y); };
        for (int y=0;y<h;) {
            if (!boundary(y)) {++y;continue;}
            int start=y;
            while (y<h && boundary(y)) ++y;
            if (side==0) edges.push_back({{float(x),float(y)},{float(x),float(start)}});
            else edges.push_back({{float(x),float(start)},{float(x),float(y)}});
        }
    }
    UnloadImageColors(pixels);
    return edges;
}

bool ShadowRenderer2D::ContainsLight(const Shape& shape,Vector4 dest,Vector2 size,Vector2 light) {
    if(dest.z<=0 || dest.w<=0) return false;
    Vector2 p{(light.x-dest.x)*size.x/dest.z,(light.y-dest.y)*size.y/dest.w};
    if(p.x<0 || p.y<0 || p.x>size.x || p.y>size.y) return false;
    // Even/odd crossings preserve holes and disconnected PNG components.
    // Edges are axis-aligned in source space; no texture readback is needed.
    bool inside=false;
    for(const Edge& edge:shape.edges) {
        // Treat the solid boundary as closed. Otherwise an exact vertex/surface
        // sample can evade both the containment and edge-facing tests.
        if(edge.a.x==edge.b.x && p.x==edge.a.x &&
           p.y>=std::min(edge.a.y,edge.b.y) && p.y<=std::max(edge.a.y,edge.b.y)) return true;
        if(edge.a.y==edge.b.y && p.y==edge.a.y &&
           p.x>=std::min(edge.a.x,edge.b.x) && p.x<=std::max(edge.a.x,edge.b.x)) return true;
        if((edge.a.y>p.y)!=(edge.b.y>p.y))
            if(p.x<edge.a.x) inside=!inside;
    }
    return inside;
}

void ShadowRenderer2D::CreateGpuShape(Shape& s) {
    auto& g=glapi();
    if (!g.ready || s.vao || s.edges.empty()) return;
    std::vector<Vertex> verts;
    verts.reserve(s.edges.size()*6);
    for (const Edge& e:s.edges) for (int c=0;c<6;++c)
        verts.push_back({e.a.x,e.a.y,e.b.x,e.b.y,float(c)});
    g.genVAO(1,&s.vao);
    g.bindVAO(s.vao);
    g.genBuffers(1,&s.vertices);
    g.bindBuffer(GL_ARRAY_BUFFER,s.vertices);
    g.bufferData(GL_ARRAY_BUFFER, std::ptrdiff_t(verts.size()*sizeof(Vertex)),verts.data(),GL_STATIC_DRAW);
    g.enableAttrib(0); g.attribPointer(0,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,ax));
    g.enableAttrib(1); g.attribPointer(1,1,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,corner));
    g.genBuffers(1,&s.instances);
    g.bindBuffer(GL_ARRAY_BUFFER,s.instances);
    g.bufferData(GL_ARRAY_BUFFER,0,nullptr,GL_DYNAMIC_DRAW);
    g.enableAttrib(2); g.attribPointer(2,4,GL_FLOAT,GL_FALSE,sizeof(Vector4),nullptr);
    g.attribDivisor(2,1);
    g.bindBuffer(GL_ARRAY_BUFFER,0);
    g.bindVAO(0);
}

bool ShadowRenderer2D::EnsureShader() {
    if (!m_shaderAttempted) {
        m_shaderAttempted=true;
        glapi().init();
        {
            m_shader=LoadShader("src/shaders/shadow_instanced.vs", "src/shaders/shadow_instanced.fs");
            if (IsShaderValid(m_shader)) {
                m_mvp=GetShaderLocation(m_shader,"mvp");
                m_light=GetShaderLocation(m_shader,"lightPosition");
                m_length=GetShaderLocation(m_shader,"shadowLength");
                m_color=GetShaderLocation(m_shader,"maskColor");
                m_sourceSize=GetShaderLocation(m_shader,"sourceSize");
                m_propMode=GetShaderLocation(m_shader,"propMode");
                m_propSettings=GetShaderLocation(m_shader,"propSettings");
                m_referenceDest=GetShaderLocation(m_shader,"referenceDest");
                if(m_mvp<0 || m_light<0 || m_length<0 || m_color<0 || m_sourceSize<0 ||
                   m_propMode<0 || m_propSettings<0 || m_referenceDest<0) {
                    TraceLog(LOG_WARNING,"Invalid instanced shadow shader; using CPU reference");
                    UnloadShader(m_shader);m_shader={};
                }
            }
        }
    }
    return IsShaderValid(m_shader);
}

bool ShadowRenderer2D::BeginCollection() {
    if (!EnsureShader() || !glapi().ready) return false;
    m_stats={}; m_collecting=true;
    for (auto& [key,s]:m_cache) s.queued.clear();
    for (auto& [key,s]:m_footprints) s.queued.clear();
    return true;
}

void ShadowRenderer2D::EndCollection() {
    m_collecting=false;
    auto& g=glapi();
    if (!g.ready) return;
    for (auto& [key,s]:m_cache) UploadInstances(s);
    for (auto& [key,s]:m_footprints) UploadInstances(s);
}

void ShadowRenderer2D::UploadInstances(Shape& s) {
    auto& g=glapi();
    if (s.queued.empty() || s.edges.empty()) return;
    CreateGpuShape(s);
    if (!s.vao || equalTransforms(s.queued,s.uploaded)) return;
    g.bindBuffer(GL_ARRAY_BUFFER,s.instances);
    auto count=std::ptrdiff_t(s.queued.size()*sizeof(Vector4));
    if (int(s.queued.size())>s.capacity) {
        g.bufferData(GL_ARRAY_BUFFER,count,s.queued.data(),GL_DYNAMIC_DRAW);
        s.capacity=int(s.queued.size());
    } else g.bufferSubData(GL_ARRAY_BUFFER,0,count,s.queued.data());
    g.bindBuffer(GL_ARRAY_BUFFER,0);
    s.uploaded=s.queued;
    m_stats.uploadBytes+=unsigned(count);
}

void ShadowRenderer2D::DrawBatch(Vector2 lightPosition,float shadowLength,Color maskColor) {
    if (!IsShaderValid(m_shader) || !glapi().ready || shadowLength<=0) return;
    auto& g=glapi();
    rlDrawRenderBatchActive();
    // A point sample embedded in opaque alpha is fully occluded. Transparent
    // texels inside the sprite bounds are not blockers. Keep MIN blending active.
    for(const auto& [key,s]:m_cache) for(Vector4 dest:s.queued)
        if(ContainsLight(s,dest,{float(std::get<3>(key)),float(std::get<4>(key))},lightPosition)) {
            DrawRectangleRec(viewBounds(),maskColor);
            ++m_stats.drawCalls;
            return;
        }
    // This is raylib's current 2D camera projection + modelview.
    Matrix mvp=MatrixMultiply(rlGetMatrixModelview(),rlGetMatrixProjection());
    g.useProgram(m_shader.id);
    g.uniform1(m_propMode,0);
    if (m_mvp>=0) g.uniformMatrix(m_mvp,1,GL_FALSE,MatrixToFloat(mvp));
    if (m_light>=0) g.uniform2(m_light,lightPosition.x,lightPosition.y);
    if (m_length>=0) g.uniform1(m_length,shadowLength);
    if (m_color>=0) g.uniform4(m_color,maskColor.r/255.0f,maskColor.g/255.0f,maskColor.b/255.0f,maskColor.a/255.0f);
    for (auto& [key,s]:m_cache) {
        if (s.queued.empty() || !s.vao || s.edges.empty()) continue;
        if (m_sourceSize>=0) g.uniform2(m_sourceSize,float(std::get<3>(key)),float(std::get<4>(key)));
        g.bindVAO(s.vao);
        g.drawInstanced(GL_TRIANGLES,0,GLsizei(s.edges.size()*6),GLsizei(s.queued.size()));
        m_stats.drawCalls++;
    }
    g.uniform1(m_propMode,1);
    for (auto& [key,s]:m_footprints) {
        if (s.queued.empty() || !s.vao) continue;
        g.uniform4(m_propSettings,std::get<0>(key),std::get<1>(key),std::get<2>(key),std::get<3>(key));
        g.bindVAO(s.vao);
        g.drawInstanced(GL_TRIANGLES,0,6,GLsizei(s.queued.size()));
        ++m_stats.drawCalls;
    }
    g.bindVAO(0);
    g.useProgram(0);
}

void ShadowRenderer2D::DrawCaster(Texture2D texture, Rectangle source, Rectangle destination,
    const ShadowStyle& style, Vector2 light, float shadowLength, Color color) {
    switch(style.mode) {
        case ShadowMode::None: return;
        case ShadowMode::Occlusion: Draw(texture,source,destination,light,shadowLength,color); return;
        case ShadowMode::Footprint:
        case ShadowMode::ContactOnly: DrawFootprint(ShadowBase(destination,style),style,light,color); return;
    }
}

void ShadowRenderer2D::DrawFootprint(Rectangle base,const ShadowStyle& style,Vector2 light,Color color) {
    if (!std::isfinite(base.x) || !std::isfinite(base.y) || !std::isfinite(base.width) ||
        !std::isfinite(base.height) || base.width<=0 || base.height<=0) return;
    auto finite=[](float value,float fallback){return std::isfinite(value)?value:fallback;};
    float length=style.mode==ShadowMode::ContactOnly ? 0 : std::max(0.0f,finite(style.maxProjection,0));
    float fade=std::max(0.001f,finite(style.nearFadeDistance,24));
    float projection=std::clamp(finite(style.projectionOpacity,0.45f),0.0f,1.0f);
    float contact=std::clamp(finite(style.contactOpacity,0.65f),0.0f,1.0f);
    if(m_collecting) {
        auto& shape=m_footprints[{length,fade,projection,contact}];
        // One reusable quad per base instance. No PNG or per-asset vertices.
        if(shape.edges.empty()) shape.edges.push_back({{0,0},{1,1}});
        shape.queued.push_back({base.x,base.y,base.width,base.height});
        ++m_stats.casters;
        return;
    }
    if(!EnsureShader()) {
        // Shader failure still retains a local soft contact shadow, never a wedge.
        rlPushMatrix();
        rlTranslatef(base.x+base.width*0.5f,base.y+base.height*0.5f,0);
        rlScalef(base.width*0.5f,base.height*0.5f,1);
        Color inner{(unsigned char)(255+(color.r-255)*contact),
            (unsigned char)(255+(color.g-255)*contact),(unsigned char)(255+(color.b-255)*contact),
            (unsigned char)(255+(color.a-255)*contact)};
        DrawCircleGradient({0,0},1,inner,WHITE);
        rlPopMatrix();return;
    }
    // Reference path submits a conservative quad on the CPU; the same analytic
    // fragment coverage is used by both backends. No instance buffers required.
    rlDrawRenderBatchActive();
    float mode=2;
    Vector4 settings{length,fade,projection,contact};
    Vector4 dest{base.x,base.y,base.width,base.height};
    Vector4 mask{color.r/255.0f,color.g/255.0f,color.b/255.0f,color.a/255.0f};
    BeginShaderMode(m_shader);
    SetShaderValue(m_shader,m_propMode,&mode,SHADER_UNIFORM_FLOAT);
    SetShaderValue(m_shader,m_propSettings,&settings,SHADER_UNIFORM_VEC4);
    SetShaderValue(m_shader,m_referenceDest,&dest,SHADER_UNIFORM_VEC4);
    SetShaderValue(m_shader,m_light,&light,SHADER_UNIFORM_VEC2);
    SetShaderValue(m_shader,m_color,&mask,SHADER_UNIFORM_VEC4);
    DrawRectangleRec({base.x-length,base.y-length,base.width+2*length,base.height+2*length},WHITE);
    EndShaderMode();
}

void ShadowRenderer2D::Draw(Texture2D texture, Rectangle source, Rectangle destination,
                            Vector2 lightPosition,float shadowLength,Color maskColor) {
    if (!texture.id || destination.width<=0 || destination.height<=0) return;
    if (source.width==0 || source.height==0) source={0,0,float(texture.width),float(texture.height)};
    ShapeKey key{texture.id,int(source.x),int(source.y),int(source.width),int(source.height)};
    auto it=m_cache.find(key);
    if (it==m_cache.end() || it->second.edges.empty()) return;
    if (m_collecting) {
        it->second.queued.push_back({destination.x,destination.y,destination.width,destination.height});
        m_stats.casters++;
        return;
    }
    // Reference/fallback path retained for tests and systems not using collection.
    if (shadowLength<=0) return;
    const auto& edges=it->second.edges;
    auto toWorld=[&](Vector2 p) {
        return Vector2{destination.x+p.x*destination.width/source.width,
                       destination.y+p.y*destination.height/source.height};
    };



    Rectangle bounds=viewBounds();
    if(ContainsLight(it->second,{destination.x,destination.y,destination.width,destination.height},
                     {source.width,source.height},lightPosition)) {
        DrawRectangleRec(bounds,maskColor);
        return;
    }
    for(const Edge& e:edges) {
        Vector2 a=toWorld(e.a),b=toWorld(e.b),delta=Vector2Subtract(b,a);
        double length=std::hypot(double(delta.x),double(delta.y));
        if(length<=0) continue;
        double distance=(double(delta.y)*(lightPosition.x-a.x)-double(delta.x)*(lightPosition.y-a.y))/length;
        if(distance<=0) continue;
        // Shared projective scale keeps the cap parallel to the edge and at
        // least shadowLength beyond it, even for a nearly grazing light.
        double w=distance/(distance+shadowLength);
        ClipPoint fa{a.x-lightPosition.x*(1-w),a.y-lightPosition.y*(1-w),w};
        ClipPoint fb{b.x-lightPosition.x*(1-w),b.y-lightPosition.y*(1-w),w};
        drawClippedQuad({ClipPoint{a.x,a.y,1},ClipPoint{b.x,b.y,1},fb,fa},bounds,maskColor);
    }
}
}
