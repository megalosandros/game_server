#include "../sdk.h"
#include <stdexcept>
#include "model.h"


namespace model {
using namespace std::literals;

Map::Map(Id id, std::string name, Real dog_speed, size_t bag_capacity) noexcept
: id_(std::move(id))
, name_(std::move(name))
, dog_speed_(dog_speed)
, bag_capacity_(bag_capacity) {
}

void Map::AddOffice(Office&& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse "s + *office.GetId());
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

 void Map::AddRoad(Road&& road) {
    roads_.emplace_back(std::move(road));
}

void Map::AddBuilding(Building&& building) {
    buildings_.emplace_back(std::move(building));
}

void Map::AddLoot(Loot::Value&& loot_value) {
    loot_values_.emplace_back(std::move(loot_value));
}

void Map::AddFrontendData(std::string&& loot_types) {
    frontend_loot_types_.assign(std::move(loot_types));
}

Game::Game(TimeInterval base_interval, Real probability, TimeInterval dog_retirement_time)
: base_interval_{base_interval}
, probability_{probability} 
, dog_retirement_time_{dog_retirement_time} {
}

void Game::AddMap(Map&& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

void Game::SetTicker(Ticker::Ptr ticker) {
    ticker_ = ticker;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::FindSession(const Map::Id &id) noexcept
{
    if (auto it = sessions_.find(id); it != sessions_.end()) {
        return &it->second;
    }

    return nullptr;
}

GameSession *Game::AddSession(const Map::Id &id) {
    const Map* map = FindMap(id);
    if (map) {
        return AddSession(*map);
    }

    return nullptr;
}

GameSession *Game::AddSession(const Map &map) {

    auto [it, inserted] = sessions_.try_emplace(map.GetId(), map, base_interval_, probability_);

    //
    //  если для этой карты уже есть сессия - не считаю ошибкой
    //

    return &it->second;
}


}  // namespace model
