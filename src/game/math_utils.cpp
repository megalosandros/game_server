#include "../sdk.h"
#include "math_utils.h"

namespace util {

bool IsEqual(double x, double y, double epsilon /*= std::numeric_limits<double>::epsilon()*/)
{
    return std::fabs(x - y) <= epsilon;
}

bool IsZero(double x, double epsilon /*= std::numeric_limits<double>::epsilon()*/)
{
    return IsEqual(x, 0.0, epsilon);
}

//
//  если горизонтальная скорость равна нулю - собака движется вертикально
//
bool IsVertical(const geom::Vec2D& s)
{
    return IsZero(s.x);
}

//
//  если вертикальная скорость равна нулю - собака движется горизонтально
//
bool IsHorizontal(const geom::Vec2D& s)
{
    return IsZero(s.y);
}

//
//  если высота больше ширины - прямоугольник вертикальный
//
bool IsVertical(const geom::Rect2D& rect)
{
    return (std::fabs(rect.bottom - rect.top) > std::fabs(rect.right - rect.left));
}

//
//  если высота меньше ширины - прямоугольник горизонтальный
//
bool IsHorizontal(const geom::Rect2D& rect)
{
    return (std::fabs(rect.bottom - rect.top) < std::fabs(rect.right - rect.left));
}


bool IsEqual(const geom::Vec2D &s1, const geom::Vec2D &s2, double epsilon /*= std::numeric_limits<double>::epsilon()*/)
{
    return (IsEqual(s1.x, s2.x, epsilon) && IsEqual(s1.y, s2.y, epsilon));
}

bool IsZero(const geom::Vec2D &s, double epsilon /*= std::numeric_limits<double>::epsilon()*/)
{
    return (IsEqual(s, geom::Vec2D{0.0, 0.0}, epsilon));
}

//
//  создать нормализованный прямоугольник из участка дороги
//
geom::Rect2D MakeRect2D(const model::Road &road)
{
    auto start = road.GetStart();
    auto end = road.GetEnd();

    if (end < start) {
        std::swap(start, end);
    }

    return {
        static_cast<double>(start.x) - model::Road::ALIGNMENT,
        static_cast<double>(start.y) - model::Road::ALIGNMENT,
        static_cast<double>(end.x) + model::Road::ALIGNMENT,
        static_cast<double>(end.y) + model::Road::ALIGNMENT
        };
}

double TimeDeltaToSeconds(model::TimeInterval dt) {

    return std::chrono::duration<double>{dt} / std::chrono::seconds{1};
}

} // namespace util

