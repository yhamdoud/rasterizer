#pragma once

#include <array>
#include <memory>

#include <SDL2/SDL.h>

#include "buffer.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "vector.hpp"

namespace rasterizer
{
class Rasterizer
{
  private:
    int width;
    int height;

    Model model;
    Color clear_color = Color{0, 0, 0, 255};

    Buffer<float> depth_buffer;

    IVec2 mouse_position;

    SDL_Window *window;
    SDL_Renderer *renderer;
    Camera camera;

  public:
    Rasterizer(std::size_t width, std::size_t height, Model &&model);
    Rasterizer(const Rasterizer &r) = delete;
    Rasterizer &operator=(const Rasterizer &r) = delete;
    ~Rasterizer();

    void run();
    void draw_wireframe();
    void draw_triangle(Vec3 p0, Vec3 p1, Vec3 p2);
    void draw_line(IVec2 p1, IVec2 p2);
    void draw_point(IVec2 p);
    void set_color(Color color);
};

} // namespace rasterizer