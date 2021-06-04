#pragma once

namespace kawe {

template<typename T>
struct Rect4 {
    T x, y, w, h;

    template<typename U>
    constexpr auto contains(const glm::vec<2, U> &point) -> bool
    {
        return point.x >= x && point.x <= x + w && point.y >= y && point.y <= y + h;
    }
};

} // namespace kawe
