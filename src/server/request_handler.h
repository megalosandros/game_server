#pragma once
#include <filesystem>
#include "logger.h"
#include "http_server.h"
#include "api_handler.h"
#include "file_handler.h"

namespace http_handler {

namespace fs = std::filesystem;


//
//  Это какой-то отдельно стоящий класс получился, на самом деле он 
//  только запросы по обработчикам
//
class RequestHandler final : public std::enable_shared_from_this<RequestHandler> {
public:
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

    explicit RequestHandler(const fs::path& root, app::Application::Ptr application, bool enable_tick_requests, Strand api_strand)
        : file_request_handler_{root}
        , api_request_handler_{application, enable_tick_requests} 
        , api_strand_{api_strand} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

        std::chrono::system_clock::time_point start_ts = std::chrono::system_clock::now();

        logger::TraceRequest(req);

        //
        //  Обработать запрос request и отправить ответ, используя send
        //  Проблема - ответы могут быть разного типа
        //  поэтому приходится использовать std::variant + паттерн visitor
        //
        VariantResponse varResp = MakeResponse(std::forward<decltype(req)>(req));

        logger::TraceResponse(start_ts, varResp);

        std::visit(send, varResp);

    }

private:
    VariantResponse MakeResponse(StringRequest &&req);

    FileRequestHandler file_request_handler_;
    ApiRequestHandler api_request_handler_;

    // пока не используется, чуть позже, пока на мьютексе
    Strand api_strand_;
};

}  // namespace http_handler
