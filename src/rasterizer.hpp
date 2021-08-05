#pragma once

#include <array>
#include <memory>

#include <SDL2/SDL.h>
#include <SDL_timer.h>

#include "camera.hpp"
#include "frame_buffer.hpp"
#include "model.hpp"
#include "shader.hpp"
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
    Shader shader;
    Color clear_color = Color{0, 0, 0, 255};

    FrameBuffer<float> depth_buffer;
    FrameBuffer<Color8> color_buffer;

    Camera camera;
    IVec2 mouse_position;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *color_texture;
    SDL_Texture *depth_texture;

    uint prev_tick;

    BufferType presented_buffer{BufferType::color};

  public:
    Rasterizer(int width, int height, Model &&model);
    Rasterizer(const Rasterizer &r) = delete;
    Rasterizer &operator=(const Rasterizer &r) = delete;
    ~Rasterizer();

    void run();
    void draw();
    void draw_triangle(Varying in0, Varying in1, Varying in2);
    void draw_line(IVec2 p1, IVec2 p2);
    void draw_point(IVec2 p);
    void draw_point(Vec2 p, Color8 c);
    void draw_point(Vec2 p, Color c);
    void set_color(Color color);
};

} // namespace rasterizer