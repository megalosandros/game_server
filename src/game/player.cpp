#include "../sdk.h"
#include "player.h"
#include <iostream>

namespace app {


Player::Player(model::GameSession &session, Id id)
: session_(&session)
, id_(id)
{    
}

model::Dog* Player::GetDog() const noexcept {
    return session_->FindDog(id_);
}

const std::string& Player::GetName() const noexcept {

    static const std::string UNKNOWN_NAME{};

    if (auto* dog = GetDog()) {
        return dog->GetName();
    }

    return UNKNOWN_NAME;
}

model::TimeInterval Player::GetIdleTime() const noexcept {

    if (auto* dog = GetDog()) {
        return dog->GetIdleTime();
    }

    return model::TimeInterval{};
}

PlayerStatistics Player::GetStatistics() const noexcept {
    
    if (auto* dog = GetDog()) {
        return {dog->GetName(), dog->GetScore(), dog->GetPlayTime()};
    }

    return {};
}

//
//  const здесь выглядит странно - но я же меняю направление
//  не у игрока, а у его собаки :)
//
void Player::ChangeDir(model::Dog::Direction dir) const noexcept {
    if (auto* dog = GetDog()) {
        dog->ChangeDir(dir);
    }
}

void Player::DismissDog() {
    session_->RemoveDog(id_);
}

void Players::AddPlayer(Token token, model::GameSession& session, model::Dog::Id id)
{
    AddPlayer(token, {session, id});
}

void Players::AddPlayer(Token token, Player&& player) {
        
    auto& ref = players_.emplace_back(std::move(player));

    try {
        token_to_player_.emplace(std::move(token), &ref);
    }
    catch (...)
    {
        // Удаляем игрока из вектора, если не удалось вставить в unordered_map
        players_.pop_back();
        throw;
    }


}

PlayerStatistics Players::RemovePlayer(const Token& token) {

    Player* player = RemoveToken(token);
    if (!player) {
        throw std::logic_error("Token not found");
    }

    //
    //  Статистика игры - это все, что останется от игрока
    //
    PlayerStatistics remains = player->GetStatistics();

    //
    //  Удалить из сессии собаку игрока, а затем уже удалить самого игрока
    //
    player->DismissDog();
    RemovePlayer(player->GetId());

    //
    //  вернуть остатки для записи в БД
    //
    return remains;
}

Player *Players::RemoveToken(const Token& token) {

    auto it = token_to_player_.find(token);
    if (it == token_to_player_.end()) {
        return nullptr;
    }

    Player *player = it->second;

    token_to_player_.erase(it);

    return player;
}

void Players::RemovePlayer(Player::Id id) {

    if (auto it = std::find(players_.begin(), players_.end(), id); it != players_.end()) {
        players_.erase(it);
    }
}

Player *Players::FindPlayer(const Token &token) const noexcept
{
    if (auto it = token_to_player_.find(token); it != token_to_player_.end()) {
        return it->second;
    }

    return nullptr;
}

Players::Pairs Players::GetPairs() const {

    Pairs pairs;

    for (const auto& pair : token_to_player_) {
        pairs.emplace_back(std::make_pair(pair.first, pair.second));
    }

    return pairs;
}


} // namespace app
