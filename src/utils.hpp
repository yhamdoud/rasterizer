#pragma once

namespace utils
{

float radians(float degrees);

template <typename T> bool in_bounds(const T &val, const T &low, const T &high)
{
    return low < val && val < high;
}

// Return fractial part of number, ignoring the sign.
float fract(float value);

} // namespace utils