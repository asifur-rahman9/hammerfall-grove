#pragma once
// In-game HUD (score, lives, ember timer, menus), drawn with Dear ImGui.

#include <deque>
#include <string>

#include "game/GameState.hpp"
#include "game/Warden.hpp"

struct GLFWwindow;
struct ImFont;

namespace ui {

struct Toast {
    std::string text;
    float age = 0.0f;
    float lifetime = 1.2f;
    unsigned int color = 0xFFFFFFFF;
};

struct HudModel {
    const ew::GameState* game = nullptr;
    ew::WardenState wardenState = ew::WardenState::Dormant;
    float wardenAlert = 0.0f;
    float hurt = 0.0f;
    bool paused = false;
    float time = 0.0f;
};

class Hud {
public:
    Hud(GLFWwindow* window, const std::string& fontDirectory);
    ~Hud();
    Hud(const Hud&) = delete;
    Hud& operator=(const Hud&) = delete;

    void beginFrame();
    void drawGame(const HudModel& model);
    void endFrame();  // renders ImGui draw data into the current framebuffer

    void addToast(const std::string& text, unsigned int color = 0xFFFFFFFF);
    void update(float dt);
    bool wantsMouse() const;

private:
    void drawTitle(const HudModel& m);
    void drawPlaying(const HudModel& m);
    void drawGameOver(const HudModel& m);

    ImFont* body_ = nullptr;
    ImFont* large_ = nullptr;
    ImFont* title_ = nullptr;
    std::deque<Toast> toasts_;
};

}  // namespace ui
