#pragma once

#include <SDL_render.h>
#include <filesystem>
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

class Model
{
  public:
    Mesh mesh;

    Model(Mesh &&mesh);
    static Model from_obj(const std::filesystem::path &path);
};

} // namespace rasterizer