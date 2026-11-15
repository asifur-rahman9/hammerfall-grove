#include "gfx/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

#include "core/MathUtil.hpp"

namespace gfx {
namespace {

constexpr int kMaxPointLights = 16;

// Material ids understood by scene.frag
constexpr int kMatTextured = 0;
constexpr int kMatGrass = 1;
constexpr int kMatTree = 2;
constexpr int kMatFlat = 3;

// Night palette
const glm::vec3 kFogColor(0.018f, 0.026f, 0.050f);
const glm::vec3 kMoonColor(0.36f, 0.43f, 0.62f);
const glm::vec3 kSkyAmbient(0.040f, 0.055f, 0.105f);
const glm::vec3 kGroundAmbient(0.012f, 0.014f, 0.012f);
const glm::vec3 kSkyTint(0.10f, 0.13f, 0.26f);
const glm::vec3 kEmberCrystal(1.0f, 0.30f, 0.05f);

ew::MeshData loadMeshData(const std::string& path) {
    ew::MeshData data;
    std::string error;
    if (!ew::loadObj(path, data, &error)) throw std::runtime_error("Failed to load model: " + error);
    return data;
}

glm::mat3 normalMatrix(const glm::mat4& model) { return glm::transpose(glm::inverse(glm::mat3(model))); }

}  // namespace

glm::vec3 Renderer::alertColor(float alert) {
    const glm::vec3 calm(0.25f, 0.55f, 1.0f);
    const glm::vec3 aware(1.0f, 0.55f, 0.12f);
    const glm::vec3 angry(1.0f, 0.10f, 0.04f);
    return alert < 0.5f ? glm::mix(calm, aware, alert * 2.0f) : glm::mix(aware, angry, (alert - 0.5f) * 2.0f);
}

Renderer::Renderer(std::string assetRoot) : assetRoot_(std::move(assetRoot)) {
    scene_ = Shader(asset("shaders/scene.vert"), asset("shaders/scene.frag"));
    shadow_ = Shader(asset("shaders/shadow.vert"), asset("shaders/shadow.frag"));
    sky_ = Shader(asset("shaders/sky.vert"), asset("shaders/sky.frag"));
    billboard_ = Shader(asset("shaders/billboard.vert"), asset("shaders/billboard.frag"));

    cube_ = Mesh(loadMeshData(asset("models/cube.obj")));
    cylinder_ = Mesh(loadMeshData(asset("models/cylinder.obj")));
    sphere_ = Mesh(loadMeshData(asset("models/sphere.obj")));
    grassTall_ = Mesh(loadMeshData(asset("models/grass1.obj")));
    grassShort_ = Mesh(loadMeshData(asset("models/grass2.obj")));
    tree_ = Mesh(loadMeshData(asset("models/tree1.obj")));
    ground_ = Mesh(ew::makePlane(260.0f, 7.0f));
    skyCube_ = Mesh(ew::makeUnitCube());

    grassTex_ = Texture::load2D(asset("textures/grass.jpg"));
    woodTex_ = Texture::load2D(asset("textures/wood.png"));
    metalTex_ = Texture::load2D(asset("textures/metal.png"));
    stoneTopTex_ = Texture::load2D(asset("textures/cementTop.png"));
    stoneBaseTex_ = Texture::load2D(asset("textures/cementBase.png"));
    skyTex_ = Texture::loadCubemap({asset("textures/skybox/right.jpg"), asset("textures/skybox/left.jpg"),
                                    asset("textures/skybox/top.jpg"), asset("textures/skybox/bottom.jpg"),
                                    asset("textures/skybox/front.jpg"), asset("textures/skybox/back.jpg")});

    // Unit quad for billboards + a dynamic per-instance buffer.
    const float corners[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
    glGenVertexArrays(1, &quadVao_);
    glBindVertexArray(quadVao_);
    glGenBuffers(1, &quadVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glGenBuffers(1, &billboardVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, billboardVbo_);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Billboard), reinterpret_cast<void*>(offsetof(Billboard, position)));
    glVertexAttribDivisor(1, 1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Billboard), reinterpret_cast<void*>(offsetof(Billboard, color)));
    glVertexAttribDivisor(2, 1);
    glBindVertexArray(0);
    static_assert(sizeof(Billboard) == 8 * sizeof(float), "Billboard must be tightly packed (pos, size, color)");
}

Renderer::~Renderer() {
    if (billboardVbo_) glDeleteBuffers(1, &billboardVbo_);
    if (quadVbo_) glDeleteBuffers(1, &quadVbo_);
    if (quadVao_) glDeleteVertexArrays(1, &quadVao_);
}

std::string Renderer::asset(const std::string& relative) const { return assetRoot_ + "/" + relative; }

void Renderer::setWorld(const ew::WorldLayout& world) {
    grassTall_.setInstances(world.tallGrass);
    grassShort_.setInstances(world.shortGrass);
    tree_.setInstances(world.trees);
}

// ---------------------------------------------------------------------------
// Draw helpers
// ---------------------------------------------------------------------------

void Renderer::drawMesh(const Mesh& mesh, const Shader& shader, const glm::mat4& model) {
    shader.set("uInstanceMode", 0);
    shader.set("uModel", model);
    shader.set("uNormalMatrix", normalMatrix(model));
    mesh.draw();
}

void Renderer::drawInstanced(const Mesh& mesh, const Shader& shader, int instanceMode) {
    shader.set("uInstanceMode", instanceMode);
    mesh.drawInstanced();
}

void Renderer::setMaterial(int material, const Texture* albedo, const glm::vec3& tint, float specular, float shininess,
                           const glm::vec3& emissive, const glm::vec2& uvScale) {
    scene_.set("uMaterial", material);
    if (albedo) albedo->bind(0);
    scene_.set("uTint", tint);
    scene_.set("uSpecular", specular);
    scene_.set("uShininess", shininess);
    scene_.set("uEmissive", emissive);
    scene_.set("uUVScale", uvScale);
}

void Renderer::drawWarden(const Shader& shader, const FrameInput& f, bool shadowPass) {
    const ew::ArmTransforms& a = f.arm;
    const glm::vec3 base = glm::vec3(a.base[3]);
    const glm::vec3 core = alertColor(f.wardenAlert) * (0.6f + 7.0f * f.wardenAlert * f.wardenAlert);
    const bool lit = !shadowPass;

    // Two-tier stone plinth
    if (lit) setMaterial(kMatTextured, &stoneBaseTex_, glm::vec3(0.75f), 0.08f, 16.0f);
    drawMesh(cube_, shader, glm::translate(glm::mat4(1.0f), base + glm::vec3(0.0f, 0.5f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 0.6f, 5.0f)));
    if (lit) setMaterial(kMatTextured, &stoneTopTex_, glm::vec3(0.8f), 0.1f, 16.0f);
    drawMesh(cube_, shader, glm::translate(glm::mat4(1.0f), base + glm::vec3(0.0f, 1.4f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(3.6f, 0.45f, 3.6f)));

    // Shoulder pivot (sphere, rotates with the base)
    drawMesh(sphere_, shader, a.base * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.6f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f)));

    // Bicep (cylinder model: radius 2, height 4 centred on the origin)
    if (lit) setMaterial(kMatTextured, &woodTex_, glm::vec3(0.9f, 0.8f, 0.7f), 0.15f, 24.0f);
    drawMesh(cylinder_, shader, a.shoulder * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.55f, 5.0f, 0.55f)));

    // Elbow joint with the glowing core: the Warden's "eye"
    if (lit) setMaterial(kMatTextured, &metalTex_, glm::vec3(1.4f), 0.9f, 64.0f, core * 0.35f);
    drawMesh(sphere_, shader, a.elbow * glm::scale(glm::mat4(1.0f), glm::vec3(0.36f)));

    // Forearm
    if (lit) setMaterial(kMatTextured, &woodTex_, glm::vec3(0.9f, 0.8f, 0.7f), 0.15f, 24.0f);
    drawMesh(cylinder_, shader, a.elbow * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 5.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.45f, 2.5f, 0.45f)));

    // Hammer head + a rune band that glows with the core
    if (lit) setMaterial(kMatTextured, &metalTex_, glm::vec3(1.6f, 1.6f, 1.75f), 1.0f, 96.0f);
    drawMesh(cube_, shader, a.hammer * glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 8.0f)));
    if (lit) setMaterial(kMatFlat, nullptr, glm::vec3(0.05f), 0.5f, 32.0f, core);
    drawMesh(cube_, shader, a.hammer * glm::scale(glm::mat4(1.0f), glm::vec3(3.08f, 3.08f, 0.5f)));
}

void Renderer::drawEmber(const Shader& shader, const FrameInput& f) {
    if (!f.emberVisible) return;
    // An elongated cube spun around its diagonal reads as a faceted crystal.
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), f.emberPosition) *
                            glm::rotate(glm::mat4(1.0f), f.time * 1.6f, glm::vec3(0.0f, 1.0f, 0.0f)) *
                            glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                            glm::rotate(glm::mat4(1.0f), glm::radians(35.26f), glm::vec3(0.0f, 0.0f, 1.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(0.6f, 0.95f, 0.6f));
    const float pulse = 0.75f + 0.25f * std::sin(f.time * 6.0f);
    setMaterial(kMatFlat, nullptr, glm::vec3(0.2f, 0.05f, 0.01f), 1.0f, 64.0f, kEmberCrystal * (1.8f * pulse * f.emberGlow));
    drawMesh(cube_, shader, model);
}

void Renderer::drawFoliage(const Shader& shader, const RenderSettings& s) {
    const bool lit = &shader == &scene_;
    glDisable(GL_CULL_FACE);  // grass cards are single-sided
    if (lit) {
        scene_.set("uWind", s.windStrength);
        setMaterial(kMatTree, nullptr, glm::vec3(1.0f), 0.05f, 8.0f);
    }
    drawInstanced(tree_, shader, 2);
    if (lit) {
        setMaterial(kMatGrass, nullptr, glm::vec3(1.0f), 0.08f, 12.0f);
        scene_.set("uHeightRef", 1.0f);
    }
    drawInstanced(grassTall_, shader, 1);
    if (lit) {
        scene_.set("uHeightRef", 0.4f);
        drawInstanced(grassShort_, shader, 1);  // too short to cast a visible shadow
    }
    glEnable(GL_CULL_FACE);
}

void Renderer::drawBillboards(const FrameInput& f, const glm::mat4& viewProj) {
    if (f.billboards.empty()) return;
    glBindBuffer(GL_ARRAY_BUFFER, billboardVbo_);
    const std::size_t bytes = f.billboards.size() * sizeof(Billboard);
    if (f.billboards.size() > billboardCapacity_) {
        billboardCapacity_ = std::max<std::size_t>(f.billboards.size(), billboardCapacity_ * 2);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(billboardCapacity_ * sizeof(Billboard)), nullptr, GL_STREAM_DRAW);
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bytes), f.billboards.data());

    const glm::mat4 invView = glm::inverse(f.view);
    billboard_.use();
    billboard_.set("uViewProj", viewProj);
    billboard_.set("uCamRight", glm::vec3(invView[0]));
    billboard_.set("uCamUp", glm::vec3(invView[1]));

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);  // additive: glows accumulate energy
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(quadVao_);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(f.billboards.size()));
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// ---------------------------------------------------------------------------
// Frame
// ---------------------------------------------------------------------------

void Renderer::render(const FrameInput& f, const RenderSettings& s, int fbW, int fbH) {
    if (fbW <= 0 || fbH <= 0) return;
    shadowMap_.create(Framebuffer::Kind::Depth, s.shadowMapSize, s.shadowMapSize);

    const glm::vec3 moonDir = ew::moonDirection();
    const glm::vec3 center(5.0f, 0.0f, 5.0f);
    const glm::mat4 lightProj = glm::ortho(-115.0f, 115.0f, -115.0f, 115.0f, 1.0f, 420.0f);
    const glm::mat4 lightView = glm::lookAt(center - moonDir * 200.0f, center, glm::vec3(0.0f, 1.0f, 0.0f));
    lightSpace_ = lightProj * lightView;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    // ---- 1. Shadow pass ----
    shadowMap_.bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    shadow_.use();
    shadow_.set("uLightSpace", lightSpace_);
    glCullFace(GL_FRONT);  // render back faces into the map to reduce shadow acne
    drawWarden(shadow_, f, true);
    glCullFace(GL_BACK);
    drawFoliage(shadow_, s);

    // ---- 2. Scene pass, straight to the window ----
    // GL_FRAMEBUFFER_SRGB makes OpenGL apply gamma correction when writing colors.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbW, fbH);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(kFogColor.r, kFogColor.g, kFogColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 viewProj = f.projection * f.view;
    scene_.use();
    scene_.set("uViewProj", viewProj);
    scene_.set("uLightSpace", lightSpace_);
    scene_.set("uTime", f.time);
    scene_.set("uWind", 0.0f);
    scene_.set("uCamPos", f.cameraPosition);
    scene_.set("uAlbedo", 0);
    scene_.set("uShadowMap", 1);
    scene_.set("uShadows", 1);
    scene_.set("uPCFRadius", s.pcfRadius);
    scene_.set("uShadowTexel", glm::vec2(1.0f / static_cast<float>(s.shadowMapSize)));
    scene_.set("uMoonDir", moonDir);
    scene_.set("uMoonColor", kMoonColor);
    scene_.set("uSkyAmbient", kSkyAmbient);
    scene_.set("uGroundAmbient", kGroundAmbient);
    scene_.set("uFogColor", kFogColor);
    scene_.set("uFogDensity", s.fogDensity);
    scene_.set("uHeightRef", 1.0f);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowMap_.texture());

    // Point lights: nearest to the camera first if there are too many.
    std::vector<PointLight> lights = f.lights;
    if (static_cast<int>(lights.size()) > kMaxPointLights) {
        std::partial_sort(lights.begin(), lights.begin() + kMaxPointLights, lights.end(), [&](const PointLight& a, const PointLight& b) {
            return glm::length(a.position - f.cameraPosition) < glm::length(b.position - f.cameraPosition);
        });
        lights.resize(kMaxPointLights);
    }
    std::vector<glm::vec3> lightPos, lightColor;
    std::vector<float> lightRange;
    for (const auto& l : lights) {
        lightPos.push_back(l.position);
        lightColor.push_back(l.color);
        lightRange.push_back(l.range);
    }
    const int lightCount = static_cast<int>(lights.size());
    scene_.set("uPointCount", lightCount);
    scene_.set("uPointPos", lightPos.data(), lightCount);
    scene_.set("uPointColor", lightColor.data(), lightCount);
    if (lightCount > 0) glUniform1fv(glGetUniformLocation(scene_.id(), "uPointRange"), lightCount, lightRange.data());

    // Ground
    setMaterial(kMatTextured, &grassTex_, glm::vec3(0.20f, 0.27f, 0.17f), 0.04f, 8.0f);
    drawMesh(ground_, scene_, glm::mat4(1.0f));

    drawWarden(scene_, f, false);
    drawEmber(scene_, f);
    drawFoliage(scene_, s);

    // Sky last: with depth test LEQUAL it only fills pixels nothing else covered
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    sky_.use();
    sky_.set("uViewRotProj", f.projection * glm::mat4(glm::mat3(f.view)));
    sky_.set("uSky", 0);
    sky_.set("uSkyTint", kSkyTint);
    sky_.set("uMoonDir", moonDir);
    sky_.set("uFogColor", kFogColor);
    sky_.set("uTime", f.time);
    skyTex_.bind(0);
    skyCube_.draw();
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    drawBillboards(f, viewProj);
    glDisable(GL_FRAMEBUFFER_SRGB);  // the HUD is drawn after this without gamma correction
}

}  // namespace gfx
