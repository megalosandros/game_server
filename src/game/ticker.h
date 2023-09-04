#pragma once
#include <memory>
#include <chrono>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>


namespace model {

namespace net = boost::asio;
namespace sys = boost::system;


class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(std::chrono::milliseconds delta)>;
    using Ptr = std::shared_ptr<Ticker>;

    Ticker(Strand strand, std::chrono::milliseconds period, Handler handler);

    void Start();

private:
    void ScheduleTick();
    void OnTick(sys::error_code ec);

private:
    Strand strand_;
    net::steady_timer timer_{strand_};
    std::chrono::milliseconds period_;
    Handler handler_;
    std::chrono::steady_clock::time_point last_tick_;
}; 


} // namespace model
