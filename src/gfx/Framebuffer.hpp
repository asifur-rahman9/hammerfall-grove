#pragma once
// Offscreen render target. Used for the shadow map.

#include "gfx/GL.hpp"

namespace gfx {

class Framebuffer {
public:
    enum class Kind {
        Depth,  // depth texture only (shadow map)
    };

    Framebuffer() = default;
    ~Framebuffer();
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    // (Re)creates the attachments. Does nothing if the size and kind are unchanged.
    void create(Kind kind, int width, int height);
    void bind() const;  // binds and sets the viewport

    GLuint fbo() const { return fbo_; }
    GLuint texture() const { return texture_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    void release();

    Kind kind_ = Kind::Depth;
    GLuint fbo_ = 0;
    GLuint texture_ = 0;
    int width_ = 0;
    int height_ = 0;
};

}  // namespace gfx
