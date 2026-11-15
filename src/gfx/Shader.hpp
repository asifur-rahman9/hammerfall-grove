#pragma once
// GLSL program wrapper: loads the vertex + fragment files, compiles and links them,
// and reports errors with the file name.

#include <glm/glm.hpp>
#include <string>

#include "gfx/GL.hpp"

namespace gfx {

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool valid() const { return id_ != 0; }
    GLuint id() const { return id_; }
    void use() const;

    void set(const char* name, int v) const;
    void set(const char* name, bool v) const { set(name, static_cast<int>(v)); }
    void set(const char* name, float v) const;
    void set(const char* name, const glm::vec2& v) const;
    void set(const char* name, const glm::vec3& v) const;
    void set(const char* name, const glm::vec4& v) const;
    void set(const char* name, const glm::mat3& m) const;
    void set(const char* name, const glm::mat4& m) const;
    void set(const char* name, const glm::vec3* v, int count) const;

private:
    GLint location(const char* name) const;
    void release();

    GLuint id_ = 0;
};

}  // namespace gfx
