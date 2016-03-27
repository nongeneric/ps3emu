#include "sqlite3.h"

#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>

#include <boost/fusion/adapted/struct/define_struct.hpp>
#include <boost/fusion/include/define_struct.hpp>

#include <boost/fusion/adapted/boost_tuple.hpp>
#include <boost/fusion/include/boost_tuple.hpp>

#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/is_sequence.hpp>

#include <boost/fusion/sequence/intrinsic/value_at.hpp>
#include <boost/fusion/include/value_at.hpp>

#include <boost/mpl/bool.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/at.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace f = boost::fusion;
namespace mpl = boost::mpl;

class DatabaseOpeningException : public virtual std::exception {
public:
    const char* what() const noexcept override {
        return "Can't open the specified SQLite database file";
    }
};

class SQLParseException : public virtual std::exception {
    const char* what() const noexcept override {
        return "An error occurred during the SQL query parsing";
    }
};

class SQLException : public virtual std::exception {
    const char* what() const noexcept override {
        return "An error occurred during the query execution";
    }
};

struct TypeCollector {
    typedef std::vector<std::string> Types;
    Types* types;
    TypeCollector(Types* types) : types(types) { }
    template<typename U> void operator()(U) {
        if (std::is_same<U, int>::value ||
            std::is_same<U, unsigned int>::value)
            types->push_back("INT");
        else if (std::is_same<U, std::string>::value)
            types->push_back("TEXT");
        else if (std::is_same<U, float>::value)
            types->push_back("FLOAT");
        else if (std::is_same<U, std::vector<uint8_t>>::value)
            types->push_back("BLOB");
        else
            assert(false); // TODO: replace by a static_assert
    }
};

/**
 *  Setter
 *  FSeq - random-access fusion sequence
 *  ValueType - one of the supported types:
 *      int, uint32_t, std::string, std::vector<uint8_t>
 *  Sets Nth element of seq to Nth column of the current row, contained by stmt
 */

template <typename FSeq, int N, typename ValueType>
struct Setter {
    static inline void Set(FSeq& seq, sqlite3_stmt* stmt) {        
        static_assert(std::is_integral<ValueType>::value && false, "Not supported type");
    }
};

template <typename FSeq, int N>
struct Setter<FSeq, N, std::vector<uint8_t>> {
    static inline void Set(FSeq& seq, sqlite3_stmt* stmt) {
        const void* blob = sqlite3_column_blob(stmt, N);
        const unsigned char* typed = static_cast<const uint8_t*>(blob);
        int count = sqlite3_column_bytes(stmt, N);
        deref(f::advance< mpl::int_<N> >(f::begin(seq))) =
            std::vector<uint8_t>(typed, typed + count);
    }
};

template <typename FSeq, int N>
struct Setter<FSeq, N, int> {
    static inline void Set(FSeq& seq, sqlite3_stmt* stmt) {
        deref(f::advance< mpl::int_<N> >(f::begin(seq))) =
            sqlite3_column_int(stmt, N);
    }
};

template <typename FSeq, int N>
struct Setter<FSeq, N, unsigned int> {
    static inline void Set(FSeq& seq, sqlite3_stmt* stmt) {
        deref(f::advance< mpl::int_<N> >(f::begin(seq))) =
            static_cast<unsigned int>(sqlite3_column_int(stmt, N));
    }
};

template <typename FSeq, int N>
struct Setter<FSeq, N, std::string> {
    static inline void Set(FSeq& seq, sqlite3_stmt* stmt) {
        deref(f::advance< mpl::int_<N> >(f::begin(seq))) =
            std::string( reinterpret_cast<const char*>(sqlite3_column_text(stmt, N)) );
    }
};

/**
 *  Filler
 *  FSeq - random-access fusion sequence
 *  N - number of columns (and elements in FSeq)
 *  Fills seq with values of the current row, contained by stmt. Works only for
 *  types supported by Setter
 */

template <typename FSeq, int N>
struct Filler {
    static inline void Fill(FSeq& seq, sqlite3_stmt* stmt) {
        typedef typename f::result_of::value_at< FSeq, mpl::int_<N> >::type Type;
        Setter<FSeq, N, Type>::Set(seq, stmt);
        Filler<FSeq, N - 1>::Fill(seq, stmt);
    }
};

template <typename FSeq>
struct Filler<FSeq, -1> {
    static inline void Fill(FSeq& seq, sqlite3_stmt* stmt) { }
};

/**
 * BinderSetter
 * Binds specified value to an sql parameter
 * Supported types:
 *     int, unsigned int, std::string, std::vector<unsigned char>
 */

template <int N, typename T>
struct BinderSetter {
    static inline void Set(sqlite3_stmt* stmt, const T& t) {
        static_assert(std::is_integral<T>::value && false, "Not supported type");
    }
};

template <int N>
struct BinderSetter<N, std::vector<unsigned char>> {
    static inline void Set(sqlite3_stmt* stmt, std::vector<unsigned char> const& t) {
        sqlite3_bind_blob(stmt, N, t.data(), t.size(), SQLITE_STATIC);
    }
};

template <int N>
struct BinderSetter<N, int> {
    static inline void Set(sqlite3_stmt* stmt, int t) {
        sqlite3_bind_int(stmt, N, t);        
    }
};

template <int N>
struct BinderSetter<N, unsigned int> {
    static inline void Set(sqlite3_stmt* stmt, unsigned int t) {
        sqlite3_bind_int64(stmt, N, static_cast<int>(t));
    }
};

template <int N>
struct BinderSetter<N, std::string> {
    static inline void Set(sqlite3_stmt* stmt, const std::string& t) {
        sqlite3_bind_text(stmt, N, t.c_str(), t.size(), NULL);
    }
};

class Null { };

/**
 * Binder
 * ts - a list of values of types Ts to bind
 * Binds ts to sql parameters of stmt
 */

template <int N, typename T, typename... Ts>
struct BinderImpl {
    static inline void Bind(sqlite3_stmt* stmt, const T& t, const Ts&... ts) {
        BinderSetter<N, T>::Set(stmt, t);
        BinderImpl<N + 1, Ts...>::Bind(stmt, ts...);
    }
};

template <int N>
struct BinderImpl<N, Null> {
    static inline void Bind(sqlite3_stmt* stmt, Null) { }
};

template <typename... Ts>
struct Binder {
    static inline void Bind(sqlite3_stmt* stmt, const Ts&... ts) {
        assert(sqlite3_bind_parameter_count(stmt) == sizeof...(Ts)
               && "Parameter count mismatch");
        BinderImpl<1, Ts..., Null>::Bind(stmt, ts..., Null());
    }
};

class SQLiteDB {
    sqlite3 *_db;    
public:
    SQLiteDB(std::string path, std::string sqlScheme) {
        bool createTables = !boost::filesystem::exists(path);
        int rc;
        rc = sqlite3_open(path.c_str(), &_db);
        if (rc) {
            sqlite3_close(_db);
            throw DatabaseOpeningException();
        }
        if (createTables) {
            rc = sqlite3_exec(_db, sqlScheme.c_str(), NULL, NULL, NULL);
            if (rc) {
                throw SQLException();
            }
        }
    }
    ~SQLiteDB() {
        sqlite3_free(_db);
    }
    template <typename... Ts>
    int Insert(std::string sql, const Ts&... ts) {
        sqlite3_stmt *statement;
        int rc;
        rc = sqlite3_prepare_v2(_db, sql.c_str(), sql.size(), &statement, NULL);
        if (rc) {
            throw SQLParseException();
        }
        Binder<Ts...>::Bind(statement, ts...);
        rc = sqlite3_step(statement);
        if (rc != SQLITE_DONE) {
            throw SQLException();
        }
        rc = sqlite3_finalize(statement);
        assert(rc == SQLITE_OK);
        return (int)sqlite3_last_insert_rowid(_db); // use uint64 all the way?
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
    std::vector<FSeq> Select(std::string sql, Ts... ts) {
        sqlite3_stmt *statement;
        int rc;
        rc = sqlite3_prepare_v2(_db, sql.c_str(), sql.size(), &statement, NULL);
        if (rc) {
            throw SQLParseException();
        }

        Binder<Ts...>::Bind(statement, ts...);

        TypeCollector::Types types;
        mpl::for_each<FSeq>(TypeCollector(&types));

        rc = sqlite3_step(statement);
        std::vector<FSeq> res;
        int idxRow = 0;
        const int staticSize = mpl::size<FSeq>::value;
        while (rc == SQLITE_ROW) {
            int colCount = sqlite3_column_count(statement);
            assert(colCount == staticSize && "Column count mismatch");

            for (int idxCol = 0; idxCol < colCount; ++idxCol) {
                sqlite3_value* colValue = sqlite3_column_value(statement, idxCol); // unprotected!!!
                int colType = sqlite3_value_type(colValue);                
                if (SQLiteTypeToString(colType) != types[idxCol] &&
                    SQLiteTypeToString(colType) != "NULL")
                    throw std::runtime_error("Type mismatch");
            }

            FSeq curSeq;
            Filler<FSeq, staticSize - 1>::Fill(curSeq, statement);
            res.push_back(curSeq);
            rc = sqlite3_step(statement);            
            idxRow++;
        }
        assert(rc == SQLITE_DONE);

        if (rc != SQLITE_DONE) {
            throw SQLException();
        }
        rc = sqlite3_finalize(statement);
        assert(rc == SQLITE_OK);

        return res;
    }
};
