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

  public:
    enum class WrapMode
    {
        repeat,
        clamp,
    };

    WrapMode mode = WrapMode::repeat;

    Texture(int width, int height, int channel_count,
            std::unique_ptr<std::uint8_t> data);

    static std::optional<Texture> from_file(const std::filesystem::path &path);

    Color8 operator()(int x, int y) const;
    Color8 operator()(IVec2 c) const;

    Color8 operator()(float u, float v) const;
    Color8 operator()(Vec2 c) const;
};

class Model
{
  public:
    std::unique_ptr<Mesh> mesh;
    std::unique_ptr<Texture> diffuse_texture;

    static Model from_obj(const std::filesystem::path &path);
};

} // namespace rasterizer