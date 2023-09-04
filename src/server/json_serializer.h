#pragma once

#include "../game/model.h"
#include "../game/app.h"

namespace json_serializer {

//
//  Сериализация объектов игры в JSON - для передачи клиенту через REST API
//
std::string SerializeMapList(const model::Game::Maps& maps);
std::string SerializeMap(const model::Map& map);

std::string SerializeJoinResult(const app::JoinGameResult &token_and_id);
std::string SerializePlayersResult(const app::PlayersResult &players);
std::string SerializeStateResult(const app::StateResult &state);
std::string SerializeRecordsResult(const app::RecordsResult &records);

}  // namespace json_serializer
