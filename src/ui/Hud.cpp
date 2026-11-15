#include "ui/Hud.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

namespace ui {
namespace {

ImU32 rgba(int r, int g, int b, float a) { return IM_COL32(r, g, b, static_cast<int>(std::clamp(a, 0.0f, 1.0f) * 255.0f)); }

ImU32 withAlpha(ImU32 col, float a) {
    const int alpha = static_cast<int>(((col >> IM_COL32_A_SHIFT) & 0xFF) * std::clamp(a, 0.0f, 1.0f));
    return (col & ~IM_COL32_A_MASK) | (static_cast<ImU32>(alpha) << IM_COL32_A_SHIFT);
}

// Text with a drop shadow so it's readable over bright parts of the scene.
void shadowText(ImDrawList* dl, ImFont* font, float size, ImVec2 pos, ImU32 col, const char* text) {
    const ImU32 shadow = withAlpha(IM_COL32(0, 0, 0, 200), ((col >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f);
    dl->AddText(font, size, ImVec2(pos.x + 2, pos.y + 2), shadow, text);
    dl->AddText(font, size, pos, col, text);
}

void centeredText(ImDrawList* dl, ImFont* font, float size, float centerX, float y, ImU32 col, const char* text) {
    const ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
    shadowText(dl, font, size, ImVec2(centerX - ts.x * 0.5f, y), col, text);
}

bool fileExists(const std::string& p) { return std::ifstream(p).good(); }

}  // namespace

Hud::Hud(GLFWwindow* window, const std::string& fontDirectory) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // don't litter the working directory
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    // Rasterize fonts at the framebuffer scale (Retina) for crisp text.
    int ww = 0, fw = 0, wh = 0, fh = 0;
    glfwGetWindowSize(window, &ww, &wh);
    glfwGetFramebufferSize(window, &fw, &fh);
    const float scale = ww > 0 ? std::max(1.0f, static_cast<float>(fw) / static_cast<float>(ww)) : 1.0f;

    const std::string font = fontDirectory + "/Roboto-Medium.ttf";
    if (fileExists(font)) {
        body_ = io.Fonts->AddFontFromFileTTF(font.c_str(), 17.0f * scale);
        large_ = io.Fonts->AddFontFromFileTTF(font.c_str(), 30.0f * scale);
        title_ = io.Fonts->AddFontFromFileTTF(font.c_str(), 72.0f * scale);
        io.FontGlobalScale = 1.0f / scale;
    } else {
        ImFontConfig cfg;
        cfg.SizePixels = 13.0f * scale;
        body_ = large_ = title_ = io.Fonts->AddFontDefault(&cfg);
        io.FontGlobalScale = 1.0f / scale;
    }

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

Hud::~Hud() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Hud::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Hud::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool Hud::wantsMouse() const { return ImGui::GetIO().WantCaptureMouse; }

void Hud::addToast(const std::string& text, unsigned int color) {
    toasts_.push_back(Toast{text, 0.0f, 1.2f, color});
    while (toasts_.size() > 4) toasts_.pop_front();
}

void Hud::update(float dt) {
    for (auto& t : toasts_) t.age += dt;
    while (!toasts_.empty() && toasts_.front().age > toasts_.front().lifetime) toasts_.pop_front();
}

void Hud::drawGame(const HudModel& m) {
    if (!m.game) return;
    // Red flash when the hammer hits you.
    if (m.hurt > 0.01f) {
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, rgba(140, 10, 5, 0.45f * m.hurt));
    }
    switch (m.game->phase()) {
        case ew::Phase::Title: drawTitle(m); break;
        case ew::Phase::Playing: drawPlaying(m); break;
        case ew::Phase::GameOver: drawGameOver(m); break;
    }
    if (m.paused) {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const ImVec2 ds = ImGui::GetIO().DisplaySize;
        dl->AddRectFilled(ImVec2(0, 0), ds, rgba(0, 0, 0, 0.45f));
        centeredText(dl, large_, 30.0f, ds.x * 0.5f, ds.y * 0.45f, IM_COL32_WHITE, "PAUSED  -  press P to resume");
    }
}

void Hud::drawTitle(const HudModel& m) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float cx = ds.x * 0.5f;
    const float glow = 0.75f + 0.25f * std::sin(m.time * 2.0f);

    dl->AddRectFilledMultiColor(ImVec2(0, ds.y * 0.55f), ds, rgba(0, 0, 0, 0.0f), rgba(0, 0, 0, 0.0f), rgba(0, 0, 0, 0.75f), rgba(0, 0, 0, 0.75f));
    centeredText(dl, title_, 72.0f, cx, ds.y * 0.16f, rgba(255, 170, 70, glow), "HAMMERFALL GROVE");
    centeredText(dl, body_, 17.0f, cx, ds.y * 0.16f + 84.0f, rgba(220, 225, 240, 0.85f),
                 "The forest's last embers drift through the clearing. An old automaton guards them.");

    const float y = ds.y * 0.70f;
    centeredText(dl, large_, 30.0f, cx, y, rgba(255, 255, 255, 0.55f + 0.45f * std::sin(m.time * 3.0f) * std::sin(m.time * 3.0f)),
                 "Press ENTER to begin");
    centeredText(dl, body_, 17.0f, cx, y + 52.0f, rgba(200, 205, 220, 0.8f),
                 "WASD move   |   Mouse look   |   Shift sprint   |   Space jump");
    centeredText(dl, body_, 17.0f, cx, y + 78.0f, rgba(200, 205, 220, 0.8f),
                 "Catch embers before they fade. Watch the Warden's core: when it burns red, move.");
    char best[64];
    std::snprintf(best, sizeof(best), "Best: %d", m.game->bestScore());
    centeredText(dl, body_, 17.0f, cx, y + 112.0f, rgba(255, 190, 110, 0.8f), best);
}

void Hud::drawPlaying(const HudModel& m) {
    const ew::GameState& g = *m.game;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float pad = 24.0f;

    // --- Score block (top-left) ---
    char buf[128];
    std::snprintf(buf, sizeof(buf), "%d", g.score());
    shadowText(dl, large_, 30.0f, ImVec2(pad, pad - 4), rgba(255, 200, 120, 1.0f), "EMBERS");
    shadowText(dl, title_, 56.0f, ImVec2(pad, pad + 26), IM_COL32_WHITE, buf);
    if (g.combo() > 1) {
        std::snprintf(buf, sizeof(buf), "x%d chain", g.combo());
        shadowText(dl, large_, 30.0f, ImVec2(pad + 4, pad + 88), rgba(255, 150, 60, 0.95f), buf);
    }
    std::snprintf(buf, sizeof(buf), "Best %d   |   Level %d", g.bestScore(), g.level() + 1);
    shadowText(dl, body_, 17.0f, ImVec2(pad + 2, pad + (g.combo() > 1 ? 124 : 92)), rgba(210, 215, 230, 0.75f), buf);

    // --- Lanterns (lives), top-right ---
    const float r = 13.0f;
    const int maxLives = g.rules().startingLives;
    for (int i = 0; i < maxLives; ++i) {
        const ImVec2 c(ds.x - pad - r - static_cast<float>(i) * (r * 2.8f), pad + r + 6);
        const bool alive = i < g.lives();
        if (alive) {
            for (int k = 3; k >= 1; --k) dl->AddCircleFilled(c, r * (1.0f + 0.45f * k), rgba(255, 150, 50, 0.06f));
            dl->AddCircleFilled(c, r, rgba(255, 175, 80, 1.0f));
            dl->AddCircleFilled(c, r * 0.5f, rgba(255, 240, 200, 1.0f));
        } else {
            dl->AddCircle(c, r, rgba(140, 140, 160, 0.6f), 0, 2.0f);
        }
    }
    shadowText(dl, body_, 17.0f, ImVec2(ds.x - pad - 112, pad + 2 * r + 16), rgba(210, 215, 230, 0.7f), "LANTERNS");

    // --- Ember timer bar, top-centre ---
    const ew::Ember& e = g.ember();
    if (e.active) {
        const float w = std::min(420.0f, ds.x * 0.36f);
        const ImVec2 p0(ds.x * 0.5f - w * 0.5f, pad + 8);
        const ImVec2 p1(p0.x + w, p0.y + 10);
        const float f = e.remainingFraction();
        const bool urgent = e.remaining() < 3.0f;
        const float flash = urgent ? 0.6f + 0.4f * std::sin(m.time * 14.0f) : 1.0f;
        dl->AddRectFilled(p0, p1, rgba(10, 12, 20, 0.7f), 5.0f);
        dl->AddRectFilled(p0, ImVec2(p0.x + w * f, p1.y), urgent ? rgba(255, 70, 40, flash) : rgba(255, 150, 50, 0.95f), 5.0f);
        std::snprintf(buf, sizeof(buf), "Ember fades in %.1fs", std::max(0.0f, e.remaining()));
        centeredText(dl, body_, 17.0f, ds.x * 0.5f, p1.y + 6, rgba(230, 230, 240, 0.85f), buf);
    }

    // --- Crosshair ---
    const ImVec2 cc(ds.x * 0.5f, ds.y * 0.5f);
    dl->AddCircle(cc, 7.0f, rgba(255, 255, 255, 0.35f), 24, 1.5f);
    dl->AddCircleFilled(cc, 1.8f, rgba(255, 255, 255, 0.8f));

    // --- Warden telegraph ---
    if (m.wardenState == ew::WardenState::WindUp || m.wardenState == ew::WardenState::Strike) {
        const float a = 0.8f + 0.2f * std::sin(m.time * 16.0f);
        centeredText(dl, large_, 30.0f, ds.x * 0.5f, ds.y * 0.72f, rgba(255, 120, 80, a), "THE WARDEN RAISES ITS HAMMER");
    } else if (g.invulnerableFor() > 0.0f) {
        centeredText(dl, body_, 17.0f, ds.x * 0.5f, ds.y * 0.72f, rgba(255, 180, 160, 0.8f), "A lantern goes dark...");
    }

    // --- Toasts float up from the crosshair ---
    float y = ds.y * 0.5f - 40.0f;
    for (const auto& t : toasts_) {
        const float k = t.age / t.lifetime;
        const float alpha = 1.0f - k * k;
        centeredText(dl, large_, 30.0f, ds.x * 0.5f, y - k * 50.0f, withAlpha(t.color, alpha), t.text.c_str());
        y -= 34.0f;
    }
}

void Hud::drawGameOver(const HudModel& m) {
    const ew::GameState& g = *m.game;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    const float cx = ds.x * 0.5f;
    dl->AddRectFilled(ImVec2(0, 0), ds, rgba(0, 0, 0, 0.55f));
    centeredText(dl, title_, 72.0f, cx, ds.y * 0.24f, rgba(255, 120, 60, 1.0f), "THE CLEARING GOES DARK");
    char buf[128];
    std::snprintf(buf, sizeof(buf), "Embers gathered: %d      Survived: %.0fs      Best: %d", g.score(), g.runTime(), g.bestScore());
    centeredText(dl, large_, 30.0f, cx, ds.y * 0.24f + 100.0f, IM_COL32_WHITE, buf);
    if (g.newBest()) centeredText(dl, large_, 30.0f, cx, ds.y * 0.24f + 145.0f, rgba(255, 200, 90, 0.6f + 0.4f * std::sin(m.time * 4.0f)), "New best!");
    centeredText(dl, body_, 17.0f, cx, ds.y * 0.62f, rgba(220, 225, 240, 0.85f), "Press R to try again   |   ESC to quit");
}

}  // namespace ui
