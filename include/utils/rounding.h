#pragma once
#include <cmath>
namespace utils {

    
inline double round_half_up(double x, int decimals = 4) {
    const double factor = std::pow(10.0, decimals);
    return (x >= 0.0)
        ? std::floor(x * factor + 0.5) / factor
        : std::ceil (x * factor - 0.5) / factor;
}

} // namespace utils