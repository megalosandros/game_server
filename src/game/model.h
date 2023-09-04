#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <deque>

#include "tagged.h"
#include "loot_generator.h"
#include "model_units.h"
#include "game_session.h"
#include "ticker.h"


namespace model {


class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};
    constexpr static Real WIDTH = 0.8;
    constexpr static Real ALIGNMENT = 0.4;

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using  Id = util::Tagged<std::string, Office>;
    static constexpr Real WIDTH = 0.5;


    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
    // запрещаю лишние операции копирования и перемещения - вообще цель
    // добиться того, чтобы Map можно было поместить только в "стабильный" контейнер
    Map(const Map &) = delete;
    Map &operator=(const Map &) = delete;
    Map &operator=(Map &&) = delete;

public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, Real dog_speed, size_t bag_capacity) noexcept;

    // пришлось оставить - иначе никак
    Map(Map &&) noexcept = default;

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    Real GetDogSpeed() const noexcept {
        return dog_speed_;
    }

    size_t GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    size_t GetLootTypeCount() const noexcept {
        return loot_values_.size();
    }

    Loot::Value GetLootTypeValue(Loot::Type type) const noexcept {
        return (type < loot_values_.size()) ? loot_values_.at(type) : 0;
    }

    const std::string& GetFrontendData() const noexcept {
        return frontend_loot_types_;
    }

    bool operator==(const Map& other) const noexcept {
        return (id_ == other.id_);
    }

    void AddRoad(Road&& road);

    void AddBuilding(Building&& building);

    void AddOffice(Office&& office);

    void AddLoot(Loot::Value&& loot_value);

    void AddFrontendData(std::string&& loot_types);

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    Real dog_speed_;
    size_t bag_capacity_;
    std::vector<Loot::Value> loot_values_;
    std::string frontend_loot_types_;
};

class Game {
public:
    using Ptr = std::shared_ptr<Game>;
    using Maps = std::deque<Map>;

    Game(TimeInterval base_interval, Real probability, TimeInterval dog_retirement_time);

    void SetTicker(Ticker::Ptr ticker);

    void AddMap(Map &&map);
    const Map* FindMap(const Map::Id &id) const noexcept;
    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    GameSession *FindSession(const Map::Id &id) noexcept;
    GameSession *AddSession(const Map::Id &id);
    GameSession *AddSession(const Map &map);

    TimeInterval GetRetirementTime() const noexcept {
        return dog_retirement_time_;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using SessionIdHasher = util::TaggedHasher<Map::Id>;
    using Sessions = std::unordered_map<Map::Id, GameSession, SessionIdHasher>;

    Maps maps_;
    MapIdToIndex map_id_to_index_;
    Sessions sessions_;
    Ticker::Ptr ticker_;
    TimeInterval base_interval_;
    Real probability_;
    TimeInterval dog_retirement_time_;
};

}  // namespace model
