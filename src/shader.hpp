#pragma once

#include "matrix.hpp"
#include "model.hpp"
#include "vector.hpp"

namespace rasterizer
{

struct Varying
{
    Vec4 pos;
    Vec3 normal;
    Vec2 uv;
};

struct Uniforms
{
    Mat4 mvp;
    Texture *texture;
};

// TODO: The shader logic is fixed for now, you should be able to define custom
// shaders. However, we would like to avoid the overhead of virtual functions.
class Shader
{
    // Viewport information.
    int width;
    int height;

  public:
    Uniforms uniforms;

    Shader(int width, int height);

    // Pipeline
    Varying vertex(const Vertex &in);
    void post_process(Varying &v);
    Varying vary(Vec3 bc, const Varying &v0, const Varying &v1,
                 const Varying &v2);
    Color8 fragment(const Varying &in);
};

} // namespace rasterizer