#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

#include "vector.hpp"

namespace rasterizer
{

struct Vertex
{
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

class Mesh
{
  private:
  public:
    std::vector<Vertex> vertices;
    Mesh(std::vector<Vertex> vertices) : vertices{std::move(vertices)} {}
};

class Texture
{
    int width;
    int height;
    int channel_count;

    std::unique_ptr<std::uint8_t> data;

    constexpr IVec2 texels(Vec2 tex_coords) const;

  public:
    enum class WrapMode
    {
        repeat,
        clamp,
    };

    enum class SampleMode
    {
        nearest,
        bilinear,
    };

    WrapMode wrap = WrapMode::repeat;
    SampleMode sample = SampleMode::bilinear;

    Texture(int width, int height, int channel_count,
            std::unique_ptr<std::uint8_t> data);

    static std::optional<Texture> from_file(const std::filesystem::path &path);

    // Index texture using normalized floating point texture coordinates.
    // Conversion to texel indices is based on the texture sample mode.
    Color8 operator()(Vec2 tex_coords) const;

    // Index texture using integer texel indices. Out-of-bounds access is based
    // on the texture wrap mode.
    Color8 operator()(IVec2 texel) const;
    Color8 operator()(int x, int y) const;

    Color8 sample_bilinear(Vec2 tex_coords) const;
    Color8 sample_nearest(Vec2 tex_coords) const;
};

class Model
{
  public:
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Texture> diffuse_texture;

    static Model from_obj(const std::filesystem::path &path);
};

} // namespace rasterizer