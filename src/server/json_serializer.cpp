#include "../sdk.h"
#include <boost/json.hpp>
#include <boost/json/string_view.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string_view>

#include "json_tags.h"
#include "json_serializer.h"

namespace json_serializer {

using namespace std::literals;
namespace json = boost::json;


//
//  Сериализую заголовок карты (ид + имя) в json::value
//
json::object SerializeObject(const model::Map& map)
{
    const auto &mapId = map.GetId();
    const auto &mapName = map.GetName();

    json::object jsonObject;

    jsonObject[JsonTag::ID] = json::value_from(*mapId);
    jsonObject[JsonTag::NAME] = json::value_from(mapName);

    return jsonObject;
}

//
//  Координаты одной дороги - в json::object
//
json::object SerializeObject(const model::Road& road)
{
    //
    //  всегда добавить x0, y0, а дальше в зависимости от направления
    //  дороги - либо x1, либо y1
    //
    const auto &start = road.GetStart();
    const auto &end = road.GetEnd();

    json::object serializedRoad;

    serializedRoad[JsonTag::X0] = json::value_from(start.x);
    serializedRoad[JsonTag::Y0] = json::value_from(start.y);
    if (road.IsHorizontal()) {
        //
        //  в горизонтальной дороге заданы x0 и x1
        //
        serializedRoad[JsonTag::X1] = json::value_from(end.x);
    }
    else {
        //
        //  интересно, а если дорога не горизонтальная - то какая?
        //  я надеюсь, что вертикальная (y0 и y1)
        //
        serializedRoad[JsonTag::Y1] = json::value_from(end.y);
    }

    return serializedRoad;
}

//
//  Координаты (x, y, w, h) одного здания в json::object
//
json::object SerializeObject(const model::Building& building)
{
    const auto &bounds = building.GetBounds();

    json::object serializedBuilding;

    serializedBuilding[JsonTag::X] = json::value_from(bounds.position.x);
    serializedBuilding[JsonTag::Y] = json::value_from(bounds.position.y);
    serializedBuilding[JsonTag::W] = json::value_from(bounds.size.width);
    serializedBuilding[JsonTag::H] = json::value_from(bounds.size.height);

    return serializedBuilding;
}

//
//  Офис - координаты, смещение, ид в json::object
//
json::object SerializeObject(const model::Office& office)
{
    const auto &position = office.GetPosition();
    const auto &offset = office.GetOffset();
    const auto &id = *office.GetId();

    json::object serializedOffice;

    serializedOffice[JsonTag::ID] = json::value_from(id);
    serializedOffice[JsonTag::X] = json::value_from(position.x);
    serializedOffice[JsonTag::Y] = json::value_from(position.y);
    serializedOffice[JsonTag::OFFSETX] = json::value_from(offset.dx);
    serializedOffice[JsonTag::OFFSETY] = json::value_from(offset.dy);

    return serializedOffice;
}

json::object SerializeObject(const model::Loot::Traits& bagItem)
{
    json::object serializedBagItem;

    serializedBagItem[JsonTag::ID] = json::value_from(bagItem.id);
    serializedBagItem[JsonTag::TYPE] = json::value_from(bagItem.type);

    return serializedBagItem;
}

json::object SerializeObject(const app::RecordsResult::value_type& record)
{
    json::object serializedRecord;

    serializedRecord[JsonTag::NAME] = record.name;
    serializedRecord[JsonTag::SCORE] = record.score;
    serializedRecord[JsonTag::PLAY_TIME] = std::chrono::duration<double>{record.play_time_ms} / std::chrono::seconds{1};//static_cast<double>(record.play_time_ms.count()) / 1000.0;

    return serializedRecord;
}

//
//  Список объектов всегда сериализуется по одному шаблону,
//  поэтому делаю template
//
template <typename ObjectList>
json::array SerializeObjects(const ObjectList& objects)
{
    json::array jsonObjects;

    for (const auto &o : objects) {
        //
        //  последовательно сериализую в json::object каждый объект из списка
        //  и помещаю (через move) в массив json::array
        //
        jsonObjects.emplace_back(SerializeObject(o));
    }

    return jsonObjects;
}


//
//  Список карт (ид + имя карты) в строку JSON
//
std::string SerializeMapList(const model::Game::Maps& maps)
{
    json::array jsonMapList = SerializeObjects(maps);

    return json::serialize(jsonMapList);
}

//
//  Сериализация в JSON - строку полной карты по ид.
//
std::string SerializeMap(const model::Map& map)
{
    //
    //  заголовок я уже умею сериализовать
    //
    json::object jsonMapObject = SerializeObject(map);

    //
    //  теперь к заголовку нужно добавить дороги, здания, офисы
    //
    jsonMapObject[JsonTag::ROADS] = SerializeObjects(map.GetRoads());
    jsonMapObject[JsonTag::BUILDINGS] = SerializeObjects(map.GetBuildings());
    jsonMapObject[JsonTag::OFFICES] = SerializeObjects(map.GetOffices());

    //
    //  и еще потерянные вещи
    //
    jsonMapObject[JsonTag::LOOT_TYPES] = json::parse(map.GetFrontendData());

    return json::serialize(jsonMapObject);
}

//
//  Сериализую результат операции Вход в игру
//
json::object SerializeObject(const app::JoinGameResult &token_and_id)
{
    json::object jsonObject;

    jsonObject[JsonTag::AUTH_TOKEN] = json::value_from(*token_and_id.token);
    jsonObject[JsonTag::PLAYER_ID] = json::value_from(token_and_id.id);

    return jsonObject;
}

std::string SerializeJoinResult(const app::JoinGameResult &token_and_id)
{
//    json::value jsonTokenAndId{ {"authToken", token_and_id.token}, {"playerId", token_and_id.id} };
    auto jsonTokenAndId = SerializeObject(token_and_id);

    return json::serialize(jsonTokenAndId);
}

std::string SerializePlayersResult(const app::PlayersResult &players)
{
    json::object jsonObject;

    for (const auto& player : players) {
        std::string id = std::to_string(player.first);

        jsonObject[id] = {{"name", player.second}};
    }

    return json::serialize(jsonObject);
}

std::string SerializeStateResult(const app::StateResult &state)
{
    json::object jsonPlayers;

    for (const auto& player : state.players) {
        std::string id = std::to_string(std::get<app::StateResult::DogId>(player));
        json::array pos = { std::get<app::StateResult::DogPos>(player).x, std::get<app::StateResult::DogPos>(player).y };
        json::array speed = { std::get<app::StateResult::DogSpeed>(player).x, std::get<app::StateResult::DogSpeed>(player).y };
        std::string dir = std::string(1, static_cast<char>(std::get<app::StateResult::DogDir>(player)));
        json::array bag = SerializeObjects(std::get<app::StateResult::DogBag>(player));
        auto score = std::get<app::StateResult::DogScore>(player);

        jsonPlayers[id] = {{"pos", pos}, {"speed", speed}, {"dir", dir}, {"bag", bag}, {"score", score}};
    }

    json::object jsonLoots;

    for (const auto& loot : state.loots) {
        std::string id = std::to_string(std::get<0>(loot));
        model::Loot::Type type = std::get<1>(loot);
        json::array pos = {std::get<2>(loot).x, std::get<2>(loot).y};

        jsonLoots[id] = {{"type", type}, {"pos", pos}};
    }

    json::object jsonResult;

    jsonResult[JsonTag::PLAYERS] = jsonPlayers;
    jsonResult[JsonTag::LOST_OBJECTS] = jsonLoots;

    return json::serialize(jsonResult);
}

std::string SerializeRecordsResult(const app::RecordsResult &records) {

    json::array jsonPlayers = SerializeObjects(records);

    return json::serialize(jsonPlayers);
}


}  // namespace json_serializer
