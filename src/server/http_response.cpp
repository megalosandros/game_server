#include "../sdk.h"
#include "http_response.h"

namespace http_handler {

//
// Создаёт StringResponse с заданными параметрами
//
StringResponse MakeStringResponse(
    StringRequest &&req,
    http::status status,
    std::string_view body,
    ContentType::Value content_type,
    Allow::Value allowed = {},
    CacheControl::Value cache_control = {}) {

    StringResponse response(status, req.version());

    response.set(http::field::content_type, content_type);
    if (!allowed.empty()) {
        response.set(http::field::allow, allowed);
    }
    if (!cache_control.empty()) {
        response.set(http::field::cache_control, cache_control);
    }
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(req.keep_alive());

    return response;

}

//
// Ответ в виде строки text/plain (Cache-Control не указан)
//
StringResponse PlainStringResponse(
    StringRequest &&req,
    http::status status,
    std::string_view body) {

    return MakeStringResponse(std::move(req), status, body, ContentType::TEXT_PLAIN);

}

//
// Ответ в виде строки application/json (Cache-Control = no-cache)
//
StringResponse JsonStringResponse(
    StringRequest &&req,
    http::status status,
    std::string_view body) {

    return MakeStringResponse(std::move(req), status, body, ContentType::APP_JSON, {}, CacheControl::NO_CACHE);

}

//
//  обработчик любого запроса с "плохим" методом
//
StringResponse InvalidMethodResponse(
    StringRequest &&req,
    std::string_view body,
    Allow::Value allowed) {

    const ContentType::Value content_type = 
        (!body.empty() && body[0] == '{') ? ContentType::APP_JSON : ContentType::TEXT_PLAIN;

    return MakeStringResponse(
        std::move(req), 
        http::status::method_not_allowed, 
        body,
        content_type,
        allowed,
        CacheControl::NO_CACHE);

}

} // namesace http_handler

