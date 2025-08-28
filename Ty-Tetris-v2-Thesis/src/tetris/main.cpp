#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Dear ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <array>
#include <vector>
#include <random>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cmath>
#include <algorithm>

// PNG loader
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"


static constexpr int BOARD_W = 10;
static constexpr int BOARD_H = 20;
static constexpr int LINES_PER_LEVEL = 10;

static constexpr double BASE_GRAVITY = 1.0;   // seconds/row @ level 1
static constexpr double FAST_GRAVITY = 0.05;  // soft drop

struct RGB { float r, g, b; };
static const RGB COL_BG{ 0.05f,0.05f,0.07f };
static const RGB COL_GRID{ 0.12f,0.12f,0.16f };

static void fatal(const char* msg) {
    std::fprintf(stderr, "[FATAL] %s\n", msg);
    std::exit(EXIT_FAILURE);
}

// Minimal shader: a unit quad scaled/offset in NDC
static const char* VS_SRC =
"#version 330 core\n"
"layout(location = 0) in vec2 aPos;\n"
"uniform vec2 uScale;\n"
"uniform vec2 uOffset;\n"
"void main(){ gl_Position = vec4(aPos * uScale + uOffset, 0.0, 1.0); }\n";
static const char* FS_SRC =
"#version 330 core\n"
"out vec4 FragColor; uniform vec3 uColor;\n"
"void main(){ FragColor = vec4(uColor, 1.0); }\n";

static GLuint makeShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; GLsizei n = 0; glGetShaderInfoLog(s, 2048, &n, log);
        std::fprintf(stderr, "Shader error:\n%.*s\n", n, log); fatal("compile");
    }
    return s;
}
static GLuint makeProgram(const char* vs, const char* fs) {
    GLuint v = makeShader(GL_VERTEX_SHADER, vs), f = makeShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram(); glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048]; GLsizei n = 0; glGetProgramInfoLog(p, 2048, &n, log);
        std::fprintf(stderr, "Link error:\n%.*s\n", n, log); fatal("link");
    }
    glDeleteShader(v); glDeleteShader(f); return p;
}

// Colors for pieces
static const std::array<std::array<float, 3>, 8> COLORS = { {
    {0.08f,0.08f,0.08f}, // 0
    {0.0f, 0.9f, 0.9f},  // 1 I
    {0.9f, 0.9f, 0.0f},  // 2 O
    {0.6f, 0.0f, 0.9f},  // 3 T
    {0.0f, 0.9f, 0.2f},  // 4 S
    {0.9f, 0.0f, 0.2f},  // 5 Z
    {0.0f, 0.2f, 0.9f},  // 6 J
    {0.9f, 0.5f, 0.0f}   // 7 L
} };

// ---------- Tetromino data as C arrays (MSVC-friendly) ----------
struct Cell { int x, y; };
struct Piece { Cell rot[4][4]; int colorIndex; };

static const Piece PIECES[7] = {
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

struct Active { int x, y, r, type; };

struct Bag7 {
    std::vector<int> bag;
    std::vector<int> queue; // preview
    std::mt19937 rng{ std::random_device{}() };
    int next() { if (bag.empty()) { bag = { 0,1,2,3,4,5,6 }; std::shuffle(bag.begin(), bag.end(), rng); } int t = bag.back(); bag.pop_back(); return t; }
    void refill(size_t want) { while (queue.size() < want) queue.push_back(next()); }
    int pull() { if (queue.empty()) refill(5); int t = queue.front(); queue.erase(queue.begin()); return t; }
};

enum class Scene { Start, Controls, Settings, LevelSelect, Playing, GameOver };

struct Game {
    int board[BOARD_H][BOARD_W] = {};
    Active cur{ 4,18,0,0 };
    Bag7 bag{};
    bool paused = false, gameOver = false;
    int score = 0, lines = 0, level = 1;
    int levelIndex = 0; // 0 easy,1 med,2 soviet
    bool musicOn = true;

    Scene scene = Scene::Start;
    int menuIndex = 0;
};

struct Input {
    std::array<int, 512> prev{};
    bool pressed(GLFWwindow* w, int key) { int s = glfwGetKey(w, key); bool p = (!prev[key] && s == GLFW_PRESS); prev[key] = s; return p; }
    bool down(GLFWwindow* w, int key) { return glfwGetKey(w, key) == GLFW_PRESS; }
};

struct Renderer {
    GLuint prog = 0, vao = 0, vbo = 0;
    GLint  locScale = -1, locOffset = -1, locColor = -1;
    float cellW = 0.0f, cellH = 0.0f;
    float left = 0.0f, bottom = -1.0f;
    float boardRight = 0.0f, boardTop = 0.0f;

    void init() {
        prog = makeProgram(VS_SRC, FS_SRC);
        locScale = glGetUniformLocation(prog, "uScale");
        locOffset = glGetUniformLocation(prog, "uOffset");
        locColor = glGetUniformLocation(prog, "uColor");
        glGenVertexArrays(1, &vao); glBindVertexArray(vao);
        glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
        const float verts[12] = { -0.5f,-0.5f, 0.5f,-0.5f, 0.5f,0.5f,
                                 -0.5f,-0.5f, 0.5f,0.5f, -0.5f,0.5f };
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    void computeScale(int fbw, int fbh) {
        cellH = 2.0f / BOARD_H;                    // 20 rows fill -1..+1 Y
        float aspect = (float)fbh / (float)fbw;    // NDC X compressed by aspect
        cellW = cellH * aspect;
        left = -(BOARD_W * cellW) * 0.5f;       // center horizontally
        bottom = -1.0f;
        boardRight = left + BOARD_W * cellW;
        boardTop = bottom + BOARD_H * cellH;
    }

    void quad(float cx, float cy, float sx, float sy, RGB col) const {
        glUseProgram(prog);
        glUniform2f(locScale, sx, sy);
        glUniform2f(locOffset, cx, cy);
        glUniform3f(locColor, col.r, col.g, col.b);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }
    void drawCell(int x, int y, int colorIdx) const {
        if (colorIdx <= 0 || colorIdx >= (int)COLORS.size()) return;
        float cx = left + (x + 0.5f) * cellW;
        float cy = bottom + (y + 0.5f) * cellH;
        const auto& c = COLORS[colorIdx];
        quad(cx, cy, cellW, cellH, { c[0],c[1],c[2] });
    }
    void drawGrid() const {
        glUseProgram(prog);
        glBindVertexArray(vao);
        glUniform2f(locScale, cellW, cellH);
        glUniform3f(locColor, COL_GRID.r, COL_GRID.g, COL_GRID.b);
        for (int y = 0; y < BOARD_H; ++y) for (int x = 0; x < BOARD_W; ++x) {
            float cx = left + (x + 0.5f) * cellW;
            float cy = bottom + (y + 0.5f) * cellH;
            glUniform2f(locOffset, cx, cy);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindVertexArray(0);
    }
    void drawBoard(const Game& g) const {
        for (int y = 0; y < BOARD_H; ++y) for (int x = 0; x < BOARD_W; ++x)
            if (g.board[y][x]) drawCell(x, y, g.board[y][x]);
    }
    void drawActive(const Game& g) const {
        const Cell* pc = PIECES[g.cur.type].rot[g.cur.r];
        int color = PIECES[g.cur.type].colorIndex;
        for (int i = 0; i < 4; ++i) {
            const Cell& c = pc[i];
            int X = g.cur.x + c.x, Y = g.cur.y + c.y;
            if (Y >= 0 && X >= 0 && X < BOARD_W) drawCell(X, Y, color);
        }
    }
};

// --------- Core game ops ----------
static bool collides(const Game& g, const Active& a) {
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
static void lockPiece(Game& g) {
    const Cell* pc = PIECES[g.cur.type].rot[g.cur.r];
    int color = PIECES[g.cur.type].colorIndex;
    for (int i = 0; i < 4; ++i) {
        const Cell& c = pc[i];
        int X = g.cur.x + c.x, Y = g.cur.y + c.y;
        if (Y >= 0 && Y < BOARD_H && X >= 0 && X < BOARD_W) g.board[Y][X] = color;
    }
}
static int clearLines(Game& g) {
    int cleared = 0;
    for (int y = 0; y < BOARD_H; ++y) {
        bool full = true; for (int x = 0; x < BOARD_W; ++x) { if (!g.board[y][x]) { full = false; break; } }
        if (full) {
            ++cleared;
            for (int yy = y; yy < BOARD_H - 1; ++yy) for (int x = 0; x < BOARD_W; ++x) g.board[yy][x] = g.board[yy + 1][x];
            for (int x = 0; x < BOARD_W; ++x) g.board[BOARD_H - 1][x] = 0; --y;
        }
    }
    if (cleared) {
        static const int T[5] = { 0,100,300,500,800 };
        g.score += T[cleared] * g.level;
        g.lines += cleared;
        int nl = (g.lines / LINES_PER_LEVEL) + 1;
        if (nl > g.level) g.level = nl;
    }
    return cleared;
}
static void spawn(Game& g) {
    g.cur.type = g.bag.pull();
    g.cur.r = 0; g.cur.x = 4; g.cur.y = 18;
    if (collides(g, g.cur)) g.gameOver = true;
}
static bool tryMove(Game& g, int dx, int dy) { Active t = g.cur; t.x += dx; t.y += dy; if (!collides(g, t)) { g.cur = t; return true; } return false; }
static void rotate(Game& g, int dir) {
    Active t = g.cur; t.r = (t.r + (dir > 0 ? 1 : 3)) & 3;
    static const Cell K[6] = { {0,0},{-1,0},{1,0},{0,-1},{-2,0},{2,0} };
    for (int i = 0; i < 6; ++i) { Active c = t; c.x += K[i].x; c.y += K[i].y; if (!collides(g, c)) { g.cur = c; return; } }
}
static void hardDrop(Game& g) { while (tryMove(g, 0, -1)) {} }

// --------- Music ----------
static void musicPlay(bool on) {
#ifdef _WIN32
    if (on) PlaySound(TEXT("resources/music/theme.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    else    PlaySound(NULL, 0, 0);
#else
    (void)on;
#endif
}
// Simple RGBA texture loader (OpenGL)
struct Texture2D {
    GLuint id = 0;
    int w = 0, h = 0;
};

static Texture2D loadTextureRGBA(const char* path) {
    Texture2D t{};
    int n = 0;
    stbi_uc* data = stbi_load(path, &t.w, &t.h, &n, 4);
    if (!data) {
        std::fprintf(stderr, "[warn] failed to load image: %s\n", path);
        return t;
    }
    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return t;
}

// ImGui style: retro / “tetris-y”
static void ApplyRetroTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 12.0f;
    s.FrameRounding = 10.0f;
    s.GrabRounding = 10.0f;
    s.ScrollbarRounding = 12.0f;
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize = 0.0f;

    ImVec4 bg = ImVec4(0.05f, 0.05f, 0.07f, 1.00f);
    ImVec4 grid = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
    ImVec4 text = ImVec4(0.95f, 0.95f, 0.98f, 1.00f);
    ImVec4 accent = ImVec4(0.20f, 0.80f, 1.00f, 1.00f);
    ImVec4 btn = ImVec4(0.12f, 0.16f, 0.22f, 1.00f);
    ImVec4 btnH = ImVec4(0.20f, 0.35f, 0.55f, 1.00f);

    auto& c = s.Colors;
    c[ImGuiCol_Text] = text;
    c[ImGuiCol_WindowBg] = ImVec4(bg.x, bg.y, bg.z, 0.98f);
    c[ImGuiCol_PopupBg] = ImVec4(bg.x, bg.y, bg.z, 0.98f);
    c[ImGuiCol_Border] = grid;
    c[ImGuiCol_FrameBg] = btn;
    c[ImGuiCol_FrameBgHovered] = btnH;
    c[ImGuiCol_Button] = btn;
    c[ImGuiCol_ButtonHovered] = btnH;
    c[ImGuiCol_ButtonActive] = accent;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = accent;
    c[ImGuiCol_Header] = btn;
    c[ImGuiCol_HeaderHovered] = btnH;
    c[ImGuiCol_HeaderActive] = accent;
}

// Load a retro pixel font if available, else keep default
static void LoadRetroFont() {
    ImGuiIO& io = ImGui::GetIO();
    const char* path = "resources/fonts/PressStart2P.ttf";
    // use a slightly larger size; ImGui auto DPI scales
    if (FILE* f = std::fopen(path, "rb")) {
        std::fclose(f);
        io.Fonts->AddFontFromFileTTF(path, 18.0f);
        // after adding fonts, the backends will build them in NewFrame()
    }
}

int main() {
    if (!glfwInit()) fatal("glfwInit failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(900, 900, "Tetris", nullptr, nullptr);
    if (!win) { glfwTerminate(); fatal("window failed"); }
    // --- Set window/taskbar icon ---
    GLFWimage images[1];
    int iconW, iconH, channels;
    unsigned char* pixels = stbi_load("resources/ui/icon.png", &iconW, &iconH, &channels, 4);
    if (pixels) {
        images[0].width = iconW;
        images[0].height = iconH;
        images[0].pixels = pixels;
        glfwSetWindowIcon(win, 1, images);
        stbi_image_free(pixels);
    }
    else {
        std::fprintf(stderr, "[warn] could not load icon.png\n");
    }
    glfwMakeContextCurrent(win); glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) fatal("glad failed");

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ApplyRetroTheme();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    LoadRetroFont();

    // load your PNG logo
    Texture2D logo = loadTextureRGBA("resources/ui/logo.png");


    Renderer r; r.init();
    Game g; g.bag.refill(5);
    Input in;
    auto last = std::chrono::high_resolution_clock::now();
    double acc = 0.0;

    auto resetGame = [&]() {
        bool keepMusic = g.musicOn;       // preserve setting
        g = Game{};                       // reset everything else
        g.musicOn = keepMusic;            // restore
        g.bag.refill(5);
        musicPlay(false);                 // ensure stopped at reset
        };

    auto startWithLevel = [&](int idx) {
        bool keepMusic = g.musicOn;       // preserve setting
        g = Game{};
        g.musicOn = keepMusic;         // restore
        g.levelIndex = idx;
        g.level = 1 + (idx == 0 ? 0 : (idx == 1 ? 4 : 9));
        g.scene = Scene::Playing;
        g.bag.refill(5);
        spawn(g);
        acc = 0.0;
        musicPlay(g.musicOn);             // start or stay silent
        };


    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        // keep cell scale square to framebuffer
        int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
        r.computeScale(fbw, fbh);

        // timing
        auto now = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration<double>(now - last).count();
        last = now;

        // Begin ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Clear GL
        glClearColor(COL_BG.r, COL_BG.g, COL_BG.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // ------------- UI SCENES (Dear ImGui) -------------
        if (g.scene == Scene::Start) {
            ImGui::SetNextWindowSize(ImVec2(520, 440), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2((fbw - 520) / 2.0f, (fbh - 440) / 2.0f), ImGuiCond_Always);
            ImGui::Begin("Tetris - Start", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

            // --- LOGO at the top ---
            if (logo.id != 0) {
                float maxW = 460.0f;            // clamp inside the window with margins
                float scale = (float)logo.w;
                if (scale > maxW) scale = maxW;
                float ratio = scale / (float)logo.w;
                ImVec2 imgSize(scale, ratio * (float)logo.h);

                // center horizontally
                float avail = ImGui::GetContentRegionAvail().x;
                float padX = (avail - imgSize.x) * 0.5f;
                if (padX > 0) ImGui::Dummy(ImVec2(padX, 0));
                ImGui::SameLine();
                ImGui::Image((ImTextureID)(intptr_t)logo.id, imgSize);
                ImGui::NewLine();
            }
            else {
                ImGui::TextWrapped("TETRIS");
                ImGui::Separator();
            }

            // --- MENU buttons ---
            if (ImGui::Button("Start Game", ImVec2(-1, 0))) g.scene = Scene::LevelSelect;
            if (ImGui::Button("Controls", ImVec2(-1, 0))) g.scene = Scene::Controls;
            if (ImGui::Button("Settings", ImVec2(-1, 0))) g.scene = Scene::Settings;
            if (ImGui::Button("Quit", ImVec2(-1, 0))) glfwSetWindowShouldClose(win, 1);

            ImGui::End();
        }

        else if (g.scene == Scene::Controls) {
            ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2((fbw - 520) / 2.0f, (fbh - 360) / 2.0f), ImGuiCond_Always);
            ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::TextUnformatted(
                "Left/Right : Move\n"
                "Down       : Soft Drop\n"
                "Up / X     : Rotate CW\n"
                "Z          : Rotate CCW\n"
                "Space      : Hard Drop\n"
                "P          : Pause\n"
                "R          : Restart to Start\n"
                "Esc        : Quit");
            if (ImGui::Button("Back")) g.scene = Scene::Start;
            ImGui::End();
        }
        else if (g.scene == Scene::Settings) {
            ImGui::SetNextWindowSize(ImVec2(420, 180), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2((fbw - 420) / 2.0f, (fbh - 180) / 2.0f), ImGuiCond_Always);
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            if (ImGui::Checkbox("Music", &g.musicOn)) {
                if (!g.musicOn) {
                    musicPlay(false);                         // stop right now
                }
                else {
                    if (g.scene == Scene::Playing)            // only auto-start if already in game
                        musicPlay(true);
                    // If not playing yet, startWithLevel() will honor the setting later
                }
            }
            if (ImGui::Button("Back")) g.scene = Scene::Start;
            ImGui::End();
        }
        else if (g.scene == Scene::LevelSelect) {
            ImGui::SetNextWindowSize(ImVec2(420, 220), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2((fbw - 420) / 2.0f, (fbh - 220) / 2.0f), ImGuiCond_Always);
            ImGui::Begin("Select Level", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            const char* levels[] = { "Easy","Medium","Soviet" };
            ImGui::RadioButton("Easy", &g.levelIndex, 0); ImGui::SameLine();
            ImGui::RadioButton("Medium", &g.levelIndex, 1); ImGui::SameLine();
            ImGui::RadioButton("Soviet", &g.levelIndex, 2);
            if (ImGui::Button("Start", ImVec2(-1, 0))) startWithLevel(g.levelIndex);
            if (ImGui::Button("Back", ImVec2(-1, 0))) g.scene = Scene::Start;
            ImGui::End();
        }

        // ------------- GAME LOOP + HUD -------------
        if (g.scene == Scene::Playing) {
            // input (escape handled by OS / VS run)
            if (in.pressed(win, GLFW_KEY_P)) g.paused = !g.paused;
            if (in.pressed(win, GLFW_KEY_R)) { musicPlay(false); g.scene = Scene::Start; }

            if (!g.paused && !g.gameOver) {
                if (in.pressed(win, GLFW_KEY_LEFT))  tryMove(g, -1, 0);
                if (in.pressed(win, GLFW_KEY_RIGHT)) tryMove(g, +1, 0);
                if (in.pressed(win, GLFW_KEY_UP) || in.pressed(win, GLFW_KEY_X)) rotate(g, +1);
                if (in.pressed(win, GLFW_KEY_Z)) rotate(g, -1);
                if (in.pressed(win, GLFW_KEY_SPACE)) { hardDrop(g); lockPiece(g); clearLines(g); spawn(g); }
            }

            double base = BASE_GRAVITY / std::pow(1.25, g.level - 1);
            if (g.levelIndex == 1) base *= 0.6; else if (g.levelIndex == 2) base *= 0.35;
            double speed = in.down(win, GLFW_KEY_DOWN) ? FAST_GRAVITY : base;

            if (!g.paused && !g.gameOver) {
                acc += dt;
                while (acc >= speed) {
                    if (!tryMove(g, 0, -1)) { lockPiece(g); clearLines(g); spawn(g); }
                    acc -= speed;
                }
            }

            // draw board
            r.drawGrid();
            r.drawBoard(g);
            if (!g.gameOver) r.drawActive(g);

            if (g.gameOver) { musicPlay(false); g.scene = Scene::GameOver; }

            if (g.paused) {
                ImGui::SetNextWindowBgAlpha(0.7f); // translucent
                ImGui::SetNextWindowSize(ImVec2(240, 100));
                ImGui::SetNextWindowPos(ImVec2((fbw - 240) / 2.0f, (fbh - 100) / 2.0f));
                ImGui::Begin("Paused", nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::Text("== PAUSED ==");
                ImGui::Text("Press P to resume");
                ImGui::End();
            }

        }

        if (g.scene == Scene::Playing) {
            // HUD panel to the right of the board (imgui handles layout)
            ImGui::SetNextWindowSize(ImVec2(280, 300), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2((fbw * (r.boardRight * 0.5f + 0.5f)) + 16.0f, 20.0f), ImGuiCond_Always);
            ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Text("Score: %d", g.score);
            ImGui::Separator();
            ImGui::Text("Next:");
            // draw next preview as small colored squares in the ImGui window
            if (!g.bag.queue.empty()) {
                int t = g.bag.queue[0];
                const Cell* pc = PIECES[t].rot[0];
                const auto& c = COLORS[PIECES[t].colorIndex];
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 p0 = ImGui::GetCursorScreenPos();
                float size = 16.0f;
                for (int i = 0; i < 4; ++i) {
                    ImVec2 a = ImVec2(p0.x + (pc[i].x + 2) * size, p0.y + (pc[i].y + 2) * size);
                    ImVec2 b = ImVec2(a.x + size - 1, a.y + size - 1);
                    dl->AddRectFilled(a, b, IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), 255), 2.0f);
                }
                ImGui::Dummy(ImVec2(5 * size, 5 * size));
            }
            ImGui::End();
        }
        else if (g.scene == Scene::GameOver) {
            ImGui::SetNextWindowSize(ImVec2(460, 220), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2((fbw - 460) / 2.0f, (fbh - 220) / 2.0f), ImGuiCond_Always);
            ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Text("FINAL SCORE: %d", g.score);
            if (ImGui::Button("Reset to Start", ImVec2(-1, 0))) { resetGame(); }
            if (ImGui::Button("Quit", ImVec2(-1, 0))) glfwSetWindowShouldClose(win, 1);
            ImGui::End();
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(win);
    }

    musicPlay(false);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
