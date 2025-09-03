#pragma once
#include <functional>
#include <vector>

struct GLFWwindow;
namespace eng { class Renderer; class Texture2D; struct ScoreRow; }
namespace game { struct Game; }

namespace ui {

	void DrawStart(GLFWwindow* win, game::Game& g, int fbw, int fbh, eng::Texture2D* logo);
	void DrawControls(game::Game& g, int fbw, int fbh);
	void DrawSettings(game::Game& g, int fbw, int fbh, const std::function<bool(bool)>& onMusicToggle);
	void DrawLevelSelect(game::Game& g, int fbw, int fbh, const std::function<void(int)>& onStart);
	void DrawHUD(eng::Renderer& r, game::Game& g, int fbw, int fbh);
	void DrawGameOver(GLFWwindow* win, game::Game& g, int fbw, int fbh, const std::function<void(void)>& onReset);
	void DrawHighScores(game::Game& g, int fbw, int fbh, const std::vector<eng::ScoreRow>& rows);

} // namespace ui
