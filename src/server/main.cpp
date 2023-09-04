#include "../sdk.h"
//
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <iostream>
#include <thread>
#include <random>

#include "../game/app.h"
#include "../game/ticker.h"
#include "../game/model_serialization.h"
#include "json_loader.h"
#include "request_handler.h"
#include "command_line.h"
#include "logger.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

//
// Запускает функцию fn на thread_count потоках, включая текущий
//
template <typename Fn>
void RunWorkers(unsigned thread_count, const Fn& fn) {
    thread_count = std::max(1u, thread_count);
    std::vector<std::jthread> workers;
    workers.reserve(thread_count - 1);
    
    //
    // Запускаем thread_count-1 рабочих потоков, выполняющих функцию fn
    //
    while (--thread_count) {
        workers.emplace_back(fn);
    }

    //
    //  и одну функцию в текущем
    //
    fn();
}


constexpr const char DB_URL_ENV_NAME[]{"GAME_DB_URL"};

std::string GetDatabaseUrlFromEnv() {
    std::string db_url;
    if (const auto* url = std::getenv(DB_URL_ENV_NAME)) {
        db_url = url;
    } else {
        throw std::runtime_error(DB_URL_ENV_NAME + " environment variable not found"s);
    }
    return db_url;
}

}  // namespace


int main(int argc, const char *argv[])
{
    // Логгер - он как бы над всеми живет, его сразу проинициализировать лучше
    logger::InitServerLogger();

    try
    {
        // Распарсить командную строку с обязательными параметрами config-file и www-root
        auto args = util::ParseCommandLine(argc, argv);
        if (!args) {
            return EXIT_SUCCESS;
        }

        // Получить корневой путь для файлов www сервера
        sys::error_code ec;
        std::filesystem::path root = std::filesystem::canonical(args->www_root, ec);
        if (ec) {
            throw std::invalid_argument("Could not find root www directory: "s + ec.message());
        }

        // Создать подключение к Postgres - достаточно одного подключения, поскольку
        // все операции с БД сериализованы
        postgres::Database db(GetDatabaseUrlFromEnv());

        // Загрузить карту из файла и построить модель игры
        auto game = json_loader::LoadGame(args->congig_file);


        // Создать объект приложения, который отвечает за игроков и сценарии использования
        auto application = std::make_shared<app::Application>(game, db, args->randomize_spawn_points);


        // Если задан файл с состоянием - восстановить состояние игры
        if (!args->state_file.empty()) {
            serialization::LoadGameState(args->state_file, game, application);

            //
            //  если вдобавок к файлу задан период автосохранения - установить
            //  в приложение "автосохранятель"
            //
            if (args->save_state_period) {
                application->AddListener(
                    std::make_unique<serialization::SerializationListener>(
                        args->state_file,
                        std::chrono::milliseconds(args->save_state_period),
                        game,
                        application
                    )
                );
            }
        }

        // Проинициализировать io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        // Добавить асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code &ec, [[maybe_unused]] int signal_number)
                           {
            if (!ec) {
                ioc.stop();
            } });

        // strand для выполнения запросов к API (пока что синхронизация сделана через мьютекс)
        auto apiStrand = net::make_strand(ioc);

        // нужно ли запускать таймер внутри игры?
        if (args->tick_period) {
            //
            //  просят включить таймер внутри игры - включаю
            //  таймер у меня - это сущность игры, поскольку время явление физическое, как дороги и здания
            //  а вот действие по таймеру будет выполняться на уровне app, поскольку отработка "тика" -
            //  это сценарий использования
            //
            auto ticker = std::make_shared<model::Ticker>(
                apiStrand,
                std::chrono::milliseconds(args->tick_period),
                [application](std::chrono::milliseconds delta) {
                    application->Tick(delta);
                });
            game->SetTicker(ticker);
            ticker->Start();
        }

        // Создать обработчик HTTP-запросов и связать его с приложением
        auto handler = std::make_shared<http_handler::RequestHandler>(root, application, !args->tick_period, apiStrand);

        // Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, [handler](auto &&req, auto &&send)
                               { (*handler)(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send)); });

        // Эта надпись сообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        logger::Trace(logger::server_started(address, port), "server started"sv);

        // Поехали! (запустить обработку асинхронных операций)
        RunWorkers(std::max(1u, num_threads), [&ioc]
                   { ioc.run(); });

        // Если задан файл с состоянием - сохранить состояние игры
        if (!args->state_file.empty()) {
            serialization::SaveGameState(args->state_file, game, application);
        }

        logger::Trace(logger::server_exited(0), "server exited"sv);
        return EXIT_SUCCESS;
    }
    catch (const std::exception &ex)
    {
        logger::Trace(logger::server_exited(EXIT_FAILURE, ex), "server exited"sv);
        return EXIT_FAILURE;
    }
}
