#pragma once
#include <boost/json/string_view.hpp>

//
//  Сложу здесь имена атрибутов для json конфигурации и REST API ответов
//  
struct JsonTag {
    
    JsonTag() = delete;

    static constexpr boost::json::string_view X    = "x";
    static constexpr boost::json::string_view X0   = "x0";
    static constexpr boost::json::string_view X1   = "x1";
    static constexpr boost::json::string_view Y    = "y";
    static constexpr boost::json::string_view Y0   = "y0";
    static constexpr boost::json::string_view Y1   = "y1";
    static constexpr boost::json::string_view W    = "w";
    static constexpr boost::json::string_view H    = "h";
    static constexpr boost::json::string_view MAPS = "maps";
    static constexpr boost::json::string_view ID   = "id";
    static constexpr boost::json::string_view NAME = "name";
    static constexpr boost::json::string_view TYPE = "type";
    static constexpr boost::json::string_view VALUE = "value";
    static constexpr boost::json::string_view ROADS     = "roads";
    static constexpr boost::json::string_view BUILDINGS = "buildings";
    static constexpr boost::json::string_view OFFICES   = "offices";
    static constexpr boost::json::string_view OFFSETX   = "offsetX";
    static constexpr boost::json::string_view OFFSETY   = "offsetY";
    static constexpr boost::json::string_view DEFAULT_DOG_SPEED = "defaultDogSpeed";
    static constexpr boost::json::string_view DOG_SPEED  = "dogSpeed";
    static constexpr boost::json::string_view DOG_RETIREMENT_TIME = "dogRetirementTime";
static constexpr boost::json::string_view LOOT_TYPES = "lootTypes";
    static constexpr boost::json::string_view DEFAULT_BAG_CAPACITY = "defaultBagCapacity";
    static constexpr boost::json::string_view BAG_CAPACITY = "bagCapacity";
    
    static constexpr boost::json::string_view LOOT_GENERATOR_CONFIG = "lootGeneratorConfig";
    static constexpr boost::json::string_view PERIOD      = "period";
    static constexpr boost::json::string_view PROBABILITY = "probability";

    static constexpr boost::json::string_view USER_NAME  = "userName";
    static constexpr boost::json::string_view MAP_ID     = "mapId";
    static constexpr boost::json::string_view AUTH_TOKEN = "authToken";
    static constexpr boost::json::string_view PLAYER_ID  = "playerId";
    static constexpr boost::json::string_view PLAYERS    = "players";
    static constexpr boost::json::string_view MOVE       = "move";
    static constexpr boost::json::string_view TIME_DELTA = "timeDelta";
    static constexpr boost::json::string_view LOST_OBJECTS = "lostObjects";

    static constexpr boost::json::string_view SCORE = "score";
    static constexpr boost::json::string_view PLAY_TIME = "playTime";

}; // JsonTag

//
//  Названия операций HTTP
//
struct JsonVerb {

    JsonVerb() = default;

    static constexpr boost::json::string_view GET    = "GET";
    static constexpr boost::json::string_view HEAD   = "HEAD";
    static constexpr boost::json::string_view POST   = "POST";
    static constexpr boost::json::string_view DELETE = "DELETE";
    static constexpr boost::json::string_view PUT    = "PUT";
    static constexpr boost::json::string_view PATCH  = "PATCH";
    static constexpr boost::json::string_view UNK    = "UNKNOWN";

}; // JsonVerb
