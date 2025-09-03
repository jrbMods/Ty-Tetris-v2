#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "../engine/Renderer.h"
#include "../engine/Texture.h"
#include "../engine/Audio.h"
#include "../engine/DB.h"
#include "../game/Tetris.h"
#include "../game/UI.h"

#include "stb_image.h" // declarations only (implementation in Texture.cpp)

#include <chrono>
#include <cmath>

static void ApplyRetroTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 12.0f; s.FrameRounding = 10.0f; s.GrabRounding = 10.0f;
    s.ScrollbarRounding = 12.0f; s.WindowBorderSize = 1.0f; s.FrameBorderSize = 0.0f;
    auto& c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.07f, 0.98f);
    c[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.16f, 1.0f);
    c[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.98f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.12f, 0.16f, 0.22f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.35f, 0.55f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.80f, 1.00f, 1.0f);
}

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(900, 900, "Tetris", nullptr, nullptr);
    if (!win) { glfwTerminate(); return 2; }
    glfwMakeContextCurrent(win); glfwSwapInterval(1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return 3;

    // Window icon
    {
        GLFWimage images[1];
        int w, h, comp;
        unsigned char* pixels = stbi_load("resources/ui/icon.png", &w, &h, &comp, 4);
        if (pixels) { images[0].width = w; images[0].height = h; images[0].pixels = pixels; glfwSetWindowIcon(win, 1, images); stbi_image_free(pixels); }
    }

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ApplyRetroTheme();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Engine subsystems
    eng::Renderer renderer; renderer.Init();
    eng::Texture2D logo; logo.LoadRGBA("resources/ui/logo.png");
    eng::Audio audio; audio.Init();
    eng::DB    db;    db.Open("tetris.db");

    // Game state
    game::Game g; g.bag.refill(5);
    auto last = std::chrono::high_resolution_clock::now();
    double accSec = 0.0;

    auto resetToStart = [&]() {
        bool keepMusic = g.musicOn;
        g = game::Game{};
        g.musicOn = keepMusic;
        g.bag.refill(5);
        audio.StopMusic();
        g.scene = game::Scene::Start;
        };
    auto startWithLevel = [&](int idx) {
        bool keepMusic = g.musicOn;
        g = game::Game{};
        g.musicOn = keepMusic;
        g.levelIndex = idx;
        g.level = 1 + (idx == 0 ? 0 : (idx == 1 ? 4 : 9));
        g.scene = game::Scene::Playing;
        g.bag.refill(5);
        game::SeedObstructions(g, idx);
        game::spawn(g);
        accSec = 0.0;
        audio.SetMusicOn(g.musicOn);
        audio.PlayMusic("resources/music/theme.wav", true);
        };
    auto onMusicToggle = [&](bool on)->bool { audio.SetMusicOn(on); return on; };
    auto getTopScores = [&]() { return db.Top(10); };

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        // timing
        auto now = std::chrono::high_resolution_clock::now();
        int deltaMs = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - last).count();
        double dt = deltaMs / 1000.0;
        last = now;

        int fbw, fbh; glfwGetFramebufferSize(win, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
        renderer.ComputeScale(fbw, fbh, game::BOARD_W, game::BOARD_H);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        using game::Scene;
        if (g.scene == Scene::Start) {
            ui::DrawStart(win, g, fbw, fbh, &logo);
        }
        else if (g.scene == Scene::Controls) {
            ui::DrawControls(g, fbw, fbh);
        }
        else if (g.scene == Scene::Settings) {
            ui::DrawSettings(g, fbw, fbh, onMusicToggle);
        }
        else if (g.scene == Scene::LevelSelect) {
            ui::DrawLevelSelect(g, fbw, fbh, startWithLevel);
        }
        else if (g.scene == Scene::HighScores) {
            auto rows = getTopScores();
            ui::DrawHighScores(g, fbw, fbh, rows);
        }

        if (g.scene == Scene::Playing) {
            // inputs (only if ImGui isn't typing)
            if (!ImGui::GetIO().WantCaptureKeyboard) {
                if (glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS) g.paused = true;
                if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) g.paused = !g.paused;

                if (!g.paused && !g.gameOver) {
                    if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS) game::tryMove(g, -1, 0);
                    if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS) game::tryMove(g, +1, 0);
                    if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS) game::rotate(g, +1);
                    if (glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS) game::rotate(g, -1);
                    if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) { game::hardDrop(g); game::lockPiece(g); game::clearLines(g); game::spawn(g); }
                }
            }

            double base = 1.0 / std::pow(1.25, g.level - 1);
            if (g.levelIndex == 1) base *= 0.6; else if (g.levelIndex == 2) base *= 0.35;
            double speed = (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS) ? 0.05 : base;

            if (!g.paused && !g.gameOver) {
                accSec += dt;
                game::MaybeAddGarbage(g, deltaMs);
                while (accSec >= speed) {
                    if (!game::tryMove(g, 0, -1)) { game::lockPiece(g); game::clearLines(g); game::spawn(g); }
                    accSec -= speed;
                }
            }

            game::DrawGrid(renderer);
            game::DrawBoard(renderer, g);
            if (!g.gameOver) game::DrawActive(renderer, g);

            ui::DrawHUD(renderer, g, fbw, fbh);

            if (g.paused) {
                ImGui::SetNextWindowBgAlpha(0.85f);
                ImGui::SetNextWindowSize(ImVec2(280, 160), ImGuiCond_Always);
                ImGui::SetNextWindowPos(ImVec2((fbw - 280) / 2.0f, (fbh - 160) / 2.0f), ImGuiCond_Always);
                ImGui::Begin("Pause", nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
                if (ImGui::Button("Resume", ImVec2(-1, 0))) g.paused = false;
                if (ImGui::Button("Restart", ImVec2(-1, 0))) { audio.StopMusic(); g.scene = Scene::Start; }
                if (ImGui::Button("Quit", ImVec2(-1, 0))) glfwSetWindowShouldClose(win, 1);
                ImGui::End();
            }

            if (g.gameOver) {
                audio.StopMusic();
                db.InsertScore(g.playerName.empty() ? "Player" : g.playerName, g.score, g.level);
                g.scene = Scene::GameOver;
            }
        }

        if (g.scene == Scene::GameOver) {
            ui::DrawGameOver(win, g, fbw, fbh, resetToStart);
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    audio.Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
