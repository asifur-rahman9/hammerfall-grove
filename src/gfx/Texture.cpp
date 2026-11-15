#include "gfx/Texture.hpp"

#include <stb_image.h>

#include <stdexcept>

namespace gfx {
namespace {

void formatsFor(int channels, bool srgb, GLenum& internalFormat, GLenum& format) {
    switch (channels) {
        case 1: internalFormat = GL_R8; format = GL_RED; break;
        case 2: internalFormat = GL_RG8; format = GL_RG; break;
        case 3: internalFormat = srgb ? GL_SRGB8 : GL_RGB8; format = GL_RGB; break;
        default: internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8; format = GL_RGBA; break;
    }
}

}  // namespace

Texture Texture::load2D(const std::string& path, bool srgb) {
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 0);
    if (!pixels) throw std::runtime_error("Failed to load texture '" + path + "': " + stbi_failure_reason());

    Texture t;
    t.target_ = GL_TEXTURE_2D;
    glGenTextures(1, &t.id_);
    glBindTexture(GL_TEXTURE_2D, t.id_);
    GLenum internalFormat, format;
    formatsFor(channels, srgb, internalFormat, format);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat), w, h, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // MAG with mipmaps is invalid (original bug)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLfloat maxAniso = 0.0f;
    if (GLEW_EXT_texture_filter_anisotropic) {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso < 8.0f ? maxAniso : 8.0f);
    }
    stbi_image_free(pixels);
    return t;
}

Texture Texture::loadCubemap(const std::array<std::string, 6>& faces, bool srgb) {
    Texture t;
    t.target_ = GL_TEXTURE_CUBE_MAP;
    glGenTextures(1, &t.id_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, t.id_);
    for (int i = 0; i < 6; ++i) {
        int w = 0, h = 0, channels = 0;
        unsigned char* pixels = stbi_load(faces[i].c_str(), &w, &h, &channels, 0);
        if (!pixels) throw std::runtime_error("Failed to load cubemap face '" + faces[i] + "'");
        GLenum internalFormat, format;
        formatsFor(channels, srgb, internalFormat, format);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<GLenum>(i), 0, static_cast<GLint>(internalFormat), w, h, 0, format,
                     GL_UNSIGNED_BYTE, pixels);
        stbi_image_free(pixels);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return t;
}

void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(target_, id_);
}

Texture::~Texture() {
    if (id_) glDeleteTextures(1, &id_);
}

Texture::Texture(Texture&& o) noexcept : id_(o.id_), target_(o.target_) { o.id_ = 0; }

Texture& Texture::operator=(Texture&& o) noexcept {
    if (this != &o) {
        if (id_) glDeleteTextures(1, &id_);
        id_ = o.id_;
        target_ = o.target_;
        o.id_ = 0;
    }
    return *this;
}

}  // namespace gfx
