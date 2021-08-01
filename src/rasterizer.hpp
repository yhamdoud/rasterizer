#pragma once

#include <memory>

#include <SDL2/SDL.h>

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

    IVec2 mouse_position;

    SDL_Window *window;
    SDL_Renderer *renderer;
    Camera camera;

  public:
    Rasterizer(int width, int height, Model &&model);
    Rasterizer(const Rasterizer &r) = delete;
    Rasterizer &operator=(const Rasterizer &r) = delete;
    ~Rasterizer();

    void run();
    void draw_wireframe();
    void draw_triangle(IVec2 p1, IVec2 p2, IVec2 p3);
    void draw_line(IVec2 p1, IVec2 p2);
    void draw_point(IVec2 p);
    void set_color(Color color);
};

} // namespace rasterizer