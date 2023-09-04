#pragma once

#include "../game/model.h"
#include "../game/app.h"
#include "ci_string.h"
#include "http_response.h"

namespace http_handler {

using namespace std::literals;

//
//  Известные в приложении эндпоинты
//
struct Endpoint {
    Endpoint() = delete;
    using Value = std::string_view;

    static constexpr Value REST_API = "/api/"sv;
    static constexpr Value MAPS_REQUEST = "/api/v1/maps"sv;
    static constexpr Value JOIN_REQUEST = "/api/v1/game/join"sv;
    static constexpr Value PLAYERS_REQUEST = "/api/v1/game/players"sv;
    static constexpr Value STATE_REQUEST = "/api/v1/game/state"sv;
    static constexpr Value ACTION_REQUEST = "/api/v1/game/player/action"sv;
    static constexpr Value TICK_REQUEST = "/api/v1/game/tick"sv;
    static constexpr Value RECORDS_REQUEST = "/api/v1/game/records"sv;

};

//
//  Базовый класс для обработки REST API запросов
//  в зависимости от запроса создается экземпляр того или иного потомка
//  и уже он обрабатывает конкретный запрос
//
class ApiHandlerBase {
public:
    using Ptr = std::shared_ptr<ApiHandlerBase>;

    //
    //  чтобы нормально все удалилось в ApiHandlerBase::Ptr
    //
    virtual ~ApiHandlerBase() = default;

    //
    //  потомки реализуют эту функцию для обработки запросов
    //  ответ помещается в std::variant, который может хранить 
    //  либо строку, либо файл, либо..
    //
    virtual StringResponse HandleRequest(StringRequest &&req, const std::optional<app::Token>& token) = 0;

    //
    //  проверка поддерживаемых методов
    //
    bool CheckRequestMethod(boost::beast::http::verb method) const noexcept;

    //
    //  проверка содержимого запроса
    //
    bool CheckContentType(std::string_view content_type) const noexcept;

    //
    //  какие методы поддерживаются?
    //
    std::string_view AllowedMethods() const noexcept;

    //
    //  нужна ли аутентификация?
    //
    bool IsAuthorizationRequired() const noexcept;

protected:
    //
    //  чтобы не было соблазнов создавать экземляры этого класса
    //
    ApiHandlerBase(
        app::Application::Ptr application,
        std::initializer_list<boost::beast::http::verb> methods,
        std::string_view allowed_methods_names,
        std::string_view content_type,
        bool is_authorization_required);

    //
    //  в целом я такое не люблю делать (protected данные) - но этот app нужен всем потомкам
    //  ну пусть тут пока полежит
    //
    app::Application::Ptr app_;

private:
    std::vector<boost::beast::http::verb> allowed_methods_;
    std::string_view allowed_methods_names_;
    std::string_view content_type_;
    bool is_authorization_required_;
};


//
//  Поддерживаемые типы запросов:
//  GET, HEAD   /api/v1/maps
//  GET, HEAD   /api/v1/maps/{id-карты}
//  POST        /api/v1/game/join
//  GET, HEAD   /api/v1/game/players
//  GET, HEAD   /api/v1/game/state
//  POST        /api/v1/game/player/action
//  POST        /api/v1/game/tick
//  GET         /api/v1/game/records
//
//  Если URI-строка запроса начинается с /api/, но не подпадает
//  ни под один из текущих форматов, сервер должен вернуть ответ с 400 статус-кодом.
//
class ApiRequestHandler {
    // не нужны эти
    ApiRequestHandler(const ApiRequestHandler&) = delete;
    ApiRequestHandler& operator=(const ApiRequestHandler&) = delete;
    ApiRequestHandler(ApiRequestHandler&&) = delete;
    ApiRequestHandler& operator=(ApiRequestHandler&&) = delete;

public:
    ApiRequestHandler(app::Application::Ptr application, bool enable_tick_requests);

    StringResponse HandleRequest(StringRequest &&req);

    //  если URI-строка запроса начинается с /api/, ...
    static bool IsApiRequest(const StringRequest &req);

private:
    void CreateEndpointConnections(bool enable_tick_requests);
    ApiHandlerBase::Ptr SelectApiHandler(std::string_view target);

    using EndpointMapper = std::unordered_map<std::string, ApiHandlerBase::Ptr>;
    app::Application::Ptr app_;
    EndpointMapper endpoints_;

    constexpr static std::string_view BAD_REQUEST = "{\n\"code\": \"badRequest\",\n\"message\": \"Bad request\"\n}"sv;
    constexpr static std::string_view INVALID_METHOD = "{\n\"code\": \"invalidMethod\",\n\"message\": \"Requested method not allowed\"\n}"sv;
    constexpr static std::string_view INVALID_CONTENT_TYPE = "{\n\"code\": \"invalidArgument\",\n\"message\": \"Invalid content type\"\n}"sv;
    constexpr static std::string_view INVALID_TOKEN = "{\n\"code\": \"invalidToken\",\n\"message\": \"Authorization header is missing\"\n}"sv;
};

//
//  получить список карт (ид + имя)
//  получить одну карту (по ид.)
//
class MapsHandler : public ApiHandlerBase {
public:
    explicit MapsHandler(app::Application::Ptr application);

    StringResponse HandleRequest(StringRequest &&req, const std::optional<app::Token>&) override;

private:
    StringResponse HandleMapsList(StringRequest&& req);
    StringResponse HandleMapInfo(StringRequest&& req, const std::string& id);
};

//
//  Вход в игру
//
class GameJoin : public ApiHandlerBase {
public:
    explicit GameJoin(app::Application::Ptr application);

    StringResponse HandleRequest(StringRequest &&req, const std::optional<app::Token>&) override;

private:
    constexpr static std::string_view BAD_REQUEST = "{\n\"code\": \"invalidArgument\",\n\"message\": \"Join game request parse error\"\n}"sv;
};

//
//  Получение списка игроков
//
class GamePlayers : public ApiHandlerBase {
public:
    explicit GamePlayers(app::Application::Ptr application);

    StringResponse HandleRequest(StringRequest &&req, const std::optional<app::Token>& token) override;

};

//
//  Запрос игрового состояния
//
class GameState : public ApiHandlerBase {
public:
    explicit GameState(app::Application::Ptr application);

    StringResponse HandleRequest(StringRequest &&req, const std::optional<app::Token>& token) override;

};

//
//  Управление действиями своего персонажа
//
class GameAction : public ApiHandlerBase {
public:
    explicit GameAction(app::Application::Ptr application);

    StringResponse HandleRequest(StringRequest &&req, const std::optional<app::Token>& token) override;

private:
    constexpr static std::string_view BAD_REQUEST = "{\n\"code\": \"invalidArgument\",\n\"message\": \"Failed to parse action JSON\"\n}"sv;
    constexpr static std::string_view RESPONSE_BODY = "{}"sv;
};

//
//  Запрос для управления временем на карте
//
class GameTick : public ApiHandlerBase {
public:
    explicit GameTick(app::Application::Ptr application);

    StringResponse HandleRequest(StringRequest &&req, const std::optional<app::Token>& token) override;

private:
    constexpr static std::string_view BAD_REQUEST = "{\n\"code\": \"invalidArgument\",\n\"message\": \"Failed to parse tick request JSON\"\n}"sv;
    constexpr static std::string_view RESPONSE_BODY = "{}"sv;
};

//
//  получить список рекордсменов
//
class RecordsHandler : public ApiHandlerBase {
public:
    explicit RecordsHandler(app::Application::Ptr application);

    StringResponse HandleRequest(StringRequest &&req, const std::optional<app::Token>&) override;

private:
    struct Parameters {
        int start{0};
        int maxItems{100};
    };

    Parameters ParseRequest(std::string_view target);

    constexpr static int MAX_ITEMS = 100;
    constexpr static std::string_view BAD_REQUEST = "{\n\"code\": \"invalidArgument\",\n\"message\": \"Parameter maxItems is invalid\"\n}"sv;
    constexpr static std::string_view P_START = "start"sv;
    constexpr static std::string_view P_MAX_ITEMS = "maxItems"sv;
};

}  // namespace http_handler
