#pragma once
#include <cmath>
namespace utils {

    
inline double round_half_up(double x, int decimals = 4) {
    const double factor = std::pow(10.0, decimals);
    return (x >= 0.0)
        ? std::floor(x * factor + 0.5) / factor
        : std::ceil (x * factor - 0.5) / factor;
}

inline bool is_zcz(int securityid) {
    int first_digit  = securityid / 100000;   // 首位
    int first_two    = securityid / 10000;    // 前两位

    if (first_digit == 3 || first_two == 68) {
        return true;
    } else {
        return false;
    }
    }
    
} // namespace utils