#include "camera.hpp"

using namespace rasterizer;

Camera::Camera(Vec3 position, Vec3 target, Vec3 up)
    : position{position}, target{target}, up{up}
{
    view = look_at(position, target, up);
}

void Camera::update(Vec2 delta)
{
    const Vec3 right{view[0][0], view[0][1], view[0][2]};

    delta *= look_sensitivity;
    delta.y /= 2.f;

    position = (rotate(rotate(Mat4{1.f}, delta.x, up), delta.y, right) *
                Vec4{position, 1.f})
                   .xyz;

    view = look_at(position, target, up);
}

void Camera::zoom(int direction)
{
    if (direction > 0)
        position *= 1.1;
    else
        position *= 0.9;
}

const Mat4 &Camera::get_view() const { return view; }