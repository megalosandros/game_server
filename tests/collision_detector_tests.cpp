#define _USE_MATH_DEFINES

#include <cmath>
#include <functional>
#include <vector>
#include <sstream>


#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include "../src/game/collision_detector.h"
#include "../src/game/game_session.h"


using geom::ItemGathererProvider;
using geom::Item;
using geom::Gatherer;
using geom::GatheringEvent;
using model::LootGathererProvider;

namespace Catch {
template<>
struct StringMaker<GatheringEvent> {
  static std::string convert(GatheringEvent const& value) {
      std::ostringstream tmp;
      tmp << "(" << value.gatherer_id << "," << value.item_id << "," << value.sq_distance << "," << value.time << ")";

      return tmp.str();
  }
};
}  // namespace Catch 


bool operator==(const GatheringEvent &x,
                const GatheringEvent &y) {

    //
    //  Требования: при проверке совпадения числах с
    //  плавающей точкой используйте абсолютную погрешность 10⁻¹⁰
    //
    static constexpr double EPS = 1e-10;

    if (x.gatherer_id != y.gatherer_id || x.item_id != y.item_id)
        return false;

    if (std::abs(x.sq_distance - y.sq_distance) > EPS)
    {
        return false;
    }

    if (std::abs(x.time - y.time) > EPS)
    {
        return false;
    }
    return true;
}


std::pair<LootGathererProvider, std::vector<GatheringEvent>>
MakeGathererProvider_NoItems() {

    LootGathererProvider prov;

    prov
    .AddGatherer({1, 2}, {4, 2}, 5.0, 0)
    .AddGatherer({0, 0}, {10, 10}, 5.0, 1)
    .AddGatherer({-5, 0}, {10, 5}, 5.0, 2);

    return std::make_pair(prov, std::vector<GatheringEvent>{});
}

std::pair<LootGathererProvider, std::vector<GatheringEvent>>
MakeGathererProvider_NoGatherers() {

    LootGathererProvider prov;

    prov
    .AddItem({1, 2}, 5.0, 0)
    .AddItem({0, 0}, 5.0, 1)
    .AddItem({-5, 0}, 5.0, 2);

    return std::make_pair(prov, std::vector<GatheringEvent>{});
}

std::pair<LootGathererProvider, std::vector<GatheringEvent>>
MakeGathererProvider_ElevenItemsOneGatherer() {

    LootGathererProvider prov;

    prov.AddItem({9, 0.27}, 0.1, 0)
        .AddItem({8, 0.24}, 0.1, 1)
        .AddItem({7, 0.21}, 0.1, 2)
        .AddItem({6, 0.18}, 0.1, 3)
        .AddItem({5, 0.15}, 0.1, 4)
        .AddItem({4, 0.12}, 0.1, 5)
        .AddItem({3, 0.09}, 0.1, 6)
        .AddItem({2, 0.06}, 0.1, 7)
        .AddItem({1, 0.03}, 0.1, 8)
        .AddItem({0, 0.0}, 0.1, 9)
        .AddItem({-1, 0}, 0.1, 10);
    prov.AddGatherer({0, 0}, {10, 0}, 0.1, 0);

    std::vector<GatheringEvent> events;

    events.emplace_back(9, 0, 0.0*0.0, 0.0);
    events.emplace_back(8, 0, 0.03*0.03, 0.1);
    events.emplace_back(7, 0, 0.06*0.06, 0.2);
    events.emplace_back(6, 0, 0.09*0.09, 0.3);
    events.emplace_back(5, 0, 0.12*0.12, 0.4);
    events.emplace_back(4, 0, 0.15*0.15, 0.5);
    events.emplace_back(3, 0, 0.18*0.18, 0.6);

    return std::make_pair(prov, events);
}

std::pair<LootGathererProvider, std::vector<GatheringEvent>>
MakeGathererProvider_OneItemFourGatherers() {

    LootGathererProvider prov;

    prov.AddItem({0, 0}, 0.0, 0);

    prov.AddGatherer({-5, 0}, {5, 0}, 1.0, 0)
        .AddGatherer({0, 1}, {0, -1}, 1.0, 1)
        .AddGatherer({-10, 10}, {101, -100}, 0.5, 2)
        .AddGatherer({-100, 100}, {10, -10}, 0.5, 3);

    std::vector<GatheringEvent> events;

    events.emplace_back(0, 2, 0.*0., 0.0);

    return std::make_pair(prov, events);
}

std::pair<LootGathererProvider, std::vector<GatheringEvent>>
MakeGathererProvider_GatherersDontMove() {

    LootGathererProvider prov;

    prov.AddItem({0, 0}, 10.0, 0);

    prov.AddGatherer({-5, 0}, {-5, 0}, 1.0, 0)
        .AddGatherer({0, 0}, {0, 0}, 1.0, 1)
        .AddGatherer({-10, 10}, {-10, 10}, 100, 2);

    return std::make_pair(prov, std::vector<GatheringEvent>{});
}


TEST_CASE( "Collision detection", "[no-items]" ) {

    auto [prov, etalon] = MakeGathererProvider_NoItems();
    auto events = FindGatherEvents(prov);

    CHECK(events.size() == etalon.size());
}

TEST_CASE( "Collision detection", "[no-gatherers]" ) {

    auto [prov, etalon] = MakeGathererProvider_NoGatherers();
    auto events = FindGatherEvents(prov);

    CHECK(events.size() == etalon.size());
}

TEST_CASE( "Collision detection", "[multiple-items-one-gatherer]" ) {

    auto [prov, etalon] = MakeGathererProvider_ElevenItemsOneGatherer();
    auto events = FindGatherEvents(prov);

    REQUIRE(events.size() == etalon.size());

    for (size_t i = 0; i < etalon.size(); ++i) {
        CHECK(events[i] == etalon[i]);
    }
}

TEST_CASE( "Collision detection", "[one-item-multiple-gatherers]" ) {

    auto [prov, etalon] = MakeGathererProvider_OneItemFourGatherers();
    auto events = FindGatherEvents(prov);

    REQUIRE(!events.empty());
    CAPTURE(events[0]);
    CHECK(events[0].gatherer_id == etalon[0].gatherer_id);
}

TEST_CASE( "Collision detection", "[gatherers-dont-move]" ) {

    auto [prov, etalon] = MakeGathererProvider_GatherersDontMove();
    auto events = FindGatherEvents(prov);

    CHECK(events.empty());
}

