#include "gfx/Mesh.hpp"

#include <cstddef>

namespace gfx {
namespace {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

}  // namespace

Mesh::Mesh(const ew::MeshData& data)
    : vertexCount_(static_cast<GLsizei>(data.positions.size())), boundsMin_(data.boundsMin), boundsMax_(data.boundsMax) {
    std::vector<Vertex> vertices(data.positions.size());
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i] = {data.positions[i], data.normals[i], data.uvs[i]};
    }

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, uv)));
    // Location 3 stays disabled until instances are attached; the shader then
    // reads the constant default (0,0,0,1), which non-instanced draws ignore.
    glBindVertexArray(0);
}

void Mesh::setInstances(const std::vector<glm::vec4>& instances) {
    glBindVertexArray(vao_);
    if (instanceVbo_ == 0) glGenBuffers(1, &instanceVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances.size() * sizeof(glm::vec4)), instances.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);
    glVertexAttribDivisor(3, 1);
    glBindVertexArray(0);
    instanceCount_ = static_cast<GLsizei>(instances.size());
}

void Mesh::draw() const {
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount_);
}

void Mesh::drawInstanced() const {
    if (instanceCount_ == 0) return;
    glBindVertexArray(vao_);
    glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount_, instanceCount_);
}

Mesh::~Mesh() { release(); }

Mesh::Mesh(Mesh&& o) noexcept
    : vao_(o.vao_), vbo_(o.vbo_), instanceVbo_(o.instanceVbo_), vertexCount_(o.vertexCount_), instanceCount_(o.instanceCount_),
      boundsMin_(o.boundsMin_), boundsMax_(o.boundsMax_) {
    o.vao_ = o.vbo_ = o.instanceVbo_ = 0;
}

Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this != &o) {
        release();
        vao_ = o.vao_;
        vbo_ = o.vbo_;
        instanceVbo_ = o.instanceVbo_;
        vertexCount_ = o.vertexCount_;
        instanceCount_ = o.instanceCount_;
        boundsMin_ = o.boundsMin_;
        boundsMax_ = o.boundsMax_;
        o.vao_ = o.vbo_ = o.instanceVbo_ = 0;
    }
    return *this;
}

void Mesh::release() {
    if (instanceVbo_) glDeleteBuffers(1, &instanceVbo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    vao_ = vbo_ = instanceVbo_ = 0;
}

}  // namespace gfx
