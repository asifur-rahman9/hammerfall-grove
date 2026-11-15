#include "app/App.hpp"

#include "gfx/GL.hpp"  // GLEW must precede GLFW
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>

#include "core/MathUtil.hpp"
#include "ui/Hud.hpp"

namespace app {
namespace {

const glm::vec3 kCenter(5.0f, 0.0f, 5.0f);
const glm::vec3 kEmberColor(1.0f, 0.42f, 0.10f);
const glm::vec3 kLanternColor(1.0f, 0.62f, 0.30f);
constexpr int kFireflyCount = 8;

void glfwErrorCallback(int code, const char* description) { std::cerr << "GLFW error " << code << ": " << description << '\n'; }

}  // namespace

App::App(AppOptions options)
    : options_(std::move(options)), game_(options_.seed), sparks_(4096, options_.seed * 31u + 5u) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // 4x anti-aliasing
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(options_.width, options_.height, "Hammerfall Grove", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create an OpenGL 3.3+ core window");
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(options_.vsync ? 1 : 0);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) throw std::runtime_error("glewInit failed");
    glGetError();  // GLEW can leave a harmless GL_INVALID_ENUM behind on core profiles
    glEnable(GL_MULTISAMPLE);

    std::cout << "OpenGL " << glGetString(GL_VERSION) << " | " << glGetString(GL_RENDERER) << '\n';

    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, [](GLFWwindow* w, int key, int, int action, int) {
        static_cast<App*>(glfwGetWindowUserPointer(w))->onKey(key, action);
    });
    if (glfwRawMouseMotionSupported()) glfwSetInputMode(window_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    renderer_ = std::make_unique<gfx::Renderer>(options_.assetRoot);
    world_ = ew::generateWorld();
    renderer_->setWorld(world_);
    hud_ = std::make_unique<ui::Hud>(window_, options_.fontDirectory);  // installs chained GLFW callbacks

    game_.setHighScorePath("highscore.txt");
    game_.loadHighScore();

    player_.reset(glm::vec3(-30.0f, player_.params().eyeHeight, -24.0f), 39.6f, -4.0f);
}

App::~App() {
    hud_.reset();
    renderer_.reset();
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void App::onKey(int key, int action) {
    if (action != GLFW_PRESS) return;
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window_, GLFW_TRUE); break;
        case GLFW_KEY_P:
            if (game_.phase() == ew::Phase::Playing) paused_ = !paused_;
            break;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            if (game_.phase() != ew::Phase::Playing) startRun();
            break;
        case GLFW_KEY_R:
            if (game_.phase() == ew::Phase::GameOver) startRun();
            break;
        default: break;
    }
}

void App::setCursorCaptured(bool captured) {
    if (captured == cursorCaptured_) return;
    cursorCaptured_ = captured;
    glfwSetInputMode(window_, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    firstMouse_ = true;
}

ew::PlayerInput App::readPlayerInput() {
    ew::PlayerInput in;
    if (!cursorCaptured_) return in;
    auto down = [&](int k) { return glfwGetKey(window_, k) == GLFW_PRESS; };
    in.forward = down(GLFW_KEY_W) || down(GLFW_KEY_UP);
    in.back = down(GLFW_KEY_S) || down(GLFW_KEY_DOWN);
    in.left = down(GLFW_KEY_A) || down(GLFW_KEY_LEFT);
    in.right = down(GLFW_KEY_D) || down(GLFW_KEY_RIGHT);
    in.jump = down(GLFW_KEY_SPACE);
    in.sprint = down(GLFW_KEY_LEFT_SHIFT) || down(GLFW_KEY_RIGHT_SHIFT);

    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(window_, &mx, &my);
    if (firstMouse_) {
        lastMouseX_ = mx;
        lastMouseY_ = my;
        firstMouse_ = false;
    }
    in.lookDeltaX = static_cast<float>(mx - lastMouseX_);
    in.lookDeltaY = static_cast<float>(my - lastMouseY_);
    lastMouseX_ = mx;
    lastMouseY_ = my;
    return in;
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

void App::startRun() {
    game_.startRun();
    warden_.reset();
    sparks_.clear();
    player_.reset(glm::vec3(-30.0f, player_.params().eyeHeight, -24.0f), 39.6f, -4.0f);
    paused_ = false;
    hurtFlash_ = shake_ = 0.0f;
}

void App::hitPlayer(const glm::vec3& from) {
    if (!game_.registerHit(from)) return;
    player_.knockBack(from);
    hurtFlash_ = 1.0f;
    shake_ = std::max(shake_, 1.0f);
    if (game_.phase() == ew::Phase::GameOver) {
        hud_->addToast("The last lantern dies", IM_COL32(255, 90, 60, 255));
    } else {
        hud_->addToast("-1 lantern", IM_COL32(255, 90, 60, 255));
    }
}

void App::handleGameEvents(const std::vector<ew::GameEvent>& events) {
    for (const auto& e : events) {
        switch (e.type) {
            case ew::GameEventType::EmberCaught: {
                ew::BurstDesc burst;
                burst.origin = e.position;
                burst.count = 160;
                burst.color = kEmberColor * 3.5f;
                burst.size = 0.11f;
                burst.minSpeed = 3.0f;
                burst.maxSpeed = 13.0f;
                burst.upwardBias = 0.45f;
                burst.gravityScale = 0.7f;
                sparks_.burst(burst);
                hud_->addToast("+" + std::to_string(e.points) + (e.points > 1 ? "  chain!" : ""), IM_COL32(255, 200, 110, 255));
                shake_ = std::max(shake_, 0.15f);
                break;
            }
            case ew::GameEventType::EmberExpired: {
                ew::BurstDesc fizzle;
                fizzle.origin = e.position;
                fizzle.count = 50;
                fizzle.color = glm::vec3(0.6f, 0.35f, 0.25f);
                fizzle.minSpeed = 0.5f;
                fizzle.maxSpeed = 2.5f;
                fizzle.gravityScale = -0.15f;  // ash drifts upward
                fizzle.size = 0.15f;
                sparks_.burst(fizzle);
                hud_->addToast("The ember faded", IM_COL32(170, 170, 190, 255));
                break;
            }
            default: break;
        }
    }
}

void App::simulate(float dt, const ew::PlayerInput& input) {
    impactThisFrame_ = false;
    if (dt <= 0.0f) return;
    time_ += dt;

    const ew::Phase phase = game_.phase();
    if (phase == ew::Phase::Playing) {
        player_.update(input, dt);
    } else if (phase == ew::Phase::GameOver) {
        ew::PlayerInput lookOnly;
        lookOnly.lookDeltaX = input.lookDeltaX;
        lookOnly.lookDeltaY = input.lookDeltaY;
        player_.update(lookOnly, dt);
    }

    warden_.update(dt, player_.eye(), player_.params().eyeHeight, ew::wardenParamsForLevel(game_.level()));

    // Hammer collisions: a direct hit during the slam, or the shockwave at impact.
    if (phase == ew::Phase::Playing) {
        if (warden_.isStriking() && glm::length(warden_.hammerPosition() - player_.eye()) < 5.5f) {
            hitPlayer(kCenter);
        }
    }
    glm::vec3 impact;
    if (warden_.consumeImpact(impact)) {
        impactThisFrame_ = true;
        ew::BurstDesc dust;
        dust.origin = glm::vec3(impact.x, 0.4f, impact.z);
        dust.count = 240;
        dust.color = glm::vec3(2.6f, 1.6f, 0.9f);
        dust.minSpeed = 4.0f;
        dust.maxSpeed = 16.0f;
        dust.upwardBias = 0.25f;
        dust.size = 0.18f;
        dust.drag = 2.5f;
        sparks_.burst(dust);
        const float dist = ew::horizontalDistance(impact, player_.eye());
        shake_ = std::max(shake_, glm::clamp(1.2f - dist / 40.0f, 0.0f, 1.0f));
        if (phase == ew::Phase::Playing && dist < 6.5f && player_.eye().y < impact.y + 6.0f) hitPlayer(kCenter);
    }

    lastEvents_ = game_.update(dt, player_.eye(), player_.forward());
    handleGameEvents(lastEvents_);

    // Embers shed rising sparks while alive.
    const ew::Ember& ember = game_.ember();
    if (ember.active) {
        emberTrailTimer_ -= dt;
        while (emberTrailTimer_ <= 0.0f) {
            emberTrailTimer_ += 0.025f;
            ew::Spark s;
            const float a = time_ * 13.0f;
            s.position = ember.position() + glm::vec3(0.35f * std::cos(a), -0.2f, 0.35f * std::sin(a));
            s.velocity = glm::vec3(0.3f * std::sin(a * 1.7f), 1.6f, 0.3f * std::cos(a * 1.3f));
            s.maxLife = s.life = 1.3f;
            s.color = kEmberColor * 4.0f * ember.remainingFraction();
            s.size = 0.09f;
            s.gravityScale = -0.05f;
            s.drag = 0.6f;
            sparks_.emit(s);
        }
    }
    sparks_.update(dt);

    hurtFlash_ = ew::damp(hurtFlash_, 0.0f, 3.5f, dt);
    shake_ = ew::damp(shake_, 0.0f, 4.0f, dt);
    hud_->update(dt);
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

gfx::FrameInput App::buildFrame(int fbW, int fbH) {
    gfx::FrameInput f;
    f.time = time_;

    glm::vec3 camPos;
    glm::vec3 camTarget;
    if (game_.phase() == ew::Phase::Title) {
        // Attract mode: slow orbit around the Warden.
        const float a = 0.9f + 0.07f * time_;
        camPos = kCenter + glm::vec3(44.0f * std::cos(a), 7.0f, 44.0f * std::sin(a));
        camTarget = kCenter + glm::vec3(0.0f, 15.0f, 0.0f);
    } else {
        camPos = player_.eye();
        camTarget = camPos + player_.forward();
    }
    if (shake_ > 0.001f) {
        const glm::vec3 jitter(std::sin(time_ * 47.0f), std::sin(time_ * 53.0f + 1.7f), std::sin(time_ * 41.0f + 3.1f));
        camPos += jitter * shake_ * 0.35f;
        camTarget += jitter * shake_ * 0.2f;
    }
    f.cameraPosition = camPos;
    f.view = glm::lookAt(camPos, camTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    f.projection = glm::perspective(glm::radians(settings_.fovDegrees), static_cast<float>(fbW) / static_cast<float>(fbH), 0.1f, 600.0f);

    f.arm = ew::forwardKinematics(warden_.config(), warden_.pose());
    f.wardenAlert = warden_.alertness();

    // Fireflies: a light + a glow each
    for (const auto& fly : ew::fireflies(time_, kFireflyCount)) {
        f.lights.push_back({fly.position, fly.color * 0.55f, 11.0f});
        f.billboards.push_back({fly.position, 0.6f, glm::vec4(fly.color * 1.3f, 1.0f)});
    }

    // The ember
    const ew::Ember& ember = game_.ember();
    if (ember.active) {
        const float remaining = ember.remainingFraction();
        // Flickers as it is about to die out.
        const float flicker = ember.remaining() < 3.0f ? 0.65f + 0.35f * std::sin(time_ * 22.0f) : 1.0f;
        f.emberVisible = true;
        f.emberPosition = ember.position();
        f.emberGlow = (0.35f + 0.65f * remaining) * flicker;
        f.lights.push_back({f.emberPosition, kEmberColor * 7.0f * f.emberGlow, 22.0f});
        f.billboards.push_back({f.emberPosition, 2.4f, glm::vec4(kEmberColor * 1.3f, f.emberGlow)});
    }

    // The Warden's core light, glowing with its alertness
    const glm::vec3 core = gfx::Renderer::alertColor(warden_.alertness());
    const float coreIntensity = 0.8f + 9.0f * warden_.alertness() * warden_.alertness();
    f.lights.push_back({f.arm.elbowPosition, core * coreIntensity, 20.0f});
    f.billboards.push_back({f.arm.elbowPosition, 2.0f + 2.5f * warden_.alertness(), glm::vec4(core * 1.6f, 0.4f + warden_.alertness())});

    // The player's lantern: lights the grass around you
    if (game_.phase() != ew::Phase::Title) {
        const glm::vec3 lantern = player_.eye() + glm::vec3(0.0f, -1.3f, 0.0f) + player_.right() * 0.7f + player_.flatForward() * 0.6f;
        const float lanternPower = game_.phase() == ew::Phase::GameOver ? 0.2f : 1.7f;
        f.lights.push_back({lantern, kLanternColor * lanternPower, 14.0f});
    }

    for (const auto& s : sparks_.sparks()) {
        const float k = s.life / s.maxLife;
        f.billboards.push_back({s.position, s.size * (0.4f + 0.6f * k), glm::vec4(s.color, k)});
    }
    return f;
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------

int App::run() {
    using Clock = std::chrono::steady_clock;
    auto last = Clock::now();

    while (!glfwWindowShouldClose(window_)) {
        const auto frameStart = Clock::now();
        glfwPollEvents();

        const float realDt = std::chrono::duration<float>(frameStart - last).count();
        last = frameStart;
        const float dt = std::min(realDt, 0.05f);  // avoid huge jumps after a stall

        const bool wantCursor = game_.phase() == ew::Phase::Playing && !paused_;
        setCursorCaptured(wantCursor);
        const ew::PlayerInput input = readPlayerInput();
        simulate(paused_ ? 0.0f : dt, input);

        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(window_, &fbW, &fbH);
        if (fbW == 0 || fbH == 0) continue;  // minimized

        const gfx::FrameInput frameInput = buildFrame(fbW, fbH);
        renderer_->render(frameInput, settings_, fbW, fbH);

        hud_->beginFrame();
        {
            ui::HudModel hm;
            hm.game = &game_;
            hm.wardenState = warden_.state();
            hm.wardenAlert = warden_.alertness();
            hm.hurt = hurtFlash_;
            hm.paused = paused_;
            hm.time = time_;
            hud_->drawGame(hm);
        }
        hud_->endFrame();

        glfwSwapBuffers(window_);
    }

    if (game_.phase() == ew::Phase::GameOver || game_.score() > 0) game_.saveHighScore();
    return 0;
}

}  // namespace app
