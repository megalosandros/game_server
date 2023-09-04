#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include "../src/game/loot_generator.h"
#include "../src/game/model.h"
#include "../src/game/app.h"

using namespace std::literals;

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

SCENARIO("Join game") {
    postgres::Database db{GetDatabaseUrlFromEnv()};
    auto game = std::make_shared<model::Game>(5s, 0.5, 1min);
    auto app = std::make_shared<app::Application>(game, db, true);

}

SCENARIO("Loot generation") {
    using loot_gen::LootGenerator;
    using TimeInterval = LootGenerator::TimeInterval;


    GIVEN("a loot generator") {
        LootGenerator gen{1s, 1.0};

        constexpr TimeInterval TIME_INTERVAL = 1s;

        WHEN("loot count is enough for every looter") {
            THEN("no loot is generated") {
                for (unsigned looters = 0; looters < 10; ++looters) {
                    for (unsigned loot = looters; loot < looters + 10; ++loot) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == 0);
                    }
                }
            }
        }

        WHEN("number of looters exceeds loot count") {
            THEN("number of loot is proportional to loot difference") {
                for (unsigned loot = 0; loot < 10; ++loot) {
                    for (unsigned looters = loot; looters < loot + 10; ++looters) {
                        INFO("loot count: " << loot << ", looters: " << looters);
                        REQUIRE(gen.Generate(TIME_INTERVAL, loot, looters) == looters - loot);
                    }
                }
            }
        }
    }

    GIVEN("a loot generator with some probability") {
        constexpr TimeInterval BASE_INTERVAL = 1s;
        LootGenerator gen{BASE_INTERVAL, 0.5};

        WHEN("time is greater than base interval") {
            THEN("number of generated loot is increased") {
                CHECK(gen.Generate(BASE_INTERVAL * 2, 0, 4) == 3);
            }
        }

        WHEN("time is less than base interval") {
            THEN("number of generated loot is decreased") {
                const auto time_interval
                    = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
                        1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
                CHECK(gen.Generate(time_interval, 0, 4) == 1);
            }
        }
    }

    GIVEN("a loot generator with custom random generator") {
        LootGenerator gen{1s, 0.5, [] {
                              return 0.5;
                          }};
        WHEN("loot is generated") {
            THEN("number of loot is proportional to random generated values") {
                const auto time_interval
                    = std::chrono::duration_cast<TimeInterval>(std::chrono::duration<double>{
                        1.0 / (std::log(1 - 0.5) / std::log(1.0 - 0.25))});
                CHECK(gen.Generate(time_interval, 0, 4) == 0);
                CHECK(gen.Generate(time_interval, 0, 4) == 1);
            }
        }
    }

}
