#include "../sdk.h"
#include <boost/asio/dispatch.hpp>
#include <iostream>

#include "logger.h"
#include "http_server.h"

namespace http_server {

//
//  Скопировано из урока async_server
//


void ReportError(beast::error_code ec, std::string_view what) {
    logger::Trace(logger::network_error(ec, what), "error"sv);
}

SessionBase::SessionBase(tcp::socket&& socket)
: stream_(std::move(socket)) {
}

void SessionBase::Run() {
    //
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    //
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    //
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    //
    request_ = {};
    stream_.expires_after(30s);

    //
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    //
    http::async_read(stream_, buffer_, request_,
                     // По окончании операции будет вызван метод OnRead
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {

    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        return ReportError(ec, "read"sv);
    }

    InjectRemoteAddr();

    HandleRequest(std::move(request_));
}
    
void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {

    if (ec) {
        return ReportError(ec, "write"sv);
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    //
    // Считываем следующий запрос
    //
    Read();
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        return ReportError(ec, "close"sv);
    }
}

//
//  Добавляю к запросу поле RemoteAddr - для логирования
//
void SessionBase::InjectRemoteAddr() {
    beast::error_code ec;
    const auto& socket = stream_.socket();
    auto endpoint = socket.remote_endpoint(ec);

    if (!ec) {
        request_.insert("RemoteAddr"sv, endpoint.address().to_string());
    }

}


}  // namespace http_server
