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
    // Perspective divide to NDC from clip space.
    v.pos.w = 1 / v.pos.w;
    // Keep the reciprocal of w for perspective correction.
    v.pos.xyz *= v.pos.w;

    // Viewport transform to screen space.
    v.pos.x = (v.pos.x + 1.f) / 2.f * (float)width;
    v.pos.y = (1.f - v.pos.y) / 2.f * (float)height;
}

Varying Shader::vary(Vec3 bc, const Varying &v0, const Varying &v1,
                     const Varying &v2)
{
    return Varying{
        bc.x * v0.pos + bc.y * v1.pos + bc.z * v2.pos,
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