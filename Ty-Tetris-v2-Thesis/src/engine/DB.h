#pragma once
#include <string>
#include <vector>

struct sqlite3;

namespace eng {

    struct ScoreRow {
        std::string name;
        int score = 0;
        int level = 1;
        std::string when; // ISO8601 timestamp
    };

    class DB {
    public:
        DB() = default;
        ~DB();

        bool Open(const char* path = "tetris.db");
        void Close();

        bool EnsureSchema();
        bool InsertScore(const std::string& name, int score, int level);
        std::vector<ScoreRow> Top(int limit = 10);

    private:
        sqlite3* m_DB = nullptr;
    };

} // namespace eng
