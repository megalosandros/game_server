#pragma once
#include <string>
#include <string_view>
#include <deque>

#include "model_units.h"
#include "loot_generator.h"
#include "collision_detector.h"

namespace model {

using namespace std::literals;

class LootGathererProvider : public geom::ItemGathererProvider {
public:
    LootGathererProvider() = default; // только для юнит тестов нужен
    LootGathererProvider(std::vector<geom::Item>&& items, std::vector<geom::Gatherer>&& gatherers) 
    : items_(std::move(items))
    , gatherers_(std::move(gatherers)) {

    }

    size_t ItemsCount() const override {
        return items_.size();
    }

    geom::Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }

    size_t GatherersCount() const override {
        return gatherers_.size();
    }

    geom::Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

    LootGathererProvider& AddItem(geom::Point2D position, double width, size_t id) {
        items_.emplace_back(geom::Item{position, width, id});
        return *this;
    }

    LootGathererProvider& AddItem(geom::Item&& item) {
        items_.emplace_back(std::move(item));
        return *this;
    }

    LootGathererProvider& AddGatherer(geom::Point2D start_pos, geom::Point2D end_pos, double width, size_t id) {
        gatherers_.emplace_back(geom::Gatherer{start_pos, end_pos, width, id});
        return *this;
    }

    LootGathererProvider& AddGatherer(geom::Gatherer&& gatherer) {
        gatherers_.emplace_back(std::move(gatherer));
        return *this;
    }


private:
    std::vector<geom::Item> items_;
    std::vector<geom::Gatherer> gatherers_;
};


class Loot {
public:
    using Id = std::uint32_t;
    using Type = std::uint32_t;
    using Value = std::uint32_t;
    static constexpr Real WIDTH = 0.0;

    struct Traits {
        Id id;
        Type type;
        Value value;

        [[nodiscard]] auto operator<=>(const Traits&) const = default;
    };

    Loot(Id id, Type type, Value value, const geom::Point2D& pos);
    Loot(const Loot &other) = default;
    Loot& operator=(const Loot& other) = default;
    Loot(Loot &&other) noexcept = default;
    Loot& operator=(Loot&& other) noexcept = default;

    Id GetId() const noexcept {
        return traits_.id;
    }

    Type GetType() const noexcept {
        return traits_.type;
    }

    Value GetValue() const noexcept {
        return traits_.value;
    }

    const Traits& GetTraits() const noexcept {
        return traits_;
    }

    const geom::Point2D& GetPos() const noexcept {
        return pos_;
    }

    bool operator==(Loot::Id otherLootId) const noexcept {
        return (traits_.id == otherLootId);
    }

private:
    Traits traits_;
    geom::Point2D pos_;
};


class Dog
{
public:
    using Id  = std::uint32_t;
    using Score = std::uint32_t;
    using Bag = std::vector<Loot::Traits>;
    static constexpr Real WIDTH = 0.6;

    enum class Direction : char { 
        Left  = 'L',
        Right = 'R',
        Up    = 'U',
        Down  = 'D',
        Stop  =  0
    };

    //
    //  Конструктор для создания собаки в процессе работы
    //
    Dog(Id id, const std::string &name, const geom::Point2D& pos, Real max_speed, size_t bag_capacity);

    //
    //  Конструктор для восстановления состояния
    //
    Dog(Id id, const std::string &name,
        const geom::Point2D& pos, const geom::Vec2D& speed, Direction dir,
        const Bag& bag, Score score,
        Real max_speed, size_t bag_capacity,
        TimeInterval::rep play_time, TimeInterval::rep idle_time);

    //  Это все нужно для сохранения/восстановления
    Dog(const Dog &) = default;
    Dog &operator=(const Dog &) = default;
    Dog(Dog &&) noexcept = default;
    Dog &operator=(Dog &&) noexcept = default;

    //
    //  результат каждого перемещения - начальная и конечная точки собаки
    //  их потом легко поместить в алгоритм поиска коллизий
    //
    geom::Gatherer Move(const class Map& map, TimeInterval dt);

    //
    //  пробую подобрать трофей - но он может не влезть в мешок
    //  если трофей подобрался - true, нет - false
    // 
    bool TryGatherLoot(const Loot& loot);

    //
    //  сгружаю трофеи на базу, за каждый трофей увеличиваю счет
    //  на стоимость трофея
    //
    void UnloadBag();

    Id GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const geom::Point2D& GetPos() const noexcept {
        return pos_;
    }

    const geom::Vec2D& GetSpeed() const noexcept {
        return speed_;
    }

    const Bag& GetBag() const noexcept {
        return bag_;
    }

    void ChangeDir(Direction dir) noexcept;

    Direction GetDir() const noexcept {
        return dir_;
    }

    Score GetScore() const noexcept {
        return score_;
    }

    size_t GetBagCapacity() const noexcept {
        return bag_capacity_;
    }

    Real GetMaxSpeed() const noexcept {
        return max_speed_;
    }

    TimeInterval GetIdleTime() const noexcept {
        return idle_time_;
    }

    TimeInterval GetPlayTime() const noexcept {
        return play_time_;
    }

    bool operator==(Dog::Id otherDogId) const noexcept {
        return (id_ == otherDogId);
    }

private:
    Id id_;
    std::string name_;
    geom::Point2D pos_ = { 0, 0 };
    geom::Vec2D speed_ = { 0, 0 };
    Direction dir_ = Direction::Up;
    Bag bag_;
    Score score_ = 0;
    Real max_speed_;
    size_t bag_capacity_;
    TimeInterval play_time_{};
    TimeInterval idle_time_{};
};


class GameSession {
    // запрещаю копирование и перемещение, чтобы нельзя было положить объект в нестабильный контейнер
    GameSession(const GameSession &) = delete;
    GameSession &operator=(const GameSession &) = delete;
    GameSession(GameSession &&) = delete;
    GameSession &operator=(GameSession &&) = delete;
public:
    using Dogs = std::deque<Dog>;
    using Loots = std::deque<Loot>;

    GameSession(const class Map &map, TimeInterval base_interval, double probability);

    Dog* AddDog(const std::string& dogName, bool randomize_spawn_point);
    Dog* FindDog(Dog::Id id) const noexcept;
    void RemoveDog(Dog::Id id);
    Loot *FindLoot(Loot::Id id) const noexcept;
    void  RemoveLoot(Loot::Id id);
    void  SetDogs(Dogs&& dogs, Dog::Id next_dog_id);
    void  SetLoots(Loots&& loots, Loot::Id next_loot_id);

    //
    //  Результат генерации трофеев - массив "элементов" для алгоритма
    //  поиска коллизий, а результат перемещения всех собак на карте -
    //  массив сборщиков
    //
    std::vector<geom::Item> GenerateLoots(TimeInterval timeDelta);
    std::vector<geom::Gatherer> MoveDogs(TimeInterval timeDelta);
    void GatherLoots(const LootGathererProvider& provider);
    
    const class Map &GetMap() const noexcept {
        return map_;
    }

    const Dogs& GetDogs() const noexcept {
        return dogs_;
    }

    const Loots& GetLoots() const noexcept {
        return loots_;
    }

    static Dog::Id GetNextDogId() noexcept {
        return next_dog_id_;
    }

    static Loot::Id GetNextLootId() noexcept {
        return next_loot_id_;
    }

private:
    geom::Point2D GenerateRandomPoint(bool randomize_point) const;
    void AddLoot(Loot::Type type);

private:
    static Dog::Id next_dog_id_;
    static Loot::Id next_loot_id_;
    const class Map &map_;
    Dogs dogs_;
    Loots loots_;
    loot_gen::LootGenerator loot_generator_;
};

} // namespace model