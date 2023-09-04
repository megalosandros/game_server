#pragma once
#include <string_view>
#include <variant>

//
// boost.beast будет использовать std::string_view вместо boost::string_view
//
//#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace http_handler {

namespace beast = boost::beast;
namespace http  = beast::http;
using namespace   std::literals;

/**
 * Различные вспомогательные типы и константы - которые используются
 * в разных классах
*/

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Ответ, тело которого представлено в виде файла
using FileResponse = http::response<http::file_body>;
// Сюда можно положить и строку и файл
using VariantResponse = std::variant<StringResponse, FileResponse>;

struct CacheControl {
    CacheControl() = delete;
    using Value = std::string_view;

    constexpr static Value NO_CACHE = "no-cache"sv;
};

struct ContentType {
    ContentType() = delete;
    using Value = std::string_view;

    constexpr static Value TEXT_HTML = "text/html"sv;
    constexpr static Value TEXT_CSS = "text/css"sv;
    constexpr static Value TEXT_PLAIN = "text/plain"sv;
    constexpr static Value TEXT_JS = "text/javascript"sv;
    constexpr static Value APP_JSON = "application/json"sv;
    constexpr static Value APP_XML = "application/xml"sv;
    constexpr static Value APP_OCT = "application/octet-stream"sv;
    constexpr static Value IMAGE_PNG = "image/png"sv;
    constexpr static Value IMAGE_JPEG = "image/jpeg"sv;
    constexpr static Value IMAGE_GIF = "image/gif"sv;
    constexpr static Value IMAGE_BMP = "image/bmp"sv;
    constexpr static Value IMAGE_ICO = "image/vnd.microsoft.icon"sv;
    constexpr static Value IMAGE_TIFF = "image/tiff"sv;
    constexpr static Value IMAGE_SVG = "image/svg+xml"sv;
    constexpr static Value AUDIO_MPEG = "audio/mpeg"sv;
    constexpr static Value UNRELEVANT = ""sv;
};

// Структура Allow задаёт область видимости для констант,
// задающий значения HTTP-заголовка Allow
struct Allow {
    Allow() = delete;
    using Value = std::string_view;

    constexpr static Value GET_HEAD = "GET, HEAD"sv;
    constexpr static Value POST = "POST"sv;
};


// Ответ в виде строки text/plain (Cache-Control не указан)
StringResponse PlainStringResponse(
    StringRequest &&req,
    http::status status,
    std::string_view body);

// Ответ в виде строки application/json (Cache-Control = no-cache)
StringResponse JsonStringResponse(
    StringRequest &&req,
    http::status status,
    std::string_view body);

//  ответ на любой запрос с "method not allowed" методом
StringResponse InvalidMethodResponse(
    StringRequest &&req,
    std::string_view body,
    Allow::Value allowed);


} // namespace http_handler