#pragma once
// Application shell: window + input (GLFW), the game loop, and wiring between
// the game logic, the renderer and the HUD.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "game/GameState.hpp"
#include "game/Player.hpp"
#include "game/Sparks.hpp"
#include "game/Warden.hpp"
#include "game/World.hpp"
#include "gfx/Renderer.hpp"

struct GLFWwindow;

namespace ui {
class Hud;
}

namespace app {

struct AppOptions {
    std::string assetRoot;
    std::string fontDirectory;
    std::uint32_t seed = 7;
    int width = 1280;
    int height = 720;
    bool vsync = true;
};

class App {
public:
    explicit App(AppOptions options);
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    int run();

private:
    void onKey(int key, int action);
    void setCursorCaptured(bool captured);
    ew::PlayerInput readPlayerInput();
    void startRun();
    void simulate(float dt, const ew::PlayerInput& input);
    void handleGameEvents(const std::vector<ew::GameEvent>& events);
    void hitPlayer(const glm::vec3& from);
    gfx::FrameInput buildFrame(int fbWidth, int fbHeight);

    AppOptions options_;
    GLFWwindow* window_ = nullptr;
    std::unique_ptr<gfx::Renderer> renderer_;
    std::unique_ptr<ui::Hud> hud_;

    gfx::RenderSettings settings_;
    ew::WorldLayout world_;
    ew::GameState game_;
    ew::Player player_;
    ew::Warden warden_;
    ew::SparkSystem sparks_;
    std::vector<ew::GameEvent> lastEvents_;

    // Presentation state
    float time_ = 0.0f;
    float hurtFlash_ = 0.0f;
    float shake_ = 0.0f;
    float emberTrailTimer_ = 0.0f;
    bool impactThisFrame_ = false;
    bool paused_ = false;
    bool cursorCaptured_ = false;
    bool firstMouse_ = true;
    double lastMouseX_ = 0.0, lastMouseY_ = 0.0;
};

}  // namespace app
