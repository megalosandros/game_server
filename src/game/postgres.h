#pragma once
#include <pqxx/pqxx>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/zview.hxx>

#include "player.h"

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

using RecordsResult = std::vector<app::PlayerStatistics>;

class Database {
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

public:
    explicit Database(const std::string& db_url);

    void SaveRecord(const app::PlayerStatistics& player);
    RecordsResult GetRecords(int start, int max_count);

private:
    void CreateTablesIf();
    void PrepareQueries();

    pqxx::connection connection_;
    static constexpr auto tag_select_query_ = "select_query"_zv;
    static constexpr auto tag_insert_query_ = "insert_query"_zv;
};

} // namespace postgres