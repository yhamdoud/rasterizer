#pragma once

#include <array>
#include <memory>

#include <SDL2/SDL.h>

#include "camera.hpp"
#include "frame_buffer.hpp"
#include "model.hpp"
#include "vector.hpp"

namespace rasterizer
{

enum class BufferType
{
    color,
    depth,
};

class Rasterizer
{
  private:
    int width;
    int height;

    Model model;
    Color clear_color = Color{0, 0, 0, 255};

    FrameBuffer<float> depth_buffer;
    FrameBuffer<Vector<std::uint8_t, 4>> color_buffer;

    Camera camera;
    IVec2 mouse_position;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Texture *depth_texture;

    BufferType presented_buffer{BufferType::color};

  public:
    Rasterizer(std::size_t width, std::size_t height, Model &&model);
    Rasterizer(const Rasterizer &r) = delete;
    Rasterizer &operator=(const Rasterizer &r) = delete;
    ~Rasterizer();

    void run();
    void draw_wireframe();
    void draw_triangle(Vec3 p0, Vec3 p1, Vec3 p2, Color color);
    void draw_line(IVec2 p1, IVec2 p2);
    void draw_point(IVec2 p);
    void draw_point(Vec2 p, Color c);
    void set_color(Color color);
};

} // namespace rasterizer