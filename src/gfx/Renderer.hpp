#pragma once
// Draws one frame in two passes:
//   1. Shadow map: depth from the moon's point of view
//   2. Scene: ground, arm, ember, trees and grass, then the sky and the glowing sprites

#include <memory>
#include <string>
#include <vector>

#include "game/Kinematics.hpp"
#include "game/World.hpp"
#include "gfx/Framebuffer.hpp"
#include "gfx/Mesh.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture.hpp"

namespace gfx {

struct RenderSettings {
    int pcfRadius = 2;          // shadow softness: 0 = hard, 1 = 3x3, 2 = 5x5 samples
    int shadowMapSize = 4096;
    float fogDensity = 0.0115f;
    float windStrength = 1.0f;
    float fovDegrees = 65.0f;
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float range;
};

struct Billboard {
    glm::vec3 position;
    float halfSize;
    glm::vec4 color;  // rgb = color, a = intensity
};

struct FrameInput {
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::vec3 cameraPosition{0.0f};
    float time = 0.0f;

    ew::ArmTransforms arm;
    float wardenAlert = 0.0f;

    bool emberVisible = false;
    glm::vec3 emberPosition{0.0f};
    float emberGlow = 1.0f;  // 0..1 (fades as the ember expires)

    std::vector<PointLight> lights;
    std::vector<Billboard> billboards;
};

class Renderer {
public:
    explicit Renderer(std::string assetRoot);
    ~Renderer();

    void setWorld(const ew::WorldLayout& world);
    void render(const FrameInput& frame, const RenderSettings& settings, int framebufferWidth, int framebufferHeight);

    // Color used to tint the Warden's core: blue (asleep) -> amber -> red (striking).
    static glm::vec3 alertColor(float alert);

private:
    std::string asset(const std::string& relative) const;
    void drawMesh(const Mesh& mesh, const Shader& shader, const glm::mat4& model);
    void drawInstanced(const Mesh& mesh, const Shader& shader, int instanceMode);
    void setMaterial(int material, const Texture* albedo, const glm::vec3& tint, float specular, float shininess,
                     const glm::vec3& emissive = glm::vec3(0.0f), const glm::vec2& uvScale = glm::vec2(1.0f));
    void drawWarden(const Shader& shader, const FrameInput& frame, bool shadowPass);
    void drawEmber(const Shader& shader, const FrameInput& frame);
    void drawFoliage(const Shader& shader, const RenderSettings& settings);
    void drawBillboards(const FrameInput& frame, const glm::mat4& viewProj);

    std::string assetRoot_;

    // Shaders
    Shader scene_, shadow_, sky_, billboard_;

    // Meshes
    Mesh cube_, cylinder_, sphere_, grassTall_, grassShort_, tree_, ground_, skyCube_;

    // Textures
    Texture grassTex_, woodTex_, metalTex_, stoneTopTex_, stoneBaseTex_, skyTex_;

    // Render targets
    Framebuffer shadowMap_;

    GLuint quadVao_ = 0, quadVbo_ = 0, billboardVbo_ = 0;
    std::size_t billboardCapacity_ = 0;

    glm::mat4 lightSpace_{1.0f};
};

}  // namespace gfx
