#pragma once

#include <cmath>
#include <numbers> // C++20

// pi
constexpr std::double_t pi = std::numbers::pi;

// 1 / pi
constexpr std::double_t inv_pi = std::numbers::inv_pi;

// 2 * pi
constexpr std::double_t two_pi = std::numbers::pi * 2.;

// 1 / ( 2 * pi )
constexpr std::double_t inv_2pi = std::numbers::inv_pi * 0.5;
