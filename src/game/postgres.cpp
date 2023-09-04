#include "../sdk.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "postgres.h"

namespace postgres {


Database::Database(const std::string& db_url)
: connection_{db_url} {

    CreateTablesIf();
    PrepareQueries();
}

void Database::CreateTablesIf() {

    pqxx::work work{connection_};
    
    work.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id UUID CONSTRAINT id_constraint PRIMARY KEY,
            name varchar(100) NOT NULL,
            score integer,
            play_time_ms integer);
            )"_zv);

    work.exec(R"(
        CREATE INDEX IF NOT EXISTS retired_players_sort ON retired_players (
            score DESC, play_time_ms, name);
            )"_zv);

    work.commit();

}

void Database::PrepareQueries() {
    connection_.prepare(tag_select_query_, R"(SELECT name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name LIMIT $1 OFFSET $2;)"_zv);
    connection_.prepare(tag_insert_query_, R"(INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);)"_zv);
}

void Database::SaveRecord(const app::PlayerStatistics& player) {
    pqxx::work work{connection_};
    work.exec_prepared(
        tag_insert_query_,
        to_string(boost::uuids::random_generator()()),
        player.name,
        player.score,
        player.play_time_ms.count()
        );
    work.commit();

}

RecordsResult Database::GetRecords(int start, int max_count) {

    RecordsResult result;
    pqxx::read_transaction r(connection_);

    for (auto [name, score, play_time_ms] : r.exec_prepared(tag_select_query_, max_count, start).iter<std::string, uint32_t, uint64_t>()) {
        result.emplace_back(name, score, std::chrono::milliseconds{play_time_ms});
    }

    return result;
}

} // namespace postgres
