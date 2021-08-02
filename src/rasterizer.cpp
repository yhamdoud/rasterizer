#include <SDL_keycode.h>
#include <SDL_render.h>
#include <SDL_timer.h>
#include <iostream>
#include <limits>
#include <memory>
#include <numbers>
#include <stdexcept>

#include <SDL2/SDL.h>
#include <SDL_mouse.h>
#include <SDL_video.h>

#include "camera.hpp"
#include "matrix.hpp"
#include "model.hpp"
#include "rasterizer.hpp"
#include "vector.hpp"

using namespace rasterizer;
using std::round;

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

Rasterizer::Rasterizer(size_t width, size_t height, Model &&model)
    : width{(int)width}, height{(int)height}, model{model},
      camera{Vec3{0.f, 0.f, 3.f}, Vec3{0.f}}, depth_buffer{width, height},
      color_buffer{width, height}
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw std::runtime_error("Failed to initialize SDL.");

    SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STATIC, width, height);
}

Rasterizer::~Rasterizer()
{
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);

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
            }
        }

        draw_wireframe();

        SDL_UpdateTexture(texture, nullptr, color_buffer.get(),
                          width * 4 * sizeof(uint8_t));

        set_color(clear_color);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
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

// Object to viewport.

void Rasterizer::draw_wireframe()
{
    const Vec3 model_scale = Vec3{10.f};

    auto new_mouse_position = get_mouse_position();

    if (middle_mouse_down())
        camera.update(new_mouse_position - mouse_position);

    mouse_position = new_mouse_position;

    const Mat4 model = scale(Mat4{1.f}, model_scale);
    const Mat4 proj =
        perspective(radians(90.f), (float)width / (float)height, 0.1f, 100.f);

    const Mat4 mvp = proj * camera.get_view() * model;

    auto mesh = this->model.mesh;

    for (size_t i = 0; i < mesh.vertices.size(); i += 3)
    {
        auto v1 = mesh.vertices[i];
        auto v2 = mesh.vertices[i + 1];
        auto v3 = mesh.vertices[i + 2];

        auto viewport = [&](const Vec3 &in)
        {
            // Model to clip space.
            auto p = mvp * Vec4{in, 1.f};

            // Perspective divide to NDC space.
            p /= p.w;

            // Viewport
            p.x = (p.x + 1.f) / 2.f * (float)width;
            p.y = (1.f - p.y) / 2.f * (float)height;

            return p.xyz;
        };

        auto p1 = viewport(v1.position);
        auto p2 = viewport(v2.position);
        auto p3 = viewport(v3.position);

        auto e1 =
            model * Vec4{v2.position, 1.f} - model * Vec4{v1.position, 1.f};
        auto e2 =
            model * Vec4{v3.position, 1.f} - model * Vec4{v1.position, 1.f};

        Vec3 normal = normalize(cross(e1.xyz, e2.xyz));
        Vec3 l = normalize(Vec3{1.f, 0.5f, 0.25f});

        Color ambient{0.1f};

        float n_dot_l = std::max(dot(normal, l), 0.f);

        auto col = ambient + n_dot_l * colors::white;

        // set_color(Color{col.rgb, 1.f});

        draw_triangle(p1, p2, p3, col);

        // Viewport transformation.
        set_color(colors::red);
        // draw_line(viewport(p1.xy), viewport(p2.xy));
        // draw_line(viewport(p2.xy), viewport(p3.xy));
        // draw_line(viewport(p1.xy), viewport(p3.xy));
    }
}

void Rasterizer::draw_point(Vec2 p, Color c)
{
    color_buffer(p.x, p.y) =
        Vector<uint8_t, 4>{c.r * 255, c.g * 255, c.b * 255, c.a * 255};
}

void Rasterizer::draw_point(IVec2 p)
{
    SDL_RenderDrawPoint(renderer, p.x, p.y);
}

// Returns the signed area of the parallelogram spanned by edges p0p1 and p0p2.
// Given the line p0p1, the edge function has the useful property that:
//  - edge(p0, p1, p2) = 0 if p2 is on the line,
//  - edge(p0, p1, p2) > 0 if p2 is to right of the line,
//  - edge(p0, p1, p2) < 0 if p2 is to right of the line.
int edge(IVec2 p0, IVec2 p1, IVec2 p2)
{
    return (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
}

// Parallel implementation of Pineda's triangle rasterization algorithm.
// https://dl.acm.org/doi/pdf/10.1145/54852.378457
// https://fgiesen.wordpress.com/2013/02/10/optimizing-the-basic-rasterizer/
// https://fgiesen.wordpress.com/2013/02/08/triangle-rasterization-in-practice/
// https://scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/
void Rasterizer::draw_triangle(Vec3 v0, Vec3 v1, Vec3 v2, Color color)
{
    // FIXME: Snapping pixels to the grid; not good.
    auto p0 = static_cast<IVec3>(v0);
    auto p1 = static_cast<IVec3>(v1);
    auto p2 = static_cast<IVec3>(v2);

    // Calculate bounding box.
    IVec2 min{std::min({p0.x, p1.x, p2.x}), std::min({p0.y, p1.y, p2.y})};
    IVec2 max{std::max({p0.x, p1.x, p2.x}), std::max({p0.y, p1.y, p2.y})};

    // Clip triangle.
    min.x = std::max(0, min.x);
    min.y = std::max(0, min.y);

    max.x = std::min(width, max.x);
    max.y = std::min(height, max.y);

    IVec2 p;

    for (p.x = min.x; p.x < max.x; p.x++)
    {
        for (p.y = min.y; p.y < max.y; p.y++)
        {
            auto b = Vec3{edge(p1.xy, p2.xy, p), edge(p2.xy, p0.xy, p),
                          edge(p0.xy, p1.xy, p)};

            if (b.x >= 0 && b.y >= 0 && b.z >= 0)
            {
                draw_point(static_cast<IVec2>(p));

                // Calculate barycentric coordinates from edge function result.
                float area = edge(p0.xy, p1.xy, p2.xy);
                b /= area;

                auto z = dot(b, Vec3{v0.z, v1.z, v2.z});

                if (z < depth_buffer(p.x, p.y))
                {
                    depth_buffer(p.x, p.y) = z;
                    draw_point(p, color);

                    if (presented_buffer == BufferType::depth)
                        draw_point(p, Color{1 / z, 1 / z, 1 / z, 1.f});
                }
            }
        }
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
