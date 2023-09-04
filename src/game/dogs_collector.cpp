#include "../sdk.h"
#include "dogs_collector.h"
#include <iostream>

namespace collector {

DogsCollector::DogsCollector(model::Game::Ptr game, app::Players::Ptr players, postgres::Database& db)
: game_(game)
, players_(players)
, db_(db) {

}

void DogsCollector::CollectRetiredDogs() noexcept {
    try {
        //
        //  получаю массив токенов и пользователей, для каждого
        //  пользователя получаю время простоя, если время простоя 
        //  стало больше или равно "dogRetirementTime" - игрок удаляется
        //

        auto tokens = players_->GetPairs();

        for (const auto &token : tokens) {
            if (token.second->GetIdleTime() >= game_->GetRetirementTime()) {
                auto statistics = players_->RemovePlayer(token.first);
                db_.SaveRecord(statistics);
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error while removing player: " << e.what() << std::endl;
    }
}


} // namespace collector