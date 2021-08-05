#include "shader.hpp"

using namespace rasterizer;

Shader::Shader(int width, int height) : width(width), height(height) {}

Varying Shader::vertex(const Vertex &in)
{
    return Varying{
        uniforms.mvp * Vec4{in.position, 1.f}, // Model to clip space.
        in.normal,
        in.uv,
    };
}

void Shader::post_process(Varying &v)
{
    // Perspective divide to NDC space. Homogenize, but keep reciprocal of w.
    v.position.w = 1 / v.position.w;
    v.position.x *= v.position.w;
    v.position.y *= v.position.w;
    v.position.z *= v.position.w;

    // Viewport transform to screen space.
    v.position.x = (v.position.x + 1.f) / 2.f * (float)width;
    v.position.y = (1.f - v.position.y) / 2.f * (float)height;
}

Varying Shader::vary(Vec3 bc, const Varying &v0, const Varying &v1,
                     const Varying &v2)
{
    return Varying{
        bc.x * v0.position + bc.y * v1.position + bc.z * v2.position,
        bc.x * v0.normal + bc.y * v1.normal + bc.z * v2.normal,
        bc.x * v0.uv + bc.y * v1.uv + bc.z * v2.uv,
    };
}

Color8 Shader::fragment(const Varying &in)
{
    if (uniforms.texture)
        return (*uniforms.texture)(in.uv);
    else
        return Color8{255};
}