#pragma once
// Texture loading via stb_image. Color textures are uploaded as sRGB so the
// GPU linearizes them on sampling (lighting happens in linear space).

#include <array>
#include <string>

#include "gfx/GL.hpp"

namespace gfx {

class Texture {
public:
    Texture() = default;
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    static Texture load2D(const std::string& path, bool srgb = true);
    // Faces in GL order: +X, -X, +Y, -Y, +Z, -Z
    static Texture loadCubemap(const std::array<std::string, 6>& faces, bool srgb = true);

    void bind(int unit) const;
    GLuint id() const { return id_; }
    GLenum target() const { return target_; }

private:
    GLuint id_ = 0;
    GLenum target_ = GL_TEXTURE_2D;
};

}  // namespace gfx
