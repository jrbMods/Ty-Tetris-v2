#include "DB.h"
#include "../../vendor/sqlite/sqlite3.h"        // <<— make path explicit
#include <cstdio>

namespace eng {

    DB::~DB() { Close(); }

    bool DB::Open(const char* path) {
        Close();
        if (sqlite3_open(path, &m_DB) != SQLITE_OK) {
            std::fprintf(stderr, "[DB] open fail: %s\n", sqlite3_errmsg(m_DB));
            Close(); return false;
        }
        return EnsureSchema();
    }

    void DB::Close() { if (m_DB) { sqlite3_close(m_DB); m_DB = nullptr; } }

    bool DB::EnsureSchema() {
        const char* sql =
            "CREATE TABLE IF NOT EXISTS scores ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " name TEXT NOT NULL,"
            " score INTEGER NOT NULL,"
            " level INTEGER NOT NULL,"
            " created_at TEXT NOT NULL DEFAULT (datetime('now'))"
            ");";
        char* err = nullptr;
        if (sqlite3_exec(m_DB, sql, nullptr, nullptr, &err) != SQLITE_OK) {
            std::fprintf(stderr, "[DB] schema fail: %s\n", err ? err : "(null)"); sqlite3_free(err); return false;
        }
        return true;
    }

    bool DB::InsertScore(const std::string& name, int score, int level) {
        const char* sql = "INSERT INTO scores(name,score,level) VALUES (?,?,?);";
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(m_DB, sql, -1, &st, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(st, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(st, 2, score);
        sqlite3_bind_int(st, 3, level);
        bool ok = (sqlite3_step(st) == SQLITE_DONE);
        sqlite3_finalize(st);
        return ok;
    }

    std::vector<ScoreRow> DB::Top(int limit) {
        std::vector<ScoreRow> out;
        const char* sql = "SELECT name,score,level,created_at FROM scores ORDER BY score DESC LIMIT ?;";
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(m_DB, sql, -1, &st, nullptr) != SQLITE_OK) return out;
        sqlite3_bind_int(st, 1, limit);
        while (sqlite3_step(st) == SQLITE_ROW) {
            ScoreRow r;
            r.name = reinterpret_cast<const char*>(sqlite3_column_text(st, 0));
            r.score = sqlite3_column_int(st, 1);
            r.level = sqlite3_column_int(st, 2);
            r.when = reinterpret_cast<const char*>(sqlite3_column_text(st, 3));
            out.push_back(r);
        }
        sqlite3_finalize(st);
        return out;
    }

} // namespace eng
