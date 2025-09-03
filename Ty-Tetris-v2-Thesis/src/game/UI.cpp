#include "UI.h"
#include "../engine/Renderer.h"
#include "../engine/Texture.h"
#include "../engine/DB.h"
#include "Tetris.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cstdio>

namespace ui {

    void DrawStart(GLFWwindow* win, game::Game& g, int fbw, int fbh, eng::Texture2D* logo) {
        ImGui::SetNextWindowSize(ImVec2(520, 460), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((fbw - 520) / 2.0f, (fbh - 460) / 2.0f), ImGuiCond_Always);
        ImGui::Begin("Tetris - Start", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        if (logo && logo->id) {
            float scale = 0.5f; // tweak to make the png smaller/bigger
            ImVec2 size(logo->w * scale, logo->h * scale);
            float avail = ImGui::GetContentRegionAvail().x;
            float padX = (avail - size.x) * 0.5f; if (padX > 0) ImGui::Dummy(ImVec2(padX, 0));
            ImGui::SameLine();
            ImGui::Image((ImTextureID)(intptr_t)logo->id, size);
            ImGui::NewLine();
        }
        else {
            ImGui::TextWrapped("TETRIS"); ImGui::Separator();
        }

        if (ImGui::Button("Start Game", ImVec2(-1, 0))) g.scene = game::Scene::LevelSelect;
        if (ImGui::Button("High Scores", ImVec2(-1, 0))) g.scene = game::Scene::HighScores;
        if (ImGui::Button("Controls", ImVec2(-1, 0))) g.scene = game::Scene::Controls;
        if (ImGui::Button("Settings", ImVec2(-1, 0))) g.scene = game::Scene::Settings;
        if (ImGui::Button("Quit", ImVec2(-1, 0))) {
            if (win) glfwSetWindowShouldClose(win, 1);
        }
        ImGui::End();
    }

    void DrawControls(game::Game& g, int fbw, int fbh) {
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
            "R          : Restart\n"
            "Esc        : Pause Menu / Quit");
        if (ImGui::Button("Back")) g.scene = game::Scene::Start;
        ImGui::End();
    }

    void DrawSettings(game::Game& g, int fbw, int fbh, const std::function<bool(bool)>& onMusicToggle) {
        ImGui::SetNextWindowSize(ImVec2(420, 210), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((fbw - 420) / 2.0f, (fbh - 210) / 2.0f), ImGuiCond_Always);
        ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        static char nameBuf[32];
        static bool init = false;
        if (!init) { init = true; std::snprintf(nameBuf, sizeof(nameBuf), "%s", g.playerName.c_str()); }
        if (ImGui::InputText("Player Name", nameBuf, sizeof(nameBuf))) g.playerName = nameBuf;

        bool m = g.musicOn;
        if (ImGui::Checkbox("Music", &m)) {
            g.musicOn = m;
            if (onMusicToggle) onMusicToggle(m);
        }
        if (ImGui::Button("Back")) g.scene = game::Scene::Start;
        ImGui::End();
    }

    void DrawLevelSelect(game::Game& g, int fbw, int fbh, const std::function<void(int)>& onStart) {
        ImGui::SetNextWindowSize(ImVec2(420, 220), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((fbw - 420) / 2.0f, (fbh - 220) / 2.0f), ImGuiCond_Always);
        ImGui::Begin("Select Level", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::RadioButton("Easy", &g.levelIndex, 0); ImGui::SameLine();
        ImGui::RadioButton("Medium", &g.levelIndex, 1); ImGui::SameLine();
        ImGui::RadioButton("Soviet", &g.levelIndex, 2);
        if (ImGui::Button("Start", ImVec2(-1, 0)) && onStart) onStart(g.levelIndex);
        if (ImGui::Button("Back", ImVec2(-1, 0))) g.scene = game::Scene::Start;
        ImGui::End();
    }

    void DrawHUD(eng::Renderer& r, game::Game& g, int fbw, int fbh) {
        ImGui::SetNextWindowSize(ImVec2(280, 300), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((fbw * (r.boardRight * 0.5f + 0.5f)) + 16.0f, 20.0f), ImGuiCond_Always);
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoInputs);
        ImGui::Text("Score: %d", g.score);
        ImGui::Text("Lines: %d", g.lines);
        ImGui::Text("Level: %d", g.level);
        ImGui::Separator();
        ImGui::Text("Next:");

        if (!g.bag.queue.empty()) {
            int t = g.bag.queue[0];
            const auto& c = game::COLORS[game::PIECES[t].colorIndex];
            const game::Cell* pc = game::PIECES[t].rot[0];

            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            float size = 16.0f;
            for (int i = 0; i < 4; ++i) {
                ImVec2 a = ImVec2(p0.x + (pc[i].x + 2) * size, p0.y + (pc[i].y + 2) * size);
                ImVec2 b = ImVec2(a.x + size - 1, a.y + size - 1);
                ImU32 col = IM_COL32((int)(c[0] * 255), (int)(c[1] * 255), (int)(c[2] * 255), 255);
                dl->AddRectFilled(a, b, col, 2.0f);
            }
            ImGui::Dummy(ImVec2(5 * size, 5 * size));
        }
        ImGui::End();
    }

    void DrawGameOver(GLFWwindow* win, game::Game& g, int fbw, int fbh, const std::function<void(void)>& onReset) {
        ImGui::SetNextWindowSize(ImVec2(460, 220), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((fbw - 460) / 2.0f, (fbh - 220) / 2.0f), ImGuiCond_Always);
        ImGui::Begin("Game Over", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        ImGui::Text("FINAL SCORE: %d", g.score);
        if (ImGui::Button("Reset to Start", ImVec2(-1, 0)) && onReset) onReset();
        if (ImGui::Button("Quit", ImVec2(-1, 0))) { if (win) glfwSetWindowShouldClose(win, 1); }
        ImGui::End();
    }

    void DrawHighScores(game::Game& g, int fbw, int fbh, const std::vector<eng::ScoreRow>& rows) {
        ImGui::SetNextWindowSize(ImVec2(520, 360), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2((fbw - 520) / 2.0f, (fbh - 360) / 2.0f), ImGuiCond_Always);
        ImGui::Begin("High Scores", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Top 10");
        ImGui::Separator();
        ImGui::Columns(4, "hscols");
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Score"); ImGui::NextColumn();
        ImGui::Text("Level"); ImGui::NextColumn();
        ImGui::Text("When"); ImGui::NextColumn();
        ImGui::Separator();

        for (const auto& r : rows) {
            ImGui::TextUnformatted(r.name.c_str()); ImGui::NextColumn();
            ImGui::Text("%d", r.score);             ImGui::NextColumn();
            ImGui::Text("%d", r.level);             ImGui::NextColumn();
            ImGui::TextUnformatted(r.when.c_str()); ImGui::NextColumn();
        }
        ImGui::Columns(1);

        if (ImGui::Button("Back")) g.scene = game::Scene::Start;
        ImGui::End();
    }

} // namespace ui
