#include "../sdk.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <iostream>
#include <fstream>
#include <cstdio>
#include "model_serialization.h"

namespace serialization {


SerializationListener::SerializationListener(
    const std::string& state_file,
    TimeInterval save_period,
    model::Game::Ptr game,
    app::Application::Ptr application) 
: state_file_(state_file)
, save_period_(save_period)
, game_(game)
, application_(application) {

}

void SerializationListener::OnTick(model::TimeInterval timeDelta) noexcept {
    try {
        time_since_save_ += timeDelta;

        if (time_since_save_ < save_period_)
        {
            //
            //  еще рано что либо сохранять
            //
            return;
        }

        SaveGameState(state_file_, game_, application_);

        time_since_save_ = {};
    }
    catch (const std::exception &) {
    }
}

void SaveGameState(const std::filesystem::path &state_file, model::Game::Ptr game, app::Application::Ptr app) noexcept
{

    try
    {
        std::string tmp_file_name(state_file.c_str() + std::string("~"));
        std::ofstream output_stream{tmp_file_name};
        boost::archive::text_oarchive output_archive{output_stream};

        //
        //  Состояние, которое сохраняется на диск, должно включать:
        //

        //
        //  - информацию обо всех динамических объектах на
        //  всех картах — собаках и потерянных предметах;
        //

        const auto &maps = game->GetMaps();

        std::vector<SessionRepr> sessions;

        for (const auto &map : maps)
        {

            const auto *session = game->FindSession(map.GetId());
            if (!session)
                continue;

            sessions.emplace_back(*session);
        }

        output_archive << sessions;

        //
        //  - информацию о токенах и идентификаторах пользователей,
        //  вошедших в игру.
        //

        auto tokens = app->GetTokensPlayers();

        std::vector<PlayerRepr> players_and_tokens;

        for (const auto &token : tokens) {
            players_and_tokens.emplace_back(token);
        }

        output_archive << players_and_tokens;

        //
        //  Чтобы данные в файле не портились, когда приложение падает при записи,
        //  используют такой приём:
        //  Сперва записать данные во временный файл. После этого на диске будут два файла — новый и целевой.
        //  Переименовать временный файл в целевой.
        //
        output_stream.close();
        std::filesystem::rename(tmp_file_name, state_file);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Save state error: " << e.what() << std::endl;
    }
}

void LoadGameState(const std::filesystem::path& state_file, model::Game::Ptr game, app::Application::Ptr app) {

    if (!std::filesystem::exists(state_file)) {
        //
        //  Если файл, путь к которому задан в --state-file,
        //  на диске не существует, сервер должен начать работу с чистого листа.
        //
        return;
    }

    std::ifstream input_stream{state_file};
    if (!input_stream) {
        //
        //  Если при восстановлении состояния возникла ошибка, сервер должен:
        //      - в log вывести сообщение об ошибке;
        //      - завершить работу и вернуть из функции код ошибки EXIT_FAILURE.
        //
        throw std::invalid_argument("Could not open state file. Check file permissions.");
    }
    
    
    boost::archive::text_iarchive input_archive{input_stream};

    std::vector<SessionRepr> sessions;

    input_archive >> sessions;

    std::vector<PlayerRepr> players_and_tokens;

    input_archive >> players_and_tokens;

    for (auto& session_repr : sessions) {
        auto* session = session_repr.Restore(game);
        if (!session)
            continue;

        for (const auto& player_and_token : players_and_tokens) {
            if (player_and_token.GetMapId() != session->GetMap().GetId())
                continue;

            app->AddPlayer(player_and_token.RestoreToken(), player_and_token.RestorePlayer(*session));
            
        }
        
    }

}

} // namespace serialization