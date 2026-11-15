#include "gfx/Shader.hpp"

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace gfx {
namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open shader file: " + path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compileStage(GLenum stage, const std::string& source, const std::string& label) {
    const GLuint shader = glCreateShader(stage);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<std::size_t>(len > 1 ? len : 1));
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile error in " + label + ":\n" + log.data());
    }
    return shader;
}

}  // namespace

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
    const GLuint vs = compileStage(GL_VERTEX_SHADER, readFile(vertexPath), vertexPath);
    GLuint fs = 0;
    try {
        fs = compileStage(GL_FRAGMENT_SHADER, readFile(fragmentPath), fragmentPath);
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }
    id_ = glCreateProgram();
    glAttachShader(id_, vs);
    glAttachShader(id_, fs);
    glLinkProgram(id_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<std::size_t>(len > 1 ? len : 1));
        glGetProgramInfoLog(id_, len, nullptr, log.data());
        release();
        throw std::runtime_error("Shader link error (" + vertexPath + " + " + fragmentPath + "):\n" + log.data());
    }
}

Shader::~Shader() { release(); }

Shader::Shader(Shader&& other) noexcept : id_(other.id_) { other.id_ = 0; }

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        release();
        id_ = other.id_;
        other.id_ = 0;
    }
    return *this;
}

void Shader::release() {
    if (id_ != 0) glDeleteProgram(id_);
    id_ = 0;
}

void Shader::use() const { glUseProgram(id_); }

GLint Shader::location(const char* name) const { return glGetUniformLocation(id_, name); }

void Shader::set(const char* name, int v) const { glUniform1i(location(name), v); }
void Shader::set(const char* name, float v) const { glUniform1f(location(name), v); }
void Shader::set(const char* name, const glm::vec2& v) const { glUniform2fv(location(name), 1, glm::value_ptr(v)); }
void Shader::set(const char* name, const glm::vec3& v) const { glUniform3fv(location(name), 1, glm::value_ptr(v)); }
void Shader::set(const char* name, const glm::vec4& v) const { glUniform4fv(location(name), 1, glm::value_ptr(v)); }
void Shader::set(const char* name, const glm::mat3& m) const { glUniformMatrix3fv(location(name), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::set(const char* name, const glm::mat4& m) const { glUniformMatrix4fv(location(name), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::set(const char* name, const glm::vec3* v, int count) const {
    if (count > 0) glUniform3fv(location(name), count, glm::value_ptr(v[0]));
}

}  // namespace gfx
