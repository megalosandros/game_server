#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <list>

#include "game_session.h"
#include "model.h"
#include "player_tokens.h"

namespace app {

struct PlayerStatistics {
    std::string name;
    model::Dog::Score score{0};
    model::TimeInterval play_time_ms{0};
};


class Player {
public:
    using Id = model::Dog::Id;

    Player(const Player&) = default;
    Player &operator=(const Player&) = default;
    Player(Player &&) = default;
    Player &operator=(Player &&) = default;
    Player(model::GameSession &session, Id id);

    void ChangeDir(model::Dog::Direction dir) const noexcept;
    void DismissDog();

    const std::string &GetName() const noexcept;
    model::TimeInterval GetIdleTime() const noexcept;
    PlayerStatistics GetStatistics() const noexcept;

    Id GetId() const noexcept {
        return id_;
    }

    const model::Map& GetMap() const noexcept {
        return session_->GetMap();
    }

    const model::GameSession& GetSession() const noexcept {
        return *session_;
    }

    bool operator==(Id otherPlayerId) const noexcept {
        return (id_ == otherPlayerId);
    }

private:
    model::Dog* GetDog() const noexcept;

    model::GameSession *session_;
    Id id_;
};


class Players {
public:
    using Ptr = std::shared_ptr<Players>;
    using List = std::list<Player>;
    using Pairs = std::vector<std::pair<Token, const Player*>>;

    Players() = default;

    void AddPlayer(Token token, model::GameSession& session, model::Dog::Id id);
    void AddPlayer(Token token, Player&& player);
    PlayerStatistics RemovePlayer(const Token& token);
    Player *FindPlayer(const Token &token) const noexcept;
    Pairs GetPairs() const;
    const List& GetPlayers() const noexcept {
        return players_;
    }

private:
    using TokenToPlayer = std::unordered_map<Token, Player*, util::TaggedHasher<Token>>;

    Player *RemoveToken(const Token& token);
    void RemovePlayer(Player::Id id);

    List players_;
    TokenToPlayer token_to_player_;
};


} // namespace app