#include "../sdk.h"
#include "json_serializer.h"
#include "json_loader.h"
#include "ci_string.h"
#include "request_handler.h"
#include "api_handler.h"

namespace http_handler {

namespace sys = boost::system;

ApiHandlerBase::ApiHandlerBase(
    app::Application::Ptr application,
    std::initializer_list<boost::beast::http::verb> methods,
    std::string_view allowed_methods_names,
    std::string_view content_type,
    bool is_authorization_required) 
: app_(application)
, allowed_methods_(methods)
, allowed_methods_names_(allowed_methods_names)
, content_type_(content_type)
, is_authorization_required_(is_authorization_required) {

}

bool ApiHandlerBase::CheckRequestMethod(boost::beast::http::verb method) const noexcept {

    if (allowed_methods_.empty()) {
        return true;
    }

    return (std::find(allowed_methods_.begin(), allowed_methods_.end(), method) != allowed_methods_.end());
}

bool ApiHandlerBase::CheckContentType(std::string_view content_type) const noexcept {
    
    if (content_type_ == ContentType::UNRELEVANT) {
        return true;
    }

    return (content_type_ == content_type);
}

std::string_view ApiHandlerBase::AllowedMethods() const noexcept {
    return allowed_methods_names_;
}

bool ApiHandlerBase::IsAuthorizationRequired() const noexcept {
    return is_authorization_required_;
}


ApiRequestHandler::ApiRequestHandler(app::Application::Ptr application, bool enable_tick_requests)
: app_(application) {

    CreateEndpointConnections(enable_tick_requests);
}


void ApiRequestHandler::CreateEndpointConnections(bool enable_tick_requests) {

    //
    //  запросы со статическим URI
    //
    endpoints_.emplace(Endpoint::MAPS_REQUEST, std::make_shared<MapsHandler>(app_));
    endpoints_.emplace(Endpoint::JOIN_REQUEST, std::make_shared<GameJoin>(app_));
    endpoints_.emplace(Endpoint::PLAYERS_REQUEST, std::make_shared<GamePlayers>(app_));
    endpoints_.emplace(Endpoint::STATE_REQUEST, std::make_shared<GameState>(app_));
    endpoints_.emplace(Endpoint::ACTION_REQUEST, std::make_shared<GameAction>(app_));
    endpoints_.emplace(Endpoint::RECORDS_REQUEST, std::make_shared<RecordsHandler>(app_));

    //
    //  можно было бы еще добавить запросы конкретной карты по ID,
    //  но тогда при передаче неправильного ID будет возвращаться ivnalid request,
    //  а нам надо, чтобы возвращалось map not found - поэтому запрос конкретной карты
    //  я тоже передаю в обрабочик карт и он уже разбирается одну надо карту или все
    //

    //
    //  ручное управление таймером - только для тестов
    //
    if (enable_tick_requests) {
        endpoints_.emplace(Endpoint::TICK_REQUEST, std::make_shared<GameTick>(app_));
    }
}


/* static */
bool ApiRequestHandler::IsApiRequest(const StringRequest &req) {
    //
    //  Сервер может использовать такую логику:
    //  если URI-строка запроса начинается с /api/,
    //  значит следует обработать запрос к API,
    //  в противном случае вернуть содержимое файла внутри корневого каталога
    //
    return req.target().starts_with(Endpoint::REST_API);
}

StringResponse ApiRequestHandler::HandleRequest(StringRequest &&req) {
    //
    //  в зависимости от метода и пути запроса создаю нужный обработчик
    //
    ApiHandlerBase::Ptr apiOperation = SelectApiHandler(req.target());

    if (!apiOperation) {
        //
        //  Обработчика не нашлось - BAD REQUEST
        //
        return JsonStringResponse(
            std::move(req),
            http::status::bad_request,
            BAD_REQUEST);
    }

    if (!apiOperation->CheckRequestMethod(req.method())) {
        //
        //  Обработчик нашелся - метод не подошел
        //
        return InvalidMethodResponse(std::move(req), INVALID_METHOD, apiOperation->AllowedMethods());
    }

    if (!apiOperation->CheckContentType(req[http::field::content_type])) {
        //
        //  Теперь и метод подошел - а содержимое запроса не того типа
        //
        return JsonStringResponse(
            std::move(req),
            http::status::bad_request,
            INVALID_CONTENT_TYPE);
    }

    std::optional<app::Token> token = std::nullopt;

    if (apiOperation->IsAuthorizationRequired()) {
        //
        //  Для запроса требуется авторизация - пробую достать и проверить токен
        //
        token = app::ParseBearerToken(req[http::field::authorization]);

        if (!token) {
            return JsonStringResponse(
                std::move(req),
                http::status::unauthorized,
                INVALID_TOKEN);
        }

    }

    //
    //  если я попал сюда, значит в целом все неплохо - можно вызвать обработчик
    //  но обработчик тоже кидается исключениями если что-то пошло не так
    //
    try {
        return apiOperation->HandleRequest(std::move(req), token);
    }
    catch (const app::Error& e) {
        return JsonStringResponse(
            std::move(req),
            e.status(),
            e.what());
    }
    catch (const std::exception& ex) {
        return PlainStringResponse(
            std::move(req),
            http::status::internal_server_error,
            ex.what());
    }

}

ApiHandlerBase::Ptr ApiRequestHandler::SelectApiHandler(std::string_view target)
{
    //
    //  target также может содержать параметры - это надо учесть
    //
    if (auto stop = target.find('?'); stop != std::string_view::npos) {
        target = target.substr(0, stop);
    }

    if (auto it = endpoints_.find(util::to_string(target)); it != endpoints_.end()) {
        return it->second;
    }

    //
    //  К сожалению случай запроса информации по одной карте пока не попадает под общую
    //  стратегию из за того, что при ошибке в идентификаторе нужно возвращать
    //  404 а не 400
    //
    if (target.starts_with(Endpoint::MAPS_REQUEST)) {
        return endpoints_.at(util::to_string(Endpoint::MAPS_REQUEST));
    }

    //
    //  Если URI-строка запроса начинается с /api/, но не подпадает
    //  ни под один из текущих форматов, сервер должен вернуть ответ с 400 статус-кодом.
    //
    return nullptr;

}


//
//  список всех карт либо информация по одной карте
//
MapsHandler::MapsHandler(app::Application::Ptr application)
: ApiHandlerBase(application, {http::verb::get, http::verb::head}, Allow::GET_HEAD, ContentType::UNRELEVANT, false)
{    
}

StringResponse MapsHandler::HandleRequest(StringRequest &&req, const std::optional<app::Token>&)
{
    auto target = req.target();

    if (target == Endpoint::MAPS_REQUEST) {
        return HandleMapsList(std::move(req));
    }

    target.remove_prefix(Endpoint::MAPS_REQUEST.size() + 1);

    return HandleMapInfo(std::move(req), util::to_string(target));

}

StringResponse MapsHandler::HandleMapsList(StringRequest&& req) {

    const auto &map_list = app_->GetMaps();
    
    auto responseBody = json_serializer::SerializeMapList(map_list);

    return JsonStringResponse(std::move(req), http::status::ok, responseBody);

}

StringResponse MapsHandler::HandleMapInfo(StringRequest&& req, const std::string& id) {
    //
    //  Либо вернут карту - либо исключение
    //
    const auto& map = app_->GetMap(model::Map::Id{id});

    auto responseBody = json_serializer::SerializeMap(map);

    return JsonStringResponse(std::move(req), http::status::ok, responseBody);

}


//
//  Вход в игру
//
GameJoin::GameJoin(app::Application::Ptr application)
: ApiHandlerBase(application, {http::verb::post}, Allow::POST, ContentType::APP_JSON, false)
{
}

StringResponse GameJoin::HandleRequest(StringRequest &&req, const std::optional<app::Token>&) {
    //
    //  Распарсить тело запроса - получить имя и ид. карты
    //
    auto name_and_mapid = json_loader::ParseJoinRequest(req.body());
    if (!name_and_mapid) {
        return JsonStringResponse(
            std::move(req),
            http::status::bad_request,
            BAD_REQUEST);
    }

    //
    //  Отдать в app
    //
    auto token_and_dogid = app_->JoinGame(name_and_mapid->name, name_and_mapid->id);

    //
    //  Сериализовать результат
    //
    auto responseBody = json_serializer::SerializeJoinResult(token_and_dogid);

    //
    //  Вернуть ответ
    //
    return JsonStringResponse(std::move(req), http::status::ok, responseBody);

}

//
//  Получение списка игроков
//
GamePlayers::GamePlayers(app::Application::Ptr application)
: ApiHandlerBase(application, {http::verb::get, http::verb::head}, Allow::GET_HEAD, ContentType::UNRELEVANT, true)
{
}

StringResponse GamePlayers::HandleRequest(StringRequest &&req, const std::optional<app::Token>& token) {
    //
    //  Получить список пользователей (внутри делается проверка что токен кому-то принадлежит)
    //
    auto players = app_->GetPlayers(*token);

    //
    //  Сериализовать результат
    //
    auto responseBody = json_serializer::SerializePlayersResult(players);

    //
    //  Вернуть ответ
    //
    return JsonStringResponse(std::move(req), http::status::ok, responseBody);

}


//
//  Запрос игрового состояния
//
GameState::GameState(app::Application::Ptr application)
: ApiHandlerBase(application, {http::verb::get, http::verb::head}, Allow::GET_HEAD, ContentType::UNRELEVANT, true)
{
}

StringResponse GameState::HandleRequest(StringRequest &&req, const std::optional<app::Token>& token) {

    //
    //  Получить состояние (список собак с координатами и т.п.)
    //
    auto states = app_->GetState(*token);

    //
    //  Сериализовать результат
    //
    auto responseBody = json_serializer::SerializeStateResult(states);

    //
    //  Вернуть ответ
    //
    return JsonStringResponse(std::move(req), http::status::ok, responseBody);

}

//
//  Управление действиями своего персонажа
//
GameAction::GameAction(app::Application::Ptr application)
: ApiHandlerBase(application, {http::verb::post}, Allow::POST, ContentType::APP_JSON, true)
{
}

StringResponse GameAction::HandleRequest(StringRequest &&req, const std::optional<app::Token>& token) {
    //
    //  Распарсить тело запроса - получить направление движения
    //
    auto move_direction = json_loader::ParseActionRequest(req.body());
    if (!move_direction) {
        return JsonStringResponse(
            std::move(req),
            http::status::bad_request,
            BAD_REQUEST);
    }

    DEBUG_TRACE(logger::game_action(static_cast<char>(*move_direction)), "game action"sv);

    //
    //  Отдать в app
    //
    app_->RotateDog(*token, *move_direction);

    //
    //  Вернуть ответ (пустые скобочки)
    //
    return JsonStringResponse(std::move(req), http::status::ok, RESPONSE_BODY);
    
}


//
//  Запрос для управления временем на карте
//
GameTick::GameTick(app::Application::Ptr application)
: ApiHandlerBase(application, {http::verb::post}, Allow::POST, ContentType::APP_JSON, false)
{
}

StringResponse GameTick::HandleRequest(StringRequest &&req, const std::optional<app::Token>&)
{
    //
    //  Распарсить тело запроса - получить дельту времени
    //
    auto time_delta = json_loader::ParseTickRequest(req.body());
    if (!time_delta) {
        return JsonStringResponse(
            std::move(req),
            http::status::bad_request,
            BAD_REQUEST);
    }

    DEBUG_TRACE(logger::game_tick(time_delta->count()), "time delta"sv);

    //
    //  Отдать в app
    //
    app_->Tick(*time_delta);

    //
    //  Вернуть ответ (пустые скобочки)
    //
    return JsonStringResponse(std::move(req), http::status::ok, RESPONSE_BODY);
    
}


//
//  получить список рекордсменов
//
RecordsHandler::RecordsHandler(app::Application::Ptr application) 
: ApiHandlerBase(application, {http::verb::get, http::verb::head}, Allow::GET_HEAD, ContentType::UNRELEVANT, false)
{
}

RecordsHandler::Parameters RecordsHandler::ParseRequest(std::string_view target) {

    //
    //  еще не известно есть ли у меня параметры в запросе
    //  если параметров нет - буду использовать дефолтные
    //
    auto stop = target.find('?');
    if (stop == std::string_view::npos) {
        return {};
    }

    //
    //  интересуют только параметры
    //
    target = target.substr(stop + 1);

    std::vector<std::string> tags;
    boost::algorithm::split(tags, target, boost::algorithm::is_any_of("?=&"), boost::algorithm::token_compress_off);

    //
    //  если параметры есть - их обязательно должно быть четное число, потому что пары:
    //  параметр = значение
    //
    if (tags.empty() || (tags.size() % 2)) {
        return {};
    }

    Parameters params;

    for (size_t i = 0; i < tags.size(); i+=2) {
        //
        //  я смело могу брать (индекс + 1) потому что 
        //  число элементов в векторе у меня всегда четное,
        //  а (индекс + 1) - всегда нечетный (1, 3, ...)
        //
        if (tags[i] == P_START) {
            try {
                params.start = std::stoi(tags[i + 1]);
            }
            catch (const std::exception&) {
                params.start = 0;
            }
        }
        if (tags[i] == P_MAX_ITEMS) {
            try {
                params.maxItems = std::stoi(tags[i + 1]);
            }
            catch (const std::exception&) {
                params.maxItems = MAX_ITEMS;
            }
        }
    }

    return params;
}

StringResponse RecordsHandler::HandleRequest(StringRequest &&req, const std::optional<app::Token>&) {
    
    //
    //  Распарсить параметры запроса - получить start и maxItems
    //
    auto params = ParseRequest(req.target());
    if  (params.maxItems > MAX_ITEMS) {
        return JsonStringResponse(
            std::move(req),
            http::status::bad_request,
            BAD_REQUEST);
    }

    //
    //  Получить список рекордсменов
    //
    auto records = app_->GetRecords(params.start, params.maxItems);

    //
    //  Сериализовать результат
    //
    auto responseBody = json_serializer::SerializeRecordsResult(records);

    //
    //  Вернуть ответ
    //
    return JsonStringResponse(std::move(req), http::status::ok, responseBody);

}




}  // namespace http_handler
