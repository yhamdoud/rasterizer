// Camera with arcball controls.

#pragma once

#include "matrix.hpp"
#include "vector.hpp"

using namespace rasterizer;

class Camera
{
    Vec3 position;
    Vec3 target;
    Vec3 up;

    Mat4 view;

    float look_sensitivity = 0.01f;

  public:
    Camera(Vec3 position, Vec3 target, Vec3 up = Vec3::up());

    void update(Vec2 delta);
    void zoom(int direction);

    const Mat4 &get_view() const;
};