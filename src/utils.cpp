#include <cmath>
#include <numbers>

#include "utils.hpp"

namespace utils
{

constexpr double radians_per_degree = std::numbers::pi / 180.f;

float radians(float degrees) { return degrees * radians_per_degree; }

float fract(float value) { return std::modf(value, &value); }

} // namespace utils
