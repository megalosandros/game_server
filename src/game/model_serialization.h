#include <filesystem>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>

#include "model.h"
#include "game_session.h"
#include "player_tokens.h"
#include "player.h"
#include "app.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, Map::Id& id, [[maybe_unused]] const unsigned version) {
    ar& (*id);
}


template <typename Archive>
void serialize(Archive& ar, Loot::Traits& obj, [[maybe_unused]] const unsigned version) {
    ar&(obj.id);
    ar&(obj.type);
    ar&(obj.value);
}


}  // namespace model

namespace app {

template <typename Archive>
void serialize(Archive& ar, Token& token, [[maybe_unused]] const unsigned version) {
    ar& (*token);
}

} // namespace app


namespace serialization {


using namespace std::literals;

//
// DogRepr (DogRepresentation) - сериализованное представление класса Dog
//
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPos())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDir())
        , score_(dog.GetScore())
        , bag_content_(dog.GetBag()) 
        , max_speed_(dog.GetMaxSpeed()) 
        , bag_capacity_(dog.GetBagCapacity()) 
        , play_time_(dog.GetPlayTime().count())
        , idle_time_(dog.GetIdleTime().count()) {
    }

    [[nodiscard]] model::Dog Restore() const {
        return {id_, name_, pos_, speed_, direction_, bag_content_, score_, max_speed_, bag_capacity_, play_time_, idle_time_};
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
        ar& max_speed_;
        ar& bag_capacity_;
        ar& play_time_;
        ar& idle_time_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    geom::Vec2D speed_;
    model::Dog::Direction direction_ = model::Dog::Direction::Up;
    model::Dog::Score score_ = 0;
    model::Dog::Bag bag_content_;
    model::Real max_speed_ = 0.;
    size_t bag_capacity_ = 0;
    model::TimeInterval::rep play_time_ = 0;
    model::TimeInterval::rep idle_time_ = 0;
};


//
// LootRepr (LootRepresentation) - сериализованное представление потерянного предмета
//
class LootRepr {
public:
    LootRepr() = default;

    explicit LootRepr(const model::Loot& loot)
        : traits_(loot.GetTraits())
        , pos_(loot.GetPos()) {
    }

    [[nodiscard]] model::Loot Restore() const {
        return {traits_.id, traits_.type, traits_.value, pos_};
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& traits_;
        ar& pos_;
    }

private:
    model::Loot::Traits traits_;
    geom::Point2D pos_;
};


//
// SessionRepr (SessionRepresentation) - сериализованное представление сессии, 
//  которая является контейнером для собак и предметов на определенной карте
//
class SessionRepr {
public:
    SessionRepr() = default;

    explicit SessionRepr(const model::GameSession& session)
        : id_(session.GetMap().GetId())
        , next_dog_id_(model::GameSession::GetNextDogId())
        , next_loot_id_(model::GameSession::GetNextLootId()) {

            const auto& dogs = session.GetDogs();
            for (const auto& dog : dogs) {
                dogs_.emplace_back(dog);
            }

            const auto& loots = session.GetLoots();
            for (const auto& loot : loots) {
                loots_.emplace_back(loot);
            }
    }

    [[nodiscard]] model::GameSession* Restore(model::Game::Ptr game) {

        if (auto* session = game->AddSession(id_); session != nullptr) {
            model::GameSession::Dogs restored_dogs;
            model::GameSession::Loots restored_loots;

            for (auto &dog : dogs_) {
                restored_dogs.emplace_back(dog.Restore());
            }
            for (auto &loot : loots_) {
                restored_loots.emplace_back(loot.Restore());
            }

            session->SetDogs(std::move(restored_dogs), next_dog_id_);
            session->SetLoots(std::move(restored_loots), next_loot_id_);

            return session;
        }

        return nullptr;
        
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& next_dog_id_;
        ar& next_loot_id_;
        ar& dogs_;
        ar& loots_;
    }

    model::Map::Id GetMapId() const noexcept {
        return id_;
    }

private:
    model::Map::Id id_{""s};
    model::Dog::Id next_dog_id_{1};
    model::Loot::Id next_loot_id_{1};
    std::vector<DogRepr> dogs_;
    std::vector<LootRepr> loots_;
};


class PlayerRepr {
public:
    PlayerRepr() = default;
    explicit PlayerRepr(const std::pair<app::Token, const app::Player*>& token_and_player)
    : token_(token_and_player.first)
    , player_id_(token_and_player.second->GetId())
    , player_map_id_(token_and_player.second->GetMap().GetId()) {

    }

    [[nodiscard]] app::Token RestoreToken() const noexcept {
        return token_;
    }

    [[nodiscard]] app::Player RestorePlayer(model::GameSession& session) const noexcept {
        return {session, player_id_};
    }

    const model::Map::Id& GetMapId() const noexcept {
        return player_map_id_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& token_;
        ar& player_id_;
        ar& player_map_id_;
    }

private:
    app::Token  token_{""s};
    app::Player::Id player_id_{0};
    model::Map::Id player_map_id_{""s};
};


class SerializationListener : public app::ApplicationListener
{
    // не нужно это
    SerializationListener(const SerializationListener&) = delete;
    SerializationListener& operator=(const SerializationListener&) = delete;
    SerializationListener(SerializationListener&&) = delete;
    SerializationListener& operator=(SerializationListener&&) = delete;

public:
    using TimeInterval = model::TimeInterval; //    std::chrono::milliseconds;

    SerializationListener(const std::string& state_file, TimeInterval save_period, model::Game::Ptr game, app::Application::Ptr application);

    void OnTick(model::TimeInterval timeDelta) noexcept override;

private:
    std::filesystem::path state_file_;
    TimeInterval save_period_;
    model::Game::Ptr game_;
    app::Application::Ptr application_;
    TimeInterval time_since_save_{};
};

void SaveGameState(const std::filesystem::path& state_file, model::Game::Ptr game, app::Application::Ptr app) noexcept;
void LoadGameState(const std::filesystem::path& state_file, model::Game::Ptr game, app::Application::Ptr app);

}  // namespace serialization
