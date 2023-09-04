#pragma once
#include "postgres.h"
#include "model.h"
#include "player.h"

namespace collector {

//
//  Сборщик ушедших на покой игроков и их собак
//
class DogsCollector
{
    // не нужно это
    DogsCollector(const DogsCollector&) = delete;
    DogsCollector& operator=(const DogsCollector&) = delete;
    DogsCollector(DogsCollector&&) = delete;
    DogsCollector& operator=(DogsCollector&&) = delete;

public:
    DogsCollector(model::Game::Ptr game, app::Players::Ptr players, postgres::Database& db);

    void CollectRetiredDogs() noexcept;

private:
    model::Game::Ptr    game_;
    app::Players::Ptr   players_;
    postgres::Database& db_;
};

} // namespace collector