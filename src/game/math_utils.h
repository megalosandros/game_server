#pragma once
#include <limits>
#include <cmath>
#include "model.h"
#include "geom.h"

namespace util {

bool IsEqual(double x, double y, double epsilon = std::numeric_limits<double>::epsilon());
bool IsZero(double x, double epsilon = std::numeric_limits<double>::epsilon());

bool IsEqual(const geom::Vec2D &s1, const geom::Vec2D &s2, double epsilon = std::numeric_limits<double>::epsilon());
bool IsZero(const geom::Vec2D &s, double epsilon = std::numeric_limits<double>::epsilon());
bool IsVertical(const geom::Vec2D& s);
bool IsHorizontal(const geom::Vec2D& s);
bool IsVertical(const geom::Rect2D& rect);
bool IsHorizontal(const geom::Rect2D& rect);

//
//  создать нормализованный прямоугольник из участка дороги
//
geom::Rect2D MakeRect2D(const model::Road &road);

double TimeDeltaToSeconds(model::TimeInterval dt);

} // namespace util
