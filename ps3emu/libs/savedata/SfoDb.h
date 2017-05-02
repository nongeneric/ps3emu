#pragma once

#include <string_view>
#include <optional>
#include <variant>
#include <memory>

namespace sql { class SQLiteDB; }

using SfoValue = std::variant<std::string, uint32_t>;

class SfoDb {
    std::unique_ptr<sql::SQLiteDB> _db;
public:
    void open(std::string_view path);
    std::optional<SfoValue> findKey(std::string_view key);
    void setValue(std::string_view key, SfoValue value);
    SfoDb();
    ~SfoDb();
};
