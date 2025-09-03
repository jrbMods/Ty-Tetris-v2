#pragma once
#include <array>
#include <vector>
#include <random>
#include <string>

namespace eng { class Renderer; }

namespace game {

    static constexpr int BOARD_W = 10;
    static constexpr int BOARD_H = 20;
    static constexpr int LINES_PER_LEVEL = 10;

    struct Cell { int x, y; };
    struct Piece { Cell rot[4][4]; int colorIndex; };

    extern const Piece PIECES[7];
    extern const std::array<std::array<float, 3>, 8> COLORS;

    struct Active { int x, y, r, type; };

    struct Bag7 {
        std::vector<int> bag;
        std::vector<int> queue;
        std::mt19937 rng{ std::random_device{}() };
        int next();
        void refill(size_t want);
        int pull();
    };

    enum class Scene { Start, Controls, Settings, LevelSelect, Playing, GameOver, HighScores };

    struct Game {
        int board[BOARD_H][BOARD_W] = {};
        Active cur{ 4,18,0,0 };
        Bag7 bag{};
        bool paused = false, gameOver = false;
        int score = 0, lines = 0, level = 1;
        int levelIndex = 0;
        bool musicOn = true;

        Scene scene = Scene::Start;
        int menuIndex = 0;

        // Obstructions
        int garbageTimerMs = 0;

        // Player name for DB
        std::string playerName = "Player";
    };

    bool collides(const Game& g, const Active& a);
    void lockPiece(Game& g);
    int  clearLines(Game& g);
    void spawn(Game& g);
    bool tryMove(Game& g, int dx, int dy);
    void rotate(Game& g, int dir);
    void hardDrop(Game& g);

    // Rendering helpers
    void DrawGrid(eng::Renderer& r);
    void DrawBoard(eng::Renderer& r, const Game& g);
    void DrawActive(eng::Renderer& r, const Game& g);
    void DrawPiecePreview(eng::Renderer& r, int type, float cx, float cy, float scale);

    // Obstructions
    void SeedObstructions(Game& g, int levelIndex);
    void MaybeAddGarbage(Game& g, int deltaMs);

} // namespace game
