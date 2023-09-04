#pragma once
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>

#include "http_response.h"

namespace logging = boost::log;


#ifdef _ENABLE_DEBUG_TRACE
#define DEBUG_TRACE(...) logger::Trace(__VA_ARGS__)
#else
#define DEBUG_TRACE(...)
#endif  // _ENABLE_DEBUG_TRACE

namespace logger {

using namespace std::literals;
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(data, "AdditionalData", boost::json::value)


void InitServerLogger();

//
//  При получении запроса:
//      message — строка request received
//      data — объект с полями:
//      ip — IP-адрес клиента (полученный через endpoint.address().to_string()),
//      URI — запрошенный адрес,
//      method — использованный метод HTTP.
//
struct request_received
{
    // request_received(const boost::asio::ip::address& address, const std::string& URI, boost::beast::http::verb method)
    // : custom_data{{"ip"s, address.to_string()}, {"URI"s, URI}, {"method"s, AsStringView(method)}}
    // {}
    request_received(std::string_view address, std::string_view URI, std::string_view method)
    : custom_data{{"ip"s, address}, {"URI"s, URI}, {"method"s, method}}
    {}

    boost::json::value custom_data;
    bool error = false;
};

//
//  При формировании ответа:
//      message — строка response sent
//      data — объект с полями:
//      response_time — время формирования ответа в миллисекундах (целое число).
//      code — статус-код ответа, например, 200 (http::response<T>::result_int()).
//      content_type — строка или null, если заголовок в ответе отсутствует.
//
struct response_sent
{
    response_sent(int64_t response_time, unsigned int code, std::string_view content_type)
    : custom_data{{"response_time", response_time}, {"code"s, code}, {"content_type"s, content_type}} 
    {}

    response_sent(int64_t response_time, unsigned int code)
    : custom_data{{"response_time", response_time}, {"code"s, code}, {"content_type"s, nullptr}} 
    {}

    boost::json::value custom_data;
    bool error = false;
};

//
//  При запуске сервера:
//      message — строка server started;
//      data — объект с полями:
//      port — порт, на котором работает сервер (обычно 8080),
//      address — адрес интерфейса (обычно 0.0.0.0).
//
struct server_started
{
    server_started(const boost::asio::ip::address& address, boost::asio::ip::port_type port)
    : custom_data{{"port"s, port}, {"address"s, address.to_string()}} {}

    boost::json::value custom_data;
    bool error = false;
};

//
//  При остановке сервера:
//      message — строка server exited;
//      data — объект с полями:
//      code — код возврата (0 при успехе, EXIT_FAILURE при ошибке),
//      exception — если выход по исключению, то описание исключения (std::exception::what()).
//
struct server_exited
{
    server_exited(int code)
    : custom_data{{"code"s, code}} {}
    
    server_exited(int code, const std::exception& err)
    : custom_data{{"code"s, code}, {"exception"s, err.what()}}
    , error(true) {}


    boost::json::value custom_data;
    bool error = false;
};

//
//  При возникновении сетевой ошибки:
//      message — строка error
//      data — объект с полями:
//      code — код ошибки (beast::error_code::value()).
//      text — сообщение ошибки (beast::error_code::message()).
//      where — место возникновения (read, write, accept).
//
struct network_error
{
    network_error(const boost::beast::error_code& ec, const std::string& where)
    : custom_data{{"code"s, ec.value()}, {"text"s, ec.message()}, {"where", where}} {}
    network_error(const boost::beast::error_code& ec, std::string_view where)
    : custom_data{{"code"s, ec.value()}, {"text"s, ec.message()}, {"where", where}} {}

    boost::json::value custom_data;
    bool error = true;
};

struct game_action
{
    game_action(const char dir)
    : custom_data{{"move"s, std::string(1, dir)}} {}

    boost::json::value custom_data;
    bool error = false;
};

struct game_tick
{
    game_tick(const std::int64_t tick)
    : custom_data{{"timeDelta"s, tick}} {}

    boost::json::value custom_data;
    bool error = false;
};

template <typename T>
void Trace(const T& attributes, std::string_view message)
{
    if (attributes.error) {
        BOOST_LOG_TRIVIAL(error) << logging::add_value(data, attributes.custom_data)  << message;
    }
    else {
       BOOST_LOG_TRIVIAL(info) << logging::add_value(data, attributes.custom_data)  << message;
    }
}

void TraceRequest(const http_handler::StringRequest &req);
void TraceResponse(std::chrono::system_clock::time_point start_ts, const http_handler::VariantResponse &resp);


} // namespace logger
