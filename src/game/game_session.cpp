#include "../sdk.h"
#include "math_utils.h"
#include "model.h"
#include "game_session.h"
#include <iostream>
#include <numeric>

namespace model {


Dog::Dog(Id id, const std::string &name, const geom::Point2D& pos, Real max_speed, size_t bag_capacity)
: id_(id)
, name_(name)
, pos_(pos)
, max_speed_(max_speed)
, bag_capacity_(bag_capacity) {
}

Dog::Dog(Id id, const std::string &name,
        const geom::Point2D& pos, const geom::Vec2D& speed, Direction dir,
        const Bag& bag, Score score,
        Real max_speed, size_t bag_capacity,
        TimeInterval::rep play_time, TimeInterval::rep idle_time) 
: id_(id)
, name_(name)
, pos_(pos)
, speed_(speed)
, dir_(dir)
, bag_(bag)
, score_(score)
, max_speed_(max_speed)
, bag_capacity_(bag_capacity)
, play_time_(play_time)
, idle_time_(idle_time) {
}


//
//  пробую подобрать трофей - но он может не влезть в мешок
//  если трофей подобрался - true, нет - false
// 
bool Dog::TryGatherLoot(const Loot& loot)
{
    if (bag_.size() < bag_capacity_) {
        bag_.emplace_back(loot.GetId(), loot.GetType(), loot.GetValue());
        return true;
    }

    return false;
}

//
//  сгружаю трофеи на базу, за каждый трофей увеличиваю счет
//  на стоимость трофея
//
void Dog::UnloadBag() {
    //
    //  Добавить в копилку стоимость каждого трофея - и затем
    //  очистить мешок
    //
    // score_ = std::accumulate(bag_.begin(), bag_.end(), score_, 
    //     [](uint32_t score, const Loot::Traits &item) { 
    //         return score + item.value; 
    //     }
    // );

    for (const auto &item : bag_) {
        score_ += item.value;
    }

    bag_.clear();
}

void Dog::ChangeDir(Direction dir) noexcept
{
    //
    //  TODO: корректно обработать случай когда направление не изменилось,
    //  чтобы не делать лишней работы
    //

    //
    //  при остановке персонажа я не меняю направление движения
    //
    dir_ = (dir != Direction::Stop) ? dir : dir_;

    //
    //  если задано какое-то направление движения - нужно сбросить
    //  счетчик простоя; при этом кажется странным что счетчик сбрасывается даже при остановке
    //  собаки - но автотесты заточены именно на такое поведение
    //
    if (idle_time_.count() != 0) {
        idle_time_ = 0ms;
    }
    
    switch (dir)
    {
    case Direction::Left:
        speed_ = {-max_speed_, 0};
        break;

    case Direction::Right:
        speed_ = {max_speed_, 0};
        break;

    case Direction::Up:
        speed_ = {0, -max_speed_};
        break;

    case Direction::Down:
        speed_ = {0, max_speed_};
        break;

    default:
        speed_ = {0, 0};
    }
}

//
//  найти дорогу, на которой находится собака
//  если собака движется горизонтально - предпочтение отдается
//  горизонтальной дороге, если вертикально - вертикальной
//
geom::Rect2D FindDogRoad(const Map::Roads& roads, const geom::Point2D& dog_pos, const geom::Vec2D& dog_speed)
{
    geom::Rect2D altRect = { 0, 0, 0, 0 };

    for (const auto& road : roads) {

        geom::Rect2D rect = util::MakeRect2D(road);
        if (!rect.Test(dog_pos)) {
            continue;
        }
        if (util::IsHorizontal(dog_speed) == util::IsHorizontal(rect)) {
            return rect;
        }

        altRect = rect;
    }

    return altRect;
}

geom::Point2D MoveDog(const geom::Point2D& dog_pos, const geom::Vec2D& dog_speed, TimeInterval dt)
{
    geom::Point2D newPos = {};

    newPos.x = (util::IsZero(dog_speed.x)) ? dog_pos.x : (dog_pos.x + dog_speed.x * util::TimeDeltaToSeconds(dt));
    newPos.y = (util::IsZero(dog_speed.y)) ? dog_pos.y : (dog_pos.y + dog_speed.y * util::TimeDeltaToSeconds(dt));

    return newPos;
}

geom::Point2D FindBoundary(const geom::Rect2D& rect, const geom::Point2D& dog_pos, Dog::Direction dir) {

    geom::Point2D boundary{};

    switch (dir) {
    case Dog::Direction::Right:
        boundary.y = dog_pos.y;
        boundary.x = rect.right;
        break;

    case Dog::Direction::Left:
        boundary.y = dog_pos.y;
        boundary.x = rect.left;
        break;

    case Dog::Direction::Up:
        boundary.x = dog_pos.x;
        boundary.y = rect.top;
        break;

    case Dog::Direction::Down:
        boundary.x = dog_pos.x;
        boundary.y = rect.bottom;
        break;

    default:
        // warning: enumeration value ‘Stop’ not handled in switch
        break;
    }

    return boundary;
}

//
//  результат каждого перемещения - начальная и конечная точки собаки
//  их потом легко поместить в алгоритм поиска коллизий
//
geom::Gatherer Dog::Move(const class Map& map, TimeInterval dt)
{
    //
    //  В любом случае с каждым движением увеличивается время в игре
    //
    play_time_ += dt;

    //
    //  Если скорость собаки равна нулю - то она никуда не движется,
    //  но увеличивается время простоя
    //
    if (util::IsZero(speed_)) {
        idle_time_ += dt;
        return {pos_, pos_, WIDTH, id_};
    }

    //
    //  Получить список дорог
    //
    const auto &roads = map.GetRoads();

    //
    //  Найти дорогу, по которой движется собака
    //  Если дороги нашлось две (перекресток), выбрать
    //  ту дорогу, направление которой совпадает с направлением
    //  движения собаки
    //
    const auto rect = FindDogRoad(roads, pos_, speed_);
    
    //
    //  Может ли собака продвинуться по дороге на дельту
    //  и не уйти за пределы?
    //
    auto newPos = MoveDog(pos_, speed_, dt);
    if (rect.Test(newPos)) {
        auto oldPos = std::exchange(pos_, newPos);
        return {oldPos, pos_, WIDTH, id_};
    }

    //
    //  При полном движении собака уходит за пределы дороги
    //  Нужно дойти до края дороги и остановиться на нем
    //
    auto oldPos = std::exchange(pos_, FindBoundary(rect, pos_, dir_));
    speed_ = {0, 0};

    return {oldPos, pos_, WIDTH, id_};

}

Loot::Loot(Id id, Type type, Value value, const geom::Point2D& pos)
: traits_{id, type, value}
, pos_(pos)
{
}


GameSession::GameSession(const Map &map, TimeInterval base_interval, double probability)
: map_(map)
, loot_generator_(base_interval, probability) {    
}


//
//  Идентификаторы начинаю с единицы, потому что ноль
//  зарезервирован под идентификатор(ы) базы
//
Dog::Id GameSession::next_dog_id_ = 1;
Loot::Id GameSession::next_loot_id_ = 1;

Dog* GameSession::AddDog(const std::string& dogName, bool randomize_spawn_point) {

    geom::Point2D pt = GenerateRandomPoint(randomize_spawn_point);

    auto& dog = dogs_.emplace_back(next_dog_id_++, dogName, pt, map_.GetDogSpeed(), map_.GetBagCapacity());

    return &dog;
}

void GameSession::AddLoot(Loot::Type type) {
    
    geom::Point2D pt = GenerateRandomPoint(true);

    loots_.emplace_back(next_loot_id_++, type, map_.GetLootTypeValue(type), pt);

}

Dog* GameSession::FindDog(Dog::Id id) const noexcept {

    if (auto it = std::find(dogs_.begin(), dogs_.end(), id); it != dogs_.end()) {
        return const_cast<Dog*>(&(*it));
    }

    return nullptr;
}

void GameSession::RemoveDog(Dog::Id id) {

    if (auto it = std::find(dogs_.begin(), dogs_.end(), id); it != dogs_.end()) {
        dogs_.erase(it);
    }

}


Loot* GameSession::FindLoot(Loot::Id id) const noexcept {

    if (auto it = std::find(loots_.begin(), loots_.end(), id); it != loots_.end()) {
        return const_cast<Loot*>(&(*it));
    }

    return nullptr;
}

void GameSession::RemoveLoot(Loot::Id id) {

    if (auto it = std::find(loots_.begin(), loots_.end(), id); it != loots_.end()) {
        loots_.erase(it);
    }

}

void GameSession::SetDogs(Dogs&& dogs, Dog::Id next_dog_id) {
    dogs_ = std::move(dogs);
    next_dog_id_ = next_dog_id;
}


void GameSession::SetLoots(Loots&& loots, Loot::Id next_loot_id) {
    loots_ = std::move(loots);
    next_loot_id_ = next_loot_id;
}

//
//  После генерации новых трофеев я получаю массив "элементов", который можно использовать
//  в алгоритме поиска коллизий
//
std::vector<geom::Item> GameSession::GenerateLoots(TimeInterval timeDelta) {

    const auto loot_count = loot_generator_.Generate(timeDelta, loots_.size(), dogs_.size());

    if (loot_count != 0) {
        //
        //  Количество типов трофеев никак не может быть равно нулю
        //
        const auto loot_types_count = map_.GetLootTypeCount();
        for (unsigned i = 0; i < loot_count; ++i) {
            AddLoot(std::rand() % loot_types_count);
        }
    }

    std::vector<geom::Item> items;

    for (const auto& loot : loots_) {
        items.emplace_back(loot.GetPos(), loot.WIDTH, loot.GetId());
    }

    return items;
}

//
//  После перемещения всех собак я получаю массив "собирателей", который затем
//  используется в алгоритме поиска коллизий
//
std::vector<geom::Gatherer> GameSession::MoveDogs(TimeInterval timeDelta) {

    std::vector<geom::Gatherer> gatherers;

    for (Dog& dog : dogs_) {
        gatherers.emplace_back(dog.Move(map_, timeDelta));
    }

    return gatherers;
}

bool IsOffice(size_t item_id) {
    return (item_id == 0);
}

void GameSession::GatherLoots(const LootGathererProvider& provider) {

    //
    //  Найти пересечения собак, трофеев и офисов
    //
    auto collectionEvents = geom::FindGatherEvents(provider);

    for (const auto& event : collectionEvents) {

        auto* dog = FindDog(event.gatherer_id);

        assert(dog && "Dog must be always!");

        if (IsOffice(event.item_id)) {
            //
            //  пересечение с офисом - выгружаю трофеи
            //
            dog->UnloadBag();
        }
        else {
            //
            //  Пересечение с трофеем, нужно проверить не забрал ли его уже кто-то
            //
            auto* loot = FindLoot(event.item_id);
            if  (!loot) {
                continue; // ok - трофей уже забрал более быстрый собакен
            }

            //
            //  пробую подобрать трофей, если получилось удаляю
            //  трофей с карты, чтобы больше никто не мог его подобрать
            //  трофей может не подобраться, если мешок уже полон
            //
            if (dog->TryGatherLoot(*loot)) {
                RemoveLoot(loot->GetId());
            }

        }
    }

}

geom::Point2D GameSession::GenerateRandomPoint(bool randomize_point) const
{
    const auto &roads = map_.GetRoads();
    if (roads.empty()) {
        //
        //  с точки зрения игры карта без дорог - это серьезная ошибка, но
        //  с точки зрения генератора случайной координаты это не страшно, 
        //  поэтому просто return
        //
        return {0, 0};
    }

    if (!randomize_point) {
        //
        //  Чтобы было легче протестировать вашу программу автотестами,
        //  временно доработайте код так, чтобы после входа в игру пёс игрока появлялся
        //  в начальной точке первой дороги карты, а не в случайно сгенерированной.
        //
        const auto &start = roads[0].GetStart();
        
        return {static_cast<double>(start.x), static_cast<double>(start.y)};
    }

    //
    //  а здесь все по-взрослому:
    //  выбираю "случайную" дорогу и на этой дороге
    //  выбираю "случайную" координату
    //

    geom::Point2D pos = {};

    const size_t roads_count = roads.size();
    const size_t road_index = std::rand() % roads_count;
    const auto &road = roads[road_index];
    const auto &start = road.GetStart();
    const auto &end = road.GetEnd();
   
    if (road.IsVertical()) {
        //
        //  фиксированный X, изменяемый Y
        //
        auto y = std::min(start.y, end.y) + (std::rand() % std::abs(start.y - end.y));
        pos = {static_cast<double>(start.x), static_cast<double>(y)};
    }
    else {
        //
        //  изменяемый X, фиксированный Y
        //
        auto x = std::min(start.x, end.x) + (std::rand() % std::abs(start.x - end.x));
        pos = {static_cast<double>(x) , static_cast<double>(end.y)};
    }

    return pos;
}

} // namespace model