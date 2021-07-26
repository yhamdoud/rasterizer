#include <iostream>
#include <numbers>

#include <SDL2/SDL.h>
#include <SDL_mouse.h>
#include <SDL_render.h>

#include "vector.hpp"

using namespace rasterizer;

constexpr int window_width = 640, window_height = 480;
constexpr Color col{255, 0, 0, 255};

static void draw_point(SDL_Renderer &renderer, IVec2 p)
{
    SDL_RenderDrawPoint(&renderer, p.x, p.y);
}

template <typename T>
static bool in_bounds(const T &val, const T &low, const T &high)
{
    return low < val && val < high;
}

// Integer-only implementation of Bresenham's line algorithm.
// https://www.cs.helsinki.fi/group/goa/mallinnus/lines/bresenh.html
static void draw_line(SDL_Renderer &renderer, IVec2 p1, IVec2 p2)
{

    if (!in_bounds(p1.x, 0, window_width) ||
        !in_bounds(p2.x, 0, window_width) ||
        !in_bounds(p1.y, 0, window_height) ||
        !in_bounds(p1.y, 0, window_height))
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

    // Equal to 2*e*dx, where e is the actual accumulated error so far.
    // We keep track of this term since it's always an integer, unlike e.
    int err = 0;

    while (p1.x <= p2.x)
    {
        draw_point(renderer, (mirror) ? p1.swap() : p1);

        err += std::abs(d.y);
        if (2 * err >= d.x)
        {
            p1.y += (d.y >= 0) ? 1 : -1;
            err -= d.x;
        }

        p1.x++;
    }
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        return 1;

    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_CreateWindowAndRenderer(window_width, window_height, 0, &window,
                                &renderer);

    bool close_window = false;

    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);

    IVec2 origin{window_width / 2, window_height / 2};
    int radius = 200;
    int line_count = 32;

    for (int i = 0; i <= line_count; i++)
    {
        float ratio = (float)i / float(line_count) * 2 * std::numbers::pi;

        Vec2 offset{std::cos(ratio), std::sin(ratio)};
        auto target = static_cast<IVec2>(radius * offset);

        draw_line(*renderer, origin, origin + target);
    }

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

        SDL_RenderPresent(renderer);
    }
}
