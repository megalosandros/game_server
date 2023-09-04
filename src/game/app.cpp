#include "../sdk.h"
#include "app.h"

namespace app {

//
//  Возможные ошибки при обработке сценариев, результаты работы сценариев
//

Error::Error(http::status status, std::string_view message)
: status_(status)
, message_(message) {
}

const char* Error::what() const noexcept
{
    return message_.c_str();
}

http::status Error::status() const noexcept
{
    return status_;
}

JoinGameResult::JoinGameResult(const Token &token, const Player::Id &id)
: token(token)
, id(id) {    
}

JoinGameRequest::JoinGameRequest(const std::string &name, const model::Map::Id &id)
: name(name)
, id(id) {
}


//
//  Сценарии работы (реализация REST API запросов к серверу)
//
namespace use_case {

//
//  Получить список карт
//
UseCaseMapsList::UseCaseMapsList(model::Game::Ptr game)
: game_(game) {
}

const model::Game::Maps& UseCaseMapsList::RunUseCase()
{
    return game_->GetMaps();
}


//
//  Получить информацию по одной заданной карте
//
UseCaseMapInfo::UseCaseMapInfo(model::Game::Ptr game)
: game_(game) {
}

const model::Map& UseCaseMapInfo::RunUseCase(const model::Map::Id &id)
{
    const auto* map = game_->FindMap(id);
    if (!map) {
        throw Error(http::status::not_found, ErrorReason::MAP_NOT_FOUND);
    }

    return *map;
}


//
//  Вход игрока в игру, получение токена авторизации
//
UseCaseJoinGame::UseCaseJoinGame(model::Game::Ptr game, Players::Ptr players, bool randomize_spawn_points)
: game_(game)
, players_(players) 
, randomize_spawn_points_(randomize_spawn_points) {
}


JoinGameResult UseCaseJoinGame::RunUseCase(const std::string &name, const model::Map::Id& mapId)
{
    //
    //  Если было передано пустое имя игрока, должен вернуться ответ со статус-кодом 400 Bad request
    //  Поле code — строка "invalidArgument"
    //
    if (name.empty()) {
        throw Error(http::status::bad_request, ErrorReason::INVALID_NAME);
    }

    const auto *map = game_->FindMap(mapId);

    //
    //  Если в качестве mapId указан несуществующий id карты,
    //  должен вернуться ответ со статус-кодом 404 Not found
    //
    if (!map) {
        throw Error(http::status::not_found, ErrorReason::MAP_NOT_FOUND);
    }

    auto *session = game_->FindSession(mapId);

    //
    //  На самом деле такого не должно быть никогда - это какая-то 
    //  серьезная логическая ошибка
    //
    if (!session) {
        if ((session = game_->AddSession(*map)) == nullptr) {
            throw Error(http::status::not_found, ErrorReason::MAP_NOT_FOUND);
        }
    }

    auto *dog = session->AddDog(name, randomize_spawn_points_);

    Token token = PlayerTokens::MakeToken();

    players_->AddPlayer(token, *session, dog->GetId());

    return {token, dog->GetId()};
}


//
//  Список игроков 
//
UseCasePlayers::UseCasePlayers(Players::Ptr players)
: players_(players) {
}

PlayersResult UseCasePlayers::RunUseCase(const Token& token)
{
    PlayersResult result;

    auto *myself = players_->FindPlayer(token);

    if (!myself) {
        throw Error(http::status::unauthorized, ErrorReason::UNKNOWN_TOKEN);
    }

    const auto &players = players_->GetPlayers();
    const auto &mymap = myself->GetMap();

    for (const auto& player : players) {
        //
        //  Получить список игроков, находящихся в одной (!!!) игровой сессии с игроком
        //
        if (mymap == player.GetMap()) {
            result.emplace_back(std::make_pair(player.GetId(), player.GetName()));
        }
    }

    return result;
}


//
//  Состояние игры (игроки, их координаты, находки, счет каждого игрока и т.д.)
//
UseCaseState::UseCaseState(Players::Ptr players)
: players_(players) {

}

StateResult UseCaseState::RunUseCase(const Token& token)
{
    StateResult result;

    auto *myself = players_->FindPlayer(token);

    if (!myself) {
        throw Error(http::status::unauthorized, ErrorReason::UNKNOWN_TOKEN);
    }

    const auto &session = myself->GetSession();

    //
    //  Собираю в массив всех собак - игроков
    //
    const auto &dogs = session.GetDogs();
    for (const auto& dog : dogs) {
        result.players.emplace_back(
            std::make_tuple(dog.GetId(), dog.GetPos(), dog.GetSpeed(), dog.GetDir(), dog.GetBag(), dog.GetScore())
        );
    }

    //
    //  Затем все трофеи
    //
    const auto &loots = session.GetLoots();
    for (const auto& loot : loots) {
        result.loots.emplace_back(
            std::make_tuple(loot.GetId(), loot.GetType(), loot.GetPos())
        );
    }

    return result;

}


//
//  Смена направления движения игрока (его собаки)
//
UseCaseAction::UseCaseAction(Players::Ptr players)
: players_(players) {

}

void UseCaseAction::RunUseCase(const Token& token, model::Dog::Direction dir) {

    auto* player = players_->FindPlayer(token);
    if  (!player) {
        throw Error(http::status::unauthorized, ErrorReason::UNKNOWN_TOKEN);
    }

    player->ChangeDir(dir);

}


//
//  Увеличение игрового времени на delta time
//
UseCaseTick::UseCaseTick(model::Game::Ptr game, Players::Ptr players)
: game_(game)
, players_(players) {
}

std::vector<geom::Item> OfficesToItems(const model::Map::Offices& offices) {

    std::vector<geom::Item> items;

    for (const auto& office : offices) {
        items.emplace_back(
            geom::Item{{static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y)}, office.WIDTH, 0}
        );
    }

    // std::transform(offices.cbegin(), offices.cend(),
    //     std::back_inserter(items),
    //     [](const auto& office) { 
    //         return geom::Item{{static_cast<double>(office.GetPosition().x), static_cast<double>(office.GetPosition().y)}, office.WIDTH, 0}; 
    //     }
    // );

    return items;

}

std::vector<geom::Item>& operator+=(std::vector<geom::Item>& left, std::vector<geom::Item>&& right) {
    left.insert(left.end(), right.begin(), right.end());
    return left;
}

void UseCaseTick::RunUseCase(model::TimeInterval timeDelta) {

    const auto &maps = game_->GetMaps();

    for (const auto& map : maps) {
        
        auto* session = game_->FindSession(map.GetId());
        if ( !session ) {
            continue; // вообще-то это треш какой-то
        }

        //
        //  Сначала генерирую потерянные вещи, затем двигаю собак
        //
        auto items = session->GenerateLoots(timeDelta);
        auto gatherers = session->MoveDogs(timeDelta);

        //
        //  Добавить к элементам офисы
        //
        items += OfficesToItems(map.GetOffices());

        //
        //  Создать провайдер вещей и сборщиков
        //
        model::LootGathererProvider gp(std::move(items), std::move(gatherers));

        //
        //  Собрать трофеи, посчитать статистику
        //
        session->GatherLoots(gp);
    }

}


//
//  Список призеров игры
//
UseCaseRecords::UseCaseRecords(postgres::Database& db)
: db_(db) {
}

RecordsResult UseCaseRecords::RunUseCase(int start, int maxItems) {

    return db_.GetRecords(start, maxItems);
}


} // namespace use_case


//
//  "Приложение" - общий интерфейс ко всем сценариям игры
//

//  Пока делаю блокировку мьютексом - позже посмотрю boost::asio::strand
#define LOCK_GAME_STATE()   \
    std::lock_guard _guard(game_state_lock_)

// открытый конструктор для использования без лишних параметров
Application::Application(model::Game::Ptr game, postgres::Database& db, bool randomize_spawn_points)
: Application(game, std::make_shared<app::Players>(), db, randomize_spawn_points) {

}

// закрытый конструктор
Application::Application(model::Game::Ptr game, Players::Ptr players, postgres::Database& db, bool randomize_spawn_points)
: players_(players)
, use_case_maps_list_(game)
, use_case_map_info_(game)
, use_case_join_game_(game, players, randomize_spawn_points)
, use_case_players_(players)
, use_case_state_(players)
, use_case_action_(players)
, use_case_tick_(game, players)
, use_case_records_(db)
, dogs_collector_(game, players, db)
{
}

void Application::AddListener(ApplicationListener::Ptr listener)
{
    listeners_.emplace_back(std::move(listener));
}

void Application::AddPlayer(Token token, Player&& player) {
    players_->AddPlayer(token, std::move(player));
}

const model::Game::Maps& Application::GetMaps()
{
    return use_case_maps_list_.RunUseCase();
}

const model::Map& Application::GetMap(const model::Map::Id &id)
{
    return use_case_map_info_.RunUseCase(id);
}

RecordsResult Application::GetRecords(int start, int maxItems)
{
    LOCK_GAME_STATE();
    return use_case_records_.RunUseCase(start, maxItems);
}

JoinGameResult Application::JoinGame(const std::string &name, const model::Map::Id& mapId)
{
    LOCK_GAME_STATE();
    return use_case_join_game_.RunUseCase(name, mapId);
}

PlayersResult Application::GetPlayers(const Token &token)
{
    LOCK_GAME_STATE();
    return use_case_players_.RunUseCase(token);
}

StateResult Application::GetState(const Token &token)
{
    LOCK_GAME_STATE();
    return use_case_state_.RunUseCase(token);
}

void Application::RotateDog(const Token &token, model::Dog::Direction dir)
{
    LOCK_GAME_STATE();
    use_case_action_.RunUseCase(token, dir);
}

void Application::Tick(model::TimeInterval timeDelta)
{
    LOCK_GAME_STATE();
    
    use_case_tick_.RunUseCase(timeDelta);

    //
    //  Некоторые игроки - собаки могли покинуть игру,
    //  нужно их удалить
    //
    dogs_collector_.CollectRetiredDogs();

    //
    //  После выполнения use case еще делаю опциональные операции,
    //  которые могут быть заданы, а могут быть и не заданы
    //  (например сохранение состояния)
    //
    for (auto& listener : listeners_) {
        listener->OnTick(timeDelta);
    }
}

//
//  Функция нужна только для сериализации
//
Players::Pairs Application::GetTokensPlayers() const 
{
    return players_->GetPairs();
}

} // namespace app