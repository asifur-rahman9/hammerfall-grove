#pragma once
// GPU mesh: interleaved position/normal/uv buffer + optional per-instance buffer.
// Vertex layout (shared by every scene shader):
//   location 0: vec3 position, 1: vec3 normal, 2: vec2 uv, 3: vec4 instance (x, z, yaw, scale)

#include <glm/glm.hpp>
#include <vector>

#include "core/ObjLoader.hpp"
#include "gfx/GL.hpp"

namespace gfx {

class Mesh {
public:
    Mesh() = default;
    explicit Mesh(const ew::MeshData& data);
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void setInstances(const std::vector<glm::vec4>& instances);

    void draw() const;
    void drawInstanced() const;

    GLsizei vertexCount() const { return vertexCount_; }
    GLsizei instanceCount() const { return instanceCount_; }
    const glm::vec3& boundsMin() const { return boundsMin_; }
    const glm::vec3& boundsMax() const { return boundsMax_; }

private:
    void release();

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint instanceVbo_ = 0;
    GLsizei vertexCount_ = 0;
    GLsizei instanceCount_ = 0;
    glm::vec3 boundsMin_{0.0f};
    glm::vec3 boundsMax_{0.0f};
};

}  // namespace gfx
