#include "../sdk.h"
#include <boost/asio/bind_executor.hpp>

#include "ticker.h"

namespace model {

using namespace std::literals;

Ticker::Ticker(Strand strand, std::chrono::milliseconds period, Handler handler)
: strand_(strand)
, period_(period)
, handler_(handler) {

}

void Ticker::Start() {
    last_tick_ = std::chrono::steady_clock::now();

    // Выполнить SchedulTick внутри strand_
    net::post(strand_, [self = this->shared_from_this()]
                  { self->ScheduleTick(); });
}

void Ticker::ScheduleTick() {
    // выполнить OnTick через промежуток времени period_
    timer_.expires_after(period_);
    timer_.async_wait(boost::asio::bind_executor(strand_, [self = this->shared_from_this()](sys::error_code ec) {
        self->OnTick(ec);}));
}

void Ticker::OnTick(sys::error_code ec) {

    if (ec) {
        //  TODO: подумать про передачу логера в либу
        return;
    }

    auto current_tick = std::chrono::steady_clock::now();
    handler_(std::chrono::duration_cast<std::chrono::milliseconds>(current_tick - last_tick_));
    last_tick_ = current_tick;
    ScheduleTick();
}


} // namespace model
