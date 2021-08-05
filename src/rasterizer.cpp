#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <SDL_keycode.h>
#include <SDL_messagebox.h>
#include <SDL_mouse.h>
#include <SDL_render.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include "camera.hpp"
#include "matrix.hpp"
#include "model.hpp"
#include "rasterizer.hpp"
#include "utils.hpp"
#include "vector.hpp"

using std::round;

using namespace rasterizer;
using namespace utils;

static IVec2 get_mouse_position()
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    return IVec2{x, y};
}

Rasterizer::Rasterizer(int width, int height, Model &&model)
    : width{width}, height{height}, model{std::move(model)},
      camera{Vec3{0.f, 2.f, 2.f}, Vec3{0.f}},
      depth_buffer{static_cast<size_t>(width), static_cast<size_t>(height)},
      color_buffer{static_cast<size_t>(width), static_cast<size_t>(height)},
      shader(width, height)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw std::runtime_error("Failed to initialize SDL.");

    SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
    color_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STATIC, width, height);
}

Rasterizer::~Rasterizer()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(color_texture);

    SDL_Quit();
}

void Rasterizer::run()
{
    bool close_window = false;

    while (!close_window)
    {

        // Event polling
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                close_window = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_f:
                    if (presented_buffer == BufferType::color)
                        presented_buffer = BufferType::depth;
                    else
                        presented_buffer = BufferType::color;

                    break;
                }
                break;
            case SDL_MOUSEWHEEL:
                camera.zoom(event.wheel.y);
                camera.update(Vec2{0});
            }
        }

        draw();

        SDL_UpdateTexture(color_texture, nullptr, color_buffer.get(),
                          width * 4 * sizeof(uint8_t));

        set_color(clear_color);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, color_texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        color_buffer.fill(0.f);
        depth_buffer.fill(std::numeric_limits<float>::max());

        // Show FPS.
        int tick = SDL_GetTicks();
        int fps = 1000.f / (tick - prev_tick);
        prev_tick = tick;

        // Clear and return to beginning of line.
        std::cout << "\33[2K\r" << fps << std::flush;
    }
}

int middle_mouse_down()
{
    return SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MIDDLE;
}

void Rasterizer::draw()
{
    shader.uniforms.mvp =
        perspective(utils::radians(90.f), (float)width / (float)height, 0.1f,
                    100.f) *
        camera.get_view();
    shader.uniforms.texture = model.diffuse_texture.get();

    auto new_mouse_position = get_mouse_position();

    if (middle_mouse_down())
        camera.update(new_mouse_position - mouse_position);

    mouse_position = new_mouse_position;

    for (size_t i = 0; i < model.mesh->vertices.size(); i += 3)
    {
        auto &v0 = model.mesh->vertices[i];
        auto &v1 = model.mesh->vertices[i + 1];
        auto &v2 = model.mesh->vertices[i + 2];

        auto out1 = shader.vertex(v0);
        auto out2 = shader.vertex(v1);
        auto out3 = shader.vertex(v2);

        shader.post_process(out1);
        shader.post_process(out2);
        shader.post_process(out3);

        draw_triangle(out1, out2, out3);
    }
}

void Rasterizer::draw_point(Vec2 p, Color c)
{
    draw_point(p, Color8{c.r * 255, c.g * 255, c.b * 255, c.a * 255});
}

void Rasterizer::draw_point(Vec2 p, Color8 c) { color_buffer(p.x, p.y) = c; }

void Rasterizer::draw_point(IVec2 p)
{
    SDL_RenderDrawPoint(renderer, p.x, p.y);
}

// Returns the signed area of the parallelogram spanned by edges p0p1 and p0p2.
// Given the line p0p1, the edge function has the useful property that:
//  - edge(p0, p1, p2) = 0 if p2 is on the line,
//  - edge(p0, p1, p2) > 0 if p2 is above/right of the line,
//  - edge(p0, p1, p2) < 0 if p2 is under/left of the line.
int edge(IVec2 p0, IVec2 p1, IVec2 p2)
{
    return (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
}

// void shade_fragment(Vec3 v0, Vec3

// Parallel implementation of Pineda's triangle rasterization algorithm.
// https://dl.acm.org/doi/pdf/10.1145/54852.378457
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
// https://scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/
// https://web.archive.org/web/20130816170418/http://devmaster.net/forums/topic/1145-advanced-rasterization/
void Rasterizer::draw_triangle(Varying in1, Varying in2, Varying in3)
{
    int prec = 16;
    float fprec = static_cast<float>(prec);

    // Use fixed-point screen coordinates for sub-pixel precision.
    IVec2 p0{std::round(fprec * in1.position.x),
             std::round(fprec * in1.position.y)};
    IVec2 p1{std::round(fprec * in2.position.x),
             std::round(fprec * in2.position.y)};
    IVec2 p2{std::round(fprec * in3.position.x),
             std::round(fprec * in3.position.y)};

    IVec2 min{std::min({p0.x, p1.x, p2.x}), std::min({p0.y, p1.y, p2.y})};
    min /= prec;
    IVec2 max{std::max({p0.x, p1.x, p2.x}), std::max({p0.y, p1.y, p2.y})};
    max /= prec;

    // Clip triangle.
    min.x = std::max(0, min.x);
    min.y = std::max(0, min.y);

    max.x = std::min(width, max.x);
    max.y = std::min(height, max.y);

    // Pixel centers are located at (0.5, 0.5).
    IVec2 p{round(fprec * (min.x + 0.5f)), round(fprec * (min.y + 0.5f))};

    // Precompute triangle edges for incremental computation of edge function.
    IVec3 bc_row{edge(p1, p2, p), edge(p2, p0, p), edge(p0, p1, p)};
    IVec3 bc_dx{p2.y - p1.y, p0.y - p2.y, p1.y - p0.y};
    IVec3 bc_dy{p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};
    bc_dx *= prec;
    bc_dy *= prec;

    // 1 / (2 * area of triangle)
    float area_reciprocal = 1.f / edge(p0, p1, p2);

    // Adhere to the top-left rule fill convention by adding bias values.
    // In clockwise order, left edges must go up while top edges stay horizontal
    // and go right.
    bc_row += prec * IVec3{bc_dy.x > 0 || (bc_dy.x == 0 && bc_dx.x > 0),
                           bc_dy.y > 0 || (bc_dy.y == 0 && bc_dx.y > 0),
                           bc_dy.z > 0 || (bc_dy.z == 0 && bc_dx.z > 0)};

    for (p.y = min.y; p.y <= max.y; p.y++)
    {
        auto bc = bc_row;

        for (p.x = min.x; p.x <= max.x; p.x++)
        {
            // Draw pixel if p is inside triangle.
            if (bc.x > 0 && bc.y > 0 && bc.z > 0)
            {
                // Normalize the barycentric coordinates.
                // TODO: Maybe we can do this using fixed-point arithmetic?
                auto bc_n = static_cast<Vec3>(bc) * area_reciprocal;
                float z = dot(
                    bc_n, Vec3{in1.position.z, in2.position.z, in3.position.z});

                if (z < depth_buffer(p.x, p.y))
                {
                    depth_buffer(p.x, p.y) = z;

                    draw_point(
                        p, shader.fragment(shader.vary(bc_n, in1, in2, in3)));

                    if (presented_buffer == BufferType::depth)
                        draw_point(p, Color{1 / z, 1 / z, 1 / z, 1.f});
                }
            }

            bc -= bc_dx;
        }

        bc_row += bc_dy;
    }
}

// Integer-only implementation of Bresenham's line algorithm.
// https://www.cs.helsinki.fi/group/goa/mallinnus/lines/bresenh.html
void Rasterizer::draw_line(IVec2 p1, IVec2 p2)
{
    if (!in_bounds(p1.x, 0, width) || !in_bounds(p2.x, 0, width) ||
        !in_bounds(p1.y, 0, height) || !in_bounds(p1.y, 0, height))
        return;

    bool mirror = false;
    if (std::abs(p1.x - p2.x) < std::abs(p1.y - p2.y))
    {
        std::swap(p1.x, p1.y);
        std::swap(p2.x, p2.y);
        mirror = true;
    }

    // Keep the invariant property that p1 lay left of p2.
    if (p1.x > p2.x)
        std::swap(p1, p2);

    auto d = p2 - p1;

    // ie = 2*e*dx, where e is the actual accumulated error so far.
    // We keep track of this term since it's always an integer, unlike e.
    int ie = 0;

    // Move from left to right, drawing the point to the right or to the
    // top/bottom right of the previous point based on the current error.
    do
    {
        draw_point((mirror) ? p1.swap() : p1);

        ie += std::abs(d.y);
        if (2 * ie >= d.x)
        {
            p1.y += (d.y >= 0) ? 1 : -1;
            ie -= d.x;
        }
    } while (++p1.x <= p2.x);
}

void Rasterizer::set_color(Color color)
{
    SDL_SetRenderDrawColor(renderer, round(color.r * 255), round(color.g * 255),
                           round(color.b * 255), round(color.a * 255));
}
