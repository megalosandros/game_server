#pragma once
#include <string_view>
#include <optional>
#include <boost/beast/http/status.hpp>

#include "model.h"
#include "player.h"
#include "dogs_collector.h"
#include "postgres.h"

namespace app {

namespace http  = boost::beast::http;
using namespace   std::literals;

//
//  Обработка ошибок и возвращаемые данные для сценариев работы
//

struct ErrorReason {
    ErrorReason() = delete;

    constexpr static std::string_view MAP_NOT_FOUND = "{\n\"code\": \"mapNotFound\",\n\"message\": \"Map not found\"\n}"sv;
    constexpr static std::string_view INVALID_NAME = "{\n\"code\": \"invalidArgument\",\n\"message\": \"Invalid name\"\n}"sv;
    constexpr static std::string_view PARSE_ERROR = "{\n\"code\": \"invalidArgument\",\n\"message\": \"Join game request parse error\"\n}"sv;
    constexpr static std::string_view UNKNOWN_TOKEN = "{\n\"code\": \"unknownToken\",\n\"message\": \"Player token has not been found\"\n}"sv;
};

class Error : public std::exception
{
public:
    Error(http::status status, std::string_view message);

    const char* what() const noexcept override;
    http::status status() const noexcept;

private:
    http::status status_;
    std::string message_;
};

struct JoinGameRequest
{
    JoinGameRequest(const std::string &name, const model::Map::Id &id);

    std::string name;
    model::Map::Id id;
};

struct JoinGameResult
{
    JoinGameResult(const Token &token, const Player::Id &id);

    Token token;
    Player::Id id;
};

using PlayersResult = std::vector<std::pair<Player::Id, std::string>>;


struct StateResult {
    using Players = std::vector<std::tuple<model::Dog::Id, geom::Point2D, geom::Vec2D, model::Dog::Direction, model::Dog::Bag, model::Dog::Score>>;
    using Loots = std::vector<std::tuple<model::Loot::Id, model::Loot::Type, geom::Point2D>>;

    static constexpr size_t DogId = 0;
    static constexpr size_t DogPos = 1;
    static constexpr size_t DogSpeed = 2;
    static constexpr size_t DogDir = 3;
    static constexpr size_t DogBag = 4;
    static constexpr size_t DogScore = 5;

    static constexpr size_t LootId = 0;
    static constexpr size_t LootType = 1;
    static constexpr size_t LootPos = 2;

    Players players;
    Loots loots;
};

using RecordsResult = std::vector<PlayerStatistics>;


//
//  Сценарии работы (реализация REST API запросов к серверу)
//
namespace use_case {

//
//  Получить список карт
//
class UseCaseMapsList {
public:
    explicit UseCaseMapsList(model::Game::Ptr game);

    const model::Game::Maps &RunUseCase();

private:
    model::Game::Ptr game_;
};

//
//  Получить информацию по одной заданной карте
//
class UseCaseMapInfo {
public:
    explicit UseCaseMapInfo(model::Game::Ptr game);

    const model::Map& RunUseCase(const model::Map::Id &id);

private:
    model::Game::Ptr game_;
};


//
//  Вход игрока в игру, получение токена авторизации
//
class UseCaseJoinGame {
public:
    UseCaseJoinGame(model::Game::Ptr game, Players::Ptr players, bool randomize_spawn_points);

    JoinGameResult RunUseCase(const std::string &name, const model::Map::Id& mapId);

private:
    model::Game::Ptr game_;
    Players::Ptr players_;
    bool randomize_spawn_points_;
};

//
//  Список игроков 
//
class UseCasePlayers {
public:
    explicit UseCasePlayers(Players::Ptr players);

    PlayersResult RunUseCase(const Token& token);

private:
    Players::Ptr players_;
};

//
//  Состояние игры (игроки, их координаты, находки, счет каждого игрока и т.д.)
//
class UseCaseState {
public:
    explicit UseCaseState(Players::Ptr players);

    StateResult RunUseCase(const Token& token);

private:
    Players::Ptr players_;
};

//
//  Смена направления движения игрока (его собаки)
//
class UseCaseAction {
public:
    explicit UseCaseAction(Players::Ptr players);

    void RunUseCase(const Token& token, model::Dog::Direction dir);

private:
    Players::Ptr players_;
};

//
//  Увеличение игрового времени на delta time
//
class UseCaseTick {
public:
    UseCaseTick(model::Game::Ptr game, Players::Ptr players);

    void RunUseCase(model::TimeInterval timeDelta);

private:
    model::Game::Ptr game_;
    Players::Ptr players_;
};

//
//  Список призеров игры
//
class UseCaseRecords {
public:
    explicit UseCaseRecords(postgres::Database& db);

    RecordsResult RunUseCase(int start, int maxItems);

private:
    postgres::Database& db_;
};

} // namespace use_case

//
//  Паттерн проектирования «Наблюдатель» (Observer) позволяет объекту
//  (в нашем случае Application) уведомлять другие объекты об изменении своего состояния.
//  При этом единственное, что объект знает о других объектах, — это то,
//  что они реализуют некоторый интерфейс (в нашем случае ApplicationListener).
//
class ApplicationListener {
public:
    using Ptr = std::unique_ptr<ApplicationListener>;
    using List = std::vector<Ptr>;

    virtual ~ApplicationListener() = default;

    virtual void OnTick(model::TimeInterval timeDelta) noexcept = 0;
};

//
//  "Приложение" - паттерн Фасад который предоставляет единый интерфейс ко всем игровым сценариям
//
class Application {
public:
    using Ptr = std::shared_ptr<Application>;

    Application(model::Game::Ptr game, postgres::Database& db, bool randomize_spawn_points);

    const model::Game::Maps &GetMaps();
    const model::Map &GetMap(const model::Map::Id &id);
    JoinGameResult JoinGame(const std::string &name, const model::Map::Id& mapId);
    PlayersResult GetPlayers(const Token &token);
    StateResult GetState(const Token &token);
    RecordsResult GetRecords(int start, int maxItems);
    void RotateDog(const Token &token, model::Dog::Direction dir);
    void Tick(model::TimeInterval timeDelta);
    void AddPlayer(Token token, Player&& player);
    void AddListener(ApplicationListener::Ptr listener);
    Players::Pairs GetTokensPlayers() const; // нужно только для сериализации

private:
    // конструктор, который создает временные параметры, которые нужны только на момент создания объекта
    Application(model::Game::Ptr game, Players::Ptr players, postgres::Database& db, bool randomize_spawn_points);

    std::mutex                game_state_lock_;
    Players::Ptr              players_;
    use_case::UseCaseMapsList use_case_maps_list_;
    use_case::UseCaseMapInfo  use_case_map_info_;
    use_case::UseCaseJoinGame use_case_join_game_;
    use_case::UseCasePlayers  use_case_players_;
    use_case::UseCaseState    use_case_state_;
    use_case::UseCaseAction   use_case_action_;
    use_case::UseCaseTick     use_case_tick_;
    use_case::UseCaseRecords  use_case_records_;
    collector::DogsCollector  dogs_collector_;
    ApplicationListener::List listeners_;
};

} // namespace app
