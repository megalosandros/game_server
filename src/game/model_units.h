#pragma once
#include <cstdint>
#include <chrono>

namespace model {

using TimeInterval = std::chrono::milliseconds;


using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

inline bool operator<(const Point& a, const Point& b) {
    return (a.x <= b.x && a.y <= b.y);
}

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

using Real = double;

struct LootGeneratorConfig {
    TimeInterval period;
    Real probability;
};

} // namespace model