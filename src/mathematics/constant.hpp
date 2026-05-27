#pragma once

#include <cmath>
#include <limits>
#include <numbers> // C++20

// pi
constexpr std::double_t pi = std::numbers::pi;

// 1 / pi
constexpr std::double_t inv_pi = std::numbers::inv_pi;

// 2 * pi
constexpr std::double_t two_pi = std::numbers::pi * 2.;

// 1 / ( 2 * pi )
constexpr std::double_t inv_2pi = std::numbers::inv_pi * 0.5;

// Quiet NaN (will not throw exception)
constexpr std::double_t NaN = std::numeric_limits<std::double_t>::quiet_NaN();
