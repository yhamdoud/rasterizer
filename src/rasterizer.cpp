#include <iostream>
#include <memory>
#include <numbers>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <SDL_mouse.h>
#include <SDL_video.h>

#include "matrix.hpp"
#include "model.hpp"
#include "rasterizer.hpp"
#include "vector.hpp"

using namespace rasterizer;

constexpr double radians_per_degree = std::numbers::pi / 180.f;

template <typename T>
static bool in_bounds(const T &val, const T &low, const T &high)
{
    return low < val && val < high;
}

static IVec2 get_mouse_position()
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    return IVec2{x, y};
}

static float radians(float degrees) { return degrees * radians_per_degree; }

Rasterizer::Rasterizer(int width, int height, Model &&model)
    : width{width}, height{height}, model{model}
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw std::runtime_error("Failed to initialize SDL.");

    SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
}

Rasterizer::~Rasterizer()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

void Rasterizer::run()
{
    bool close_window = false;

    while (!close_window)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        switch (event.type)
        {
        case SDL_QUIT:
            close_window = true;
            break;
        }

        set_color(clear_color);
        SDL_RenderClear(renderer);

        set_color(colors::red);
        draw_wireframe();

        SDL_RenderPresent(renderer);
    }
}

void Rasterizer::draw_wireframe()
{
    const Vec3 model_pos = Vec3{0.f, 0.f, 5.f};
    const Vec3 model_scale = Vec3{1.f};
    const Vec3 cam_pos = Vec3{0.0f, 0.0f, 0.f};

    const Mat4 model = translate(scale(Mat4{1.f}, model_scale), model_pos);
    const Mat4 view = look_at(cam_pos, model_pos, Vec3::up());
    const Mat4 proj =
        perspective(radians(50.f), (float)width / (float)height, 0.1f, 100.f);

    const Mat4 mvp = proj * view * model;

    auto mesh = this->model.mesh;

    for (size_t i = 0; i < mesh.vertices.size(); i += 3)
    {
        auto v1 = mesh.vertices[i];
        auto v2 = mesh.vertices[i + 1];
        auto v3 = mesh.vertices[i + 2];

        // Model to clip space.
        auto p1 = mvp * Vec4{v1.position, 1.f};
        // Perspective divison, points end up in NDC space.
        p1 /= p1.w;

        auto p2 = mvp * Vec4{v2.position, 1.f};
        p2 /= p2.w;

        auto p3 = mvp * Vec4{v3.position, 1.f};
        p3 /= p3.w;

        auto viewport = [&](const Vec2 &p)
        {
            return IVec2{(p.x + 1.f) / 2.f * (float)this->width,
                         (1.f - p.y) / 2.f * (float)this->height};
        };

        // Viewport transformation.
        draw_line(viewport(p1.xy), viewport(p2.xy));
        draw_line(viewport(p2.xy), viewport(p3.xy));
        draw_line(viewport(p1.xy), viewport(p3.xy));
    }
}

void Rasterizer::draw_point(IVec2 p)
{
    SDL_RenderDrawPoint(renderer, p.x, p.y);
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
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}