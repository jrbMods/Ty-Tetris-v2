#include "Tetris.h"
#include "../engine/Renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace game {

    const std::array<std::array<float, 3>, 8> COLORS = { {
        {0.08f,0.08f,0.08f},
        {0.0f, 0.9f, 0.9f},
        {0.9f, 0.9f, 0.0f},
        {0.6f, 0.0f, 0.9f},
        {0.0f, 0.9f, 0.2f},
        {0.9f, 0.0f, 0.2f},
        {0.0f, 0.2f, 0.9f},
        {0.9f, 0.5f, 0.0f}
    } };

    const Piece PIECES[7] = {
        { { { {-1,0},{0,0},{1,0},{2,0} }, { {1,-1},{1,0},{1,1},{1,2} },
            { {-1,1},{0,1},{1,1},{2,1} }, { {0,-1},{0,0},{0,1},{0,2} } }, 1 },
        { { { {0,0},{1,0},{0,1},{1,1} }, { {0,0},{1,0},{0,1},{1,1} },
            { {0,0},{1,0},{0,1},{1,1} }, { {0,0},{1,0},{0,1},{1,1} } }, 2 },
        { { { {-1,0},{0,0},{1,0},{0,1} }, { {0,-1},{0,0},{0,1},{1,0} },
            { {-1,0},{0,0},{1,0},{0,-1} },{ {0,-1},{0,0},{0,1},{-1,0} } }, 3 },
        { { { {-1,0},{0,0},{0,1},{1,1} }, { {0,-1},{0,0},{1,0},{1,1} },
            { {-1,-1},{0,-1},{0,0},{1,0} },{ {-1,-1},{-1,0},{0,0},{0,1} } }, 4 },
        { { { {-1,1},{0,1},{0,0},{1,0} }, { {1,-1},{1,0},{0,0},{0,1} },
            { {-1,0},{0,0},{0,-1},{1,-1} },{ {0,-1},{0,0},{-1,0},{-1,1} } }, 5 },
        { { { {-1,0},{0,0},{1,0},{-1,1} },{ {0,-1},{0,0},{0,1},{1,-1} },
            { {-1,0},{0,0},{1,0},{1,-1} }, { {0,-1},{0,0},{0,1},{-1,1} } }, 6 },
        { { { {-1,0},{0,0},{1,0},{1,1} }, { {0,-1},{0,0},{0,1},{1,1} },
            { {-1,-1},{-1,0},{0,0},{1,0} },{ {-1,-1},{0,-1},{0,0},{0,1} } }, 7 }
    };

    int Bag7::next() {
        if (bag.empty()) {
            bag = { 0,1,2,3,4,5,6 };
            std::shuffle(bag.begin(), bag.end(), rng);
        }
        int t = bag.back(); bag.pop_back(); return t;
    }
    void Bag7::refill(size_t want) { while (queue.size() < want) queue.push_back(next()); }
    int Bag7::pull() { if (queue.empty()) refill(5); int t = queue.front(); queue.erase(queue.begin()); return t; }

    bool collides(const Game& g, const Active& a) {
        const Cell* pc = PIECES[a.type].rot[a.r];
        for (int i = 0; i < 4; ++i) {
            const Cell& c = pc[i];
            int X = a.x + c.x, Y = a.y + c.y;
            if (X < 0 || X >= BOARD_W || Y < 0) return true;
            if (Y >= BOARD_H) continue;
            if (g.board[Y][X] != 0) return true;
        }
        return false;
    }

    void lockPiece(Game& g) {
        const Cell* pc = PIECES[g.cur.type].rot[g.cur.r];
        int color = PIECES[g.cur.type].colorIndex;
        for (int i = 0; i < 4; ++i) {
            const Cell& c = pc[i];
            int X = g.cur.x + c.x, Y = g.cur.y + c.y;
            if (Y >= 0 && Y < BOARD_H && X >= 0 && X < BOARD_W) g.board[Y][X] = color;
        }
    }

    int clearLines(Game& g) {
        int cleared = 0;
        for (int y = 0; y < BOARD_H; ++y) {
            bool full = true; for (int x = 0; x < BOARD_W; ++x) { if (!g.board[y][x]) { full = false; break; } }
            if (full) {
                ++cleared;
                for (int yy = y; yy < BOARD_H - 1; ++yy)
                    for (int x = 0; x < BOARD_W; ++x) g.board[yy][x] = g.board[yy + 1][x];
                for (int x = 0; x < BOARD_W; ++x) g.board[BOARD_H - 1][x] = 0; --y;
            }
        }
        if (cleared) {
            static const int T[5] = { 0,100,300,500,800 };
            g.score += T[cleared] * g.level;
            g.lines += cleared;
            int nl = (g.lines / LINES_PER_LEVEL) + 1; if (nl > g.level) g.level = nl;
        }
        return cleared;
    }

    void spawn(Game& g) {
        g.cur.type = g.bag.pull();
        g.cur.r = 0; g.cur.x = 4; g.cur.y = 18;
        if (collides(g, g.cur)) g.gameOver = true;
    }

    bool tryMove(Game& g, int dx, int dy) {
        Active t = g.cur; t.x += dx; t.y += dy; if (!collides(g, t)) { g.cur = t; return true; } return false;
    }

    void rotate(Game& g, int dir) {
        Active t = g.cur; t.r = (t.r + (dir > 0 ? 1 : 3)) & 3;
        static const Cell K[6] = { {0,0},{-1,0},{1,0},{0,-1},{-2,0},{2,0} };
        for (int i = 0; i < 6; ++i) { Active c = t; c.x += K[i].x; c.y += K[i].y; if (!collides(g, c)) { g.cur = c; return; } }
    }

    void hardDrop(Game& g) { while (tryMove(g, 0, -1)) {} }

    void DrawGrid(eng::Renderer& r) {
        for (int y = 0; y < BOARD_H; ++y) for (int x = 0; x < BOARD_W; ++x) {
            float cx = r.left + (x + 0.5f) * r.cellW;
            float cy = r.bottom + (y + 0.5f) * r.cellH;
            r.Quad(cx, cy, r.cellW, r.cellH, { 0.12f,0.12f,0.16f });
        }
    }

    void DrawBoard(eng::Renderer& r, const Game& g) {
        for (int y = 0; y < BOARD_H; ++y) for (int x = 0; x < BOARD_W; ++x) {
            int col = g.board[y][x];
            if (!col) continue;
            const auto& c = COLORS[col];
            float cx = r.left + (x + 0.5f) * r.cellW;
            float cy = r.bottom + (y + 0.5f) * r.cellH;
            r.Quad(cx, cy, r.cellW, r.cellH, { c[0],c[1],c[2] });
        }
    }

    void DrawActive(eng::Renderer& r, const Game& g) {
        const Cell* pc = PIECES[g.cur.type].rot[g.cur.r];
        int color = PIECES[g.cur.type].colorIndex;
        const auto& c = COLORS[color];
        for (int i = 0; i < 4; ++i) {
            const Cell& cc = pc[i];
            int X = g.cur.x + cc.x, Y = g.cur.y + cc.y;
            if (Y >= 0 && X >= 0 && X < BOARD_W) {
                float cx = r.left + (X + 0.5f) * r.cellW;
                float cy = r.bottom + (Y + 0.5f) * r.cellH;
                r.Quad(cx, cy, r.cellW, r.cellH, { c[0],c[1],c[2] });
            }
        }
    }

    void DrawPiecePreview(eng::Renderer& r, int type, float cx, float cy, float scale) {
        int color = PIECES[type].colorIndex;
        const auto& c = COLORS[color];
        const Cell* pc = PIECES[type].rot[0];
        for (int i = 0; i < 4; ++i) {
            const Cell& cc = pc[i];
            r.Quad(cx + cc.x * scale, cy + cc.y * scale, scale, scale, { c[0],c[1],c[2] });
        }
    }

    // Obstructions
    void SeedObstructions(Game& g, int levelIndex) {
        int rows = (levelIndex == 0) ? 2 : (levelIndex == 1 ? 5 : 8);
        std::mt19937 rng{ 1337u };
        for (int y = 0; y < rows; ++y) {
            int py = y;
            for (int x = 0; x < BOARD_W; ++x) {
                float p = (levelIndex == 2) ? 0.35f : (levelIndex == 1 ? 0.22f : 0.12f);
                if ((rng() % 1000) / 1000.0f < p) {
                    g.board[py][x] = 7;
                }
            }
        }
    }

    void MaybeAddGarbage(Game& g, int deltaMs) {
        g.garbageTimerMs += deltaMs;
        int interval = (g.levelIndex == 2) ? 8000 : (g.levelIndex == 1 ? 12000 : 18000);
        if (g.garbageTimerMs < interval) return;
        g.garbageTimerMs = 0;

        int hole = std::min(BOARD_W - 1, std::max(0, (int)(std::rand() % BOARD_W)));
        for (int y = BOARD_H - 1; y > 0; --y) {
            for (int x = 0; x < BOARD_W; ++x) g.board[y][x] = g.board[y - 1][x];
        }
        for (int x = 0; x < BOARD_W; ++x) g.board[0][x] = (x == hole) ? 0 : 5;
    }

} // namespace game
