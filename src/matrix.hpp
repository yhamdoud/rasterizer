// Generic matrix implementation parametrized by element type and dimension.

#pragma once

#include <array>
#include <cmath>
#include <iostream>
#include <ostream>

#include "vector.hpp"

using std::array;
using std::cos;
using std::ostream;
using std::sin;

namespace rasterizer
{

template <typename T, size_t m, size_t n, T... d> struct Matrix
{
    array<array<T, n>, m> data;

    constexpr Matrix() = default;

    template <typename... U>
    constexpr Matrix(const T &first, const U &...rest) : data{first, rest...}
    {
    }

    constexpr Matrix(const T &diag) : data{}
    {
        for (size_t i = 0; i < std::min(m, n); i++)
            data[i][i] = diag;
    }

    constexpr auto &operator[](size_t i) { return data[i]; }
    constexpr auto operator[](size_t i) const { return data[i]; }

    friend constexpr ostream &operator<<(ostream &out,
                                         const Matrix<T, m, n> &mat)
    {
        out << "{ ";

        for (const auto &row : mat.data)
        {
            out << "{ ";

            for (const auto e : row)
                out << e << " ";

            out << "} ";
        }

        out << "}";

        return out;
    }

    constexpr Matrix<T, m, n> &operator+=(const Matrix<T, m, n> &mat)
    {
        for (size_t i = 0; i < m; i++)
            for (size_t j = 0; j < m; j++)
                this[i][j] += mat[i][j];

        return this;
    }

    friend constexpr Matrix<T, m, n> operator+(Matrix<T, m, n> m1,
                                               const Matrix<T, m, n> &m2)
    {
        return m1 += m2;
    }

    friend constexpr Matrix<T, m, n> operator-(Matrix<T, m, n> m1,
                                               const Matrix<T, m, n> &m2)
    {
        return m1 -= m2;
    }

    constexpr Matrix<T, m, n> &operator-=(const Matrix<T, m, n> &mat)
    {
        for (size_t i = 0; i < m; i++)
            for (size_t j = 0; j < m; j++)
                this[i][j] -= mat[i][j];

        return this;
    }

    template <size_t p>
    friend constexpr Matrix<T, m, p> operator*(const Matrix<T, m, n> &m1,
                                               const Matrix<T, n, p> &m2)
    {
        Matrix<T, m, p> res{};

        for (size_t i = 0; i < m; i++)
            for (size_t j = 0; j < p; j++)
                for (size_t k = 0; k < n; k++)
                    res[i][j] += m1[i][k] * m2[k][j];

        return res;
    }

    friend constexpr Vector<T, m> operator*(const Matrix<T, m, n> &mat,
                                            const Vector<T, n> &vec)
    {
        Vector<T, m> res{};

        for (size_t i = 0; i < m; i++)
            for (size_t j = 0; j < n; j++)
                res[i] += mat[i][j] * vec[j];

        return res;
    }
};

template <typename T, size_t n>
inline Matrix<T, n, n> translate(Matrix<T, n, n> mat,
                                 const Vector<T, n - 1> &vec)
{
    for (size_t i = 0; i < n - 1; i++)
        mat[i][n - 1] -= vec[i];

    return mat;
}

template <typename T, size_t n>
inline Matrix<T, n, n> scale(Matrix<T, n, n> mat, const Vector<T, n - 1> &vec)
{
    for (size_t i = 0; i < n - 1; i++)
        mat[i][i] *= vec[i];

    return mat;
}

// https://docs.gl/gl3/glRotate
template <typename T>
inline Matrix<T, 4, 4> rotate(const Matrix<T, 4, 4> &mat, T angle,
                              const Vector<T, 3> &axis)
{
    auto a = normalize(axis);

    T c = std::cos(angle);
    T s = std::sin(angle);
    auto t = (T(1) - c) * a;

    // Marix for rotation around an arbitrary axis.
    // clang-format off
    return Matrix<T, 4, 4>
    {
        a.x * t.x + c, a.x * t.y - a.z * s, a.x * t.z + a.y * s, 0,
        a.y * t.x + a.z * s, a.y * t.y + c, a.y * t.z - a.x * s, 0,
        a.x * t.z - a.y * s, a.y * t.z + a.x * s, a.z * t.z + c, 0,
        0, 0, 0, 1,
    } * mat;
    // clang-format on
}

template <typename T>
Matrix<T, 4, 4> look_at(const Vector<T, 3> &pos, const Vector<T, 3> &target,
                        const Vector<T, 3> &up)
{
    // We look into the negative z-direction by convention.
    const auto u = normalize(target - pos);
    const auto v = normalize(cross(up, u));
    const auto w = cross(u, v);

    // clang-format off
    return  Matrix<T, 4, 4>{
        v.x, v.y, v.z, -pos.x,
        w.x, w.y, w.z, -pos.y,
        u.x, u.y, u.z, -pos.z,
        0.f, 0.f, 0.f, 1.f
    };
    // clang-format on
}

// https://docs.gl/gl3/glFrustum
template <typename T>
Matrix<T, 4, 4> frustrum(const T &left, const T &right, const T &bottom,
                         const T &top, const T &near, const T &far)
{
    auto fn = far - near;
    auto rl = right - left;
    auto tb = top - bottom;

    return Matrix<T, 4, 4>{
        // clang-format off
        2.f * near / rl, 0.f, (right + left) / rl, 0.f,
        0.f, 2.f * near / tb, (top + bottom) / tb, 0.f,
        0.f, 0.f, far + near / -fn, -2 * far * near / fn,
        0.f, 0.f, -1.f, 0.f,
        // clang-format on
    };
}

template <typename T>
Matrix<T, 4, 4> perspective(const T &fov, const T &aspect, const T &near,
                            const T &far)
{
    auto top = near * tanf(fov / 2);
    auto right = top * aspect;

    return frustrum(-right, right, -top, top, near, far);
}

// https://docs.gl/gl3/glOrtho
template <typename T>
constexpr Matrix<T, 4, 4> orthographic(const T left, const T right,
                                       const T bottom, const T top,
                                       const T near, const T far)
{
    auto fn = far - near;
    auto rl = right - left;
    auto tb = top - bottom;

    // clang-format off
    return Matrix<T, 4, 4>{
        2.f / rl, 0.f, 0.f, (right + left) / -rl,
        0.f, 2.f / tb, 0.f, (top + bottom) / -tb,
        0.f, 0.f, -2.f / fn, (far + near) / -fn,
        0.f, 0.f, 0.f, 1.f,
    };
    // clang-format on
}

using Mat4 = Matrix<float, 4, 4>;
using IMat4 = Matrix<int, 4, 4>;

using Mat3 = Matrix<float, 3, 3>;
using IMat3 = Matrix<int, 3, 3>;

} // namespace rasterizer