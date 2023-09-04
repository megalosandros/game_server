#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>

namespace geom {

struct CollectionResult {
    bool IsCollected(double collect_radius) const {
        //
        //  Столкновение засчитывается при совпадении двух факторов:
        //  расстояние от предмета до прямой перемещения собирателя не превышает величины w + W, где w — радиус предмета, а W — радиус собирателя,
        //  проекция предмета на прямую перемещения собирателя попадает на отрезок перемещения.
        //
        //  proj_ratio характеризует то, какую долю отрезка нужно пройти до столкновения с выбранной точкой. Если 
        //  proj_ratio < 0, то проекция находилась бы левее точки, если 
        //  proj_ratio > 1, то правее точки B.
        //  Пересечение будет только в том случае, когда proj_ration лежит между 0 и 1:
        //
        if (proj_ratio < 0 || proj_ratio > 1) {
            return false;
        }

        //
        //  collect_radius = w + W
        //  sq_distance = pow(distance, 2)
        //
        return sq_distance <= collect_radius * collect_radius;
    }

    // Квадрат расстояния до точки
    double sq_distance;
    // Доля пройденного отрезка
    double proj_ratio;
};

// Движемся из точки a в точку b и пытаемся подобрать точку c
CollectionResult TryCollectPoint(Point2D a, Point2D b, Point2D c);

struct Item {
    Point2D position;
    double width;
    size_t id;
};

struct Gatherer {
    Point2D start_pos;
    Point2D end_pos;
    double width;
    size_t id;
};

class ItemGathererProvider {
public:
    virtual ~ItemGathererProvider() = default;
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace geom