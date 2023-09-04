#include "../sdk.h"
#include "http_response.h"
#include "json_tags.h"
#include "logger.h"

namespace logger {

using namespace std::literals;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;
namespace json = boost::json;
namespace beast = boost::beast;
namespace http = beast::http;


void LogFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {

    json::object obj;

    //
    //  Некоторых значений может не быть - касается всех атрибутов
    //

    auto rec_time_stamp = rec[timestamp];
    if (rec_time_stamp) {
        obj.emplace("timestamp", to_iso_extended_string(*rec_time_stamp));
    }

    auto rec_data = rec[data];
    if (rec_data) {
        obj.emplace("data", rec_data.get());
    }

    auto rec_message = rec[expr::smessage];
    if (rec_message) {
        obj.emplace("message", rec_message.get());
    }

    strm << obj;

}

void InitServerLogger() {
    logging::add_common_attributes();

    logging::add_console_log( 
        std::cout,
        keywords::format = &LogFormatter,
        keywords::auto_flush = true
    ); 
    
}

void TraceRequest(const http_handler::StringRequest &req)
{
    logger::Trace(request_received(req["RemoteAddr"sv], req.target(), req.method_string()), "request received"sv);
}


template <typename Response>
void TraceResponseImpl(int64_t respTime, const Response& resp)
{
    std::string_view contentType = resp[boost::beast::http::field::content_type];
    if (contentType.empty()) {
        logger::Trace(response_sent(respTime, resp.result_int()), "response sent"sv);
    }
    else {
        logger::Trace(response_sent(respTime, resp.result_int(), resp[boost::beast::http::field::content_type]), "response sent"sv);
    }
}

void TraceResponse(std::chrono::system_clock::time_point start_ts, const http_handler::VariantResponse &resp)
{
    std::chrono::system_clock::time_point end_ts = std::chrono::system_clock::now();
    auto diff = (end_ts - start_ts) / 1ms;

    std::visit([diff](const auto& arg){ TraceResponseImpl(diff, arg); }, resp);

}

} // namespace logger