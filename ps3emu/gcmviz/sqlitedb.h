#include "sqlite3.h"

#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/hana.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

namespace sql {

class SqliteDB;
    
namespace hana = boost::hana;
using namespace boost::hana::literals;

class DatabaseOpeningException : public virtual std::exception {
public:
    const char* what() const noexcept override {
        return "Can't open the specified SQLite database file";
    }
};

class SQLException : public virtual std::exception {
    std::string _message;
public:
    inline SQLException(std::string message) : _message(message) { }
    const char* what() const noexcept override {
        return _message.c_str();
    }
};

template <typename FSeq>
void set(FSeq& seq, sqlite3_stmt* statement) {
    int i = 0;
    auto accessors = hana::accessors<FSeq>();
    auto members = hana::to_tuple(hana::members(seq));
    hana::for_each(hana::make_range(0_c, hana::size(seq)), [&](auto t) {
        auto m = members[t];
        using U = decltype(m);
        if constexpr(std::is_integral<U>::value) {
             hana::second(accessors[t])(seq) = sqlite3_column_int64(statement, i);
        } else if constexpr(std::is_same<U, std::string>::value) {
             hana::second(accessors[t])(seq)= std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement, i)));
        } else if constexpr(std::is_same<U, std::vector<uint8_t>>::value) {
            auto blob = sqlite3_column_blob(statement, i);
            auto typed = static_cast<const uint8_t*>(blob);
            auto count = sqlite3_column_bytes(statement, i);
            hana::second(accessors[t])(seq) = std::vector<uint8_t>(typed, typed + count);
        } else {
            static_assert(std::is_integral<U>::value && false, "unknown type");
        }
        i++;
    });
}

class Statement {
    sqlite3_stmt* _handle = nullptr;
    friend class SQLiteDB;
public:
    Statement() = default;
    Statement(Statement&) = delete;
    inline Statement(Statement&& s);
    inline Statement(std::string_view sql, SQLiteDB& db);
    inline ~Statement();
    inline Statement& operator=(Statement&& s);
};

template <typename FSeq>
std::vector<std::string> collectTypes() {
    std::vector<std::string> types;
    hana::for_each(hana::members(FSeq()), [&](auto t) {
        using U = decltype(t);
        if constexpr(std::is_integral<U>::value) {
            types.push_back("INT");
        } else if constexpr(std::is_same<U, std::string>::value){
            types.push_back("TEXT");
        } else if constexpr(std::is_same<U, float>::value) {
            types.push_back("FLOAT");
        } else if constexpr(std::is_same<U, std::vector<uint8_t>>::value) {
            types.push_back("BLOB");
        } else {
            static_assert(std::is_integral<U>::value && false, "unknown type");
        }
    });
    return types;
}

class SQLiteDB {
    sqlite3 *_db = nullptr;
    void throwSqlException(std::string error) {
        throw SQLException(error + " - (" + std::to_string(sqlite3_errcode(_db)) + ") " +
                           sqlite3_errmsg(_db));
    }
    
    template <typename... Ts>
    void bind(sqlite3_stmt* statement, const Ts&... ts) {
        assert(sqlite3_bind_parameter_count(statement) == sizeof...(Ts));
        int i = 1;
        hana::for_each(hana::make_tuple(ts...), [&](auto t) {
            using U = decltype(t);
            int res = 0;
            if constexpr(std::is_integral<U>::value) {
                res = sqlite3_bind_int64(statement, i, t);
            } else if constexpr(std::is_same<U, std::string>::value) {
                res = sqlite3_bind_text(statement, i, t.c_str(), -1, SQLITE_TRANSIENT);
            } else if constexpr(std::is_same<U, std::vector<uint8_t>>::value) {
                res = sqlite3_bind_blob(statement, i, t.data(), t.size(), SQLITE_TRANSIENT);
            } else {
                static_assert(std::is_integral<U>::value && false, "unknown type");
            }
            if (res)
                throwSqlException("bind");
            i++;
        });
    }
    
    friend class Statement;
    
public:
    SQLiteDB(std::string path, std::string sqlScheme) {
        int rc;
        rc = sqlite3_open(path.c_str(), &_db);
        if (rc) {
            sqlite3_close(_db);
            throw DatabaseOpeningException();
        }
        sqlite3_busy_timeout(_db, 1000);
        rc = sqlite3_exec(_db, sqlScheme.c_str(), NULL, NULL, NULL);
        if (rc) {
            throwSqlException("open");
        }
    }
    ~SQLiteDB() {
        if (_db) {
            sqlite3_free(_db);
        }
    }
    template <typename... Ts>
    uint64_t Insert(Statement& statement, Ts&&... ts) {
        bind(statement._handle, ts...);
        auto rc = sqlite3_step(statement._handle);
        if (rc != SQLITE_DONE) {
            throwSqlException("insert");
        }
        sqlite3_reset(statement._handle);
        return sqlite3_last_insert_rowid(_db);
    }
    template <typename... Ts>
    uint64_t Insert(std::string_view sql, Ts&&... ts) {
        Statement statement(sql, *this);
        return Insert(statement, std::forward<Ts>(ts)...);
    }
    std::string SQLiteTypeToString(int type) {
        switch(type) {
        case SQLITE_INTEGER:
            return "INT";
        case SQLITE_FLOAT:
            return "FLOAT";
        case SQLITE_TEXT:
            return "TEXT";
        case SQLITE_BLOB:
            return "BLOB";
        case SQLITE_NULL:
            return "NULL";
        default:
            assert(false);
        }
        throw std::runtime_error("unsupported type");
    }
    template <typename FSeq, typename... Ts>
    std::vector<FSeq> Select(Statement& statement, Ts&&... ts) {
        bind(statement._handle, ts...);
        auto types = collectTypes<FSeq>();

        std::vector<FSeq> res;
        int idxRow = 0;
        int rc;
        for (;;) {
            rc = sqlite3_step(statement._handle);
            if (rc != SQLITE_ROW) {
                if (rc != SQLITE_DONE)
                    throwSqlException("select");
                break;
            }
            unsigned colCount = sqlite3_column_count(statement._handle);
            assert(colCount == hana::size(FSeq()).value && "Column count mismatch");

            for (auto idxCol = 0u; idxCol < colCount; ++idxCol) {
                sqlite3_value* colValue = sqlite3_column_value(statement._handle, idxCol); // unprotected!!!
                int colType = sqlite3_value_type(colValue);                
                if (SQLiteTypeToString(colType) != types[idxCol] &&
                    SQLiteTypeToString(colType) != "NULL")
                    throw std::runtime_error("Type mismatch");
            }

            FSeq row;
            set(row, statement._handle);
            res.push_back(row);
            idxRow++;
        }
        assert(rc == SQLITE_DONE);
        sqlite3_reset(statement._handle);
        return res;
    }
    template <typename FSeq, typename... Ts>
    std::vector<FSeq> Select(std::string_view sql, Ts&&... ts) {
        Statement statement(sql, *this);
        return Select<FSeq, Ts...>(statement, std::forward<Ts>(ts)...);
    }
};

inline Statement::Statement(Statement&& s) {
    _handle = s._handle;
    s._handle = nullptr;
}

inline Statement::Statement(std::string_view sql, SQLiteDB& db) {
    auto rc = sqlite3_prepare_v2(db._db, begin(sql), -1, &_handle, NULL);
    if (rc)
        db.throwSqlException("prepare statement");
}

inline Statement::~Statement() {
    if (!_handle)
        return;
    sqlite3_finalize(_handle);
    _handle = nullptr;
}

inline Statement& Statement::operator=(Statement&& s) {
    std::swap(_handle, s._handle);
    return *this;
}

}
