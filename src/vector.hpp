// Generic n-dimensional vector implementation inspired by
// https://www.reedbeta.com/blog/on-vector-math-libraries/.

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <ostream>
#include <utility>

using std::array;

namespace rasterizer
{

// We use CRTP to avoid code duplication when writing template specializations.
// Operations are hidden friends to support heterogeneous arithmetic through
// implicit type conversion of arguments.
template <typename Vec, typename T> struct VectorBase
{
    Vec &self() { return static_cast<Vec &>(*this); }
    const Vec &self() const { return static_cast<const Vec &>(*this); }

    // Iterators
    constexpr auto begin() const noexcept { return self().data.begin(); }
    constexpr auto end() const noexcept { return self().data.end(); }
    constexpr auto cbegin() const noexcept { return self().data.cbegin(); }
    constexpr auto cend() const noexcept { return self().data.cend(); }

    constexpr auto &operator[](size_t i) { return self().data[i]; }
    constexpr const auto &operator[](size_t i) const { return self().data[i]; }

    friend constexpr std::ostream &operator<<(std::ostream &out, const Vec &v)
    {
        out << "{ ";

        for (auto &e : v)
            // Print as number.
            out << +e << " ";

        out << "}";

        return out;
    }

    friend constexpr bool operator==(const Vec &v1, const Vec &v2)
    {
        for (size_t i = 0; i < Vec::size; i++)
            if (v1[i] != v2[i])
                return false;

        return true;
    }

    friend constexpr bool operator!=(const Vec &v1, const Vec &v2)
    {
        return !(v1 == v2);
    }

    // Vector arithmetic

    constexpr Vec &operator+=(const Vec &v)
    {
        for (size_t i = 0; i < Vec::size; i++)
            self()[i] += v[i];

        return self();
    }

    constexpr Vec &operator-=(const Vec &v)
    {
        for (size_t i = 0; i < Vec::size; i++)
            self()[i] -= v[i];

        return self();
    }

    constexpr Vec &operator*=(const Vec &v)
    {
        for (size_t i = 0; i < Vec::size; i++)
            self()[i] *= v[i];

        return self();
    }

    constexpr Vec &operator/=(const Vec &v)
    {
        for (size_t i = 0; i < Vec::size; i++)
            self() /= v[i];

        return self();
    }

    friend constexpr Vec operator+(Vec v1, const Vec &v2) { return v1 += v2; }
    friend constexpr Vec operator-(Vec v1, const Vec &v2) { return v1 -= v2; }
    friend constexpr Vec operator*(Vec v1, const Vec &v2) { return v1 *= v2; }
    friend constexpr Vec operator/(Vec v1, const Vec &v2) { return v1 /= v2; }

    friend constexpr Vec operator-(Vec v) { return T{-1} * v; }

    // Scalar arithmetic

    constexpr Vec &operator*=(const T &s)
    {
        for (size_t i = 0; i < Vec::size; i++)
            self()[i] *= s;

        return self();
    }

    constexpr Vec &operator/=(const T &s)
    {
        for (size_t i = 0; i < Vec::size; i++)
            self()[i] /= s;

        return self();
    }

    friend constexpr Vec operator*(Vec v, const T &s) { return v *= s; }
    friend constexpr Vec operator*(const T &s, Vec v) { return v *= s; }
    friend constexpr Vec operator/(Vec v, const T &s) { return v /= s; }

    // Non-operators

    constexpr auto &min() const
    {
        return *std::min(self().begin(), self().end());
    }

    constexpr auto &max() const
    {
        return *std::max(self().begin(), self().end());
    }

    constexpr auto magnitude() const { return std::sqrt(dot(self(), self())); }

    // TODO: Handle divide by zero?
    friend constexpr auto normalize(Vec v) { return v / v.magnitude(); }

    friend constexpr auto dot(Vec v1, const Vec &v2)
    {
        T product_sum{};

        // Dot product is the sum of the product of corresponding elements.
        for (size_t i = 0; i < Vec::size; i++)
            product_sum += v1[i] * v2[i];

        return product_sum;
    }

    // Linearly interpolate between two vectors.
    friend constexpr auto lerp(const Vec &v1, const Vec &v2, float t)
    {
        return (1 - t) * v1 + t * v2;
    }
};

template <typename T, size_t n> struct Vector : VectorBase<Vector<T, n>, T>
{
    static constexpr int size = n;

    array<T, n> data;

    constexpr Vector() = default;
};

template <typename T> struct Vector<T, 2> : VectorBase<Vector<T, 2>, T>
{
    static constexpr int size = 2;

    constexpr Vector() = default;

    constexpr Vector(const T &e) : data{e, e} {}

    constexpr Vector(const T &e1, const T &e2) : data{e1, e2} {}

    operator Vector<float, size>() requires(std::is_same_v<T, int>)
    {
        return Vector<float, size>{static_cast<float>(this->x),
                                   static_cast<float>(this->y)};
    }

    explicit operator Vector<int, size>() requires(std::is_same_v<T, float>)
    {
        return Vector<int, size>{static_cast<int>(this->x),
                                 static_cast<int>(this->y)};
    }

    // Swap x and y components.
    constexpr Vector swap() { return Vector(this->y, this->x); }

    union
    {
        array<T, size> data;

        struct
        {
            T x, y;
        };
    };
};

template <typename T> struct Vector<T, 3> : VectorBase<Vector<T, 3>, T>
{
    static constexpr int size = 3;

    constexpr Vector(const T &e1, const T &e2, const T &e3) : data{e1, e2, e3}
    {
    }

    constexpr Vector(const T &e) : data{e, e, e} {}
    constexpr Vector(const T &v, const T &e) : data{v.x, v.y, e} {}

    operator Vector<float, size>() requires(std::is_same_v<T, int>)
    {
        return Vector<float, size>{static_cast<float>(this->x),
                                   static_cast<float>(this->y),
                                   static_cast<float>(this->z)};
    }

    explicit operator Vector<int, size>() requires(std::is_same_v<T, float>)
    {
        return Vector<int, size>{static_cast<int>(this->x),
                                 static_cast<int>(this->y),
                                 static_cast<int>(this->z)};
    }

    friend constexpr auto cross(const Vector &v1, const Vector &v2)
    {
        return Vector{
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z,
            v1.x * v2.y - v1.y * v2.x,
        };
    }

    static Vector<T, 3> up() { return Vector{T{0}, T{1}, T{0}}; };
    static Vector<T, 3> forward() { return Vector{T{0}, T{0}, T{1}}; };
    static Vector<T, 3> left() { return Vector{T{1}, T{0}, T{0}}; };

    union
    {
        array<T, size> data;

        struct
        {
            T x, y, z;
        };

        struct
        {
            T r, g, b;
        };

        Vector<T, 2> xy;
    };
};

template <typename T> struct Vector<T, 4> : VectorBase<Vector<T, 4>, T>
{
    using type = T;
    static constexpr int size = 4;

    constexpr Vector() : data{} {}

    constexpr Vector(const T &e) : data{e, e, e, e} {}

    constexpr Vector(const T &e1, const T &e2, const T &e3, const T &e4)
        : data{e1, e2, e3, e4}
    {
    }

    operator Vector<float, size>() requires(std::is_same_v<T, int>)
    {
        return Vector<float, size>{
            static_cast<float>(this->x), static_cast<float>(this->y),
            static_cast<float>(this->z), static_cast<float>(this->w)};
    }

    explicit operator Vector<int, size>() requires(std::is_same_v<T, float>)
    {
        return Vector<int, size>{
            static_cast<int>(this->x), static_cast<int>(this->y),
            static_cast<int>(this->z), static_cast<int>(this->w)};
    }

    constexpr Vector(const Vector<T, 3> &v, const T &e) : data{v.x, v.y, v.z, e}
    {
    }

    union
    {
        array<T, 4> data;

        struct
        {
            T x, y, z, w;
        };

        struct
        {
            T r, g, b, a;
        };

        Vector<T, 2> xy;
        Vector<T, 3> xyz;
        Vector<T, 3> rgb;
    };
};

// Shorthands
using Vec2 = Vector<float, 2>;
using Vec3 = Vector<float, 3>;
using Vec4 = Vector<float, 4>;

using IVec2 = Vector<int, 2>;
using IVec3 = Vector<int, 3>;
using IVec4 = Vector<int, 4>;

using Color = Vec4;

namespace colors
{

constexpr Color white = Color{1.f, 1.f, 1.f, 1.f};
constexpr Color black = Color{0, 0, 0, 1.f};

constexpr Color red = Color{1.f, 0, 0, 1.f};
constexpr Color green = Color{0, 1.f, 0, 1.f};
constexpr Color blue = Color{0, 0, 1.f, 1.f};

} // namespace colors

} // namespace rasterizer