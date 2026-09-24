#include "frame_buffer.h"

#include <glad/glad.h>

#include <iostream>
#include <string>

#include "graphics/gl/gl_debug.h"

namespace
{
GLTexture createAttachment(int width, int height, int samples, GLenum format)
{
    if (samples > 1)
    {
        GLTexture tex = gl::createTexture(GL_TEXTURE_2D_MULTISAMPLE);
        // fixed sample locations: the same pattern for color and depth, as a multisampled
        // framebuffer requires
        glTextureStorage2DMultisample(tex.id(), samples, format, width, height, GL_TRUE);
        return tex;
    }

    // multisampled textures have no sampler state: only regular ones get filtering/wrapping
    GLTexture tex = gl::createTexture(GL_TEXTURE_2D);
    glTextureStorage2D(tex.id(), 1, format, width, height);
    glTextureParameteri(tex.id(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex.id(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex.id(), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(tex.id(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return tex;
}
} // namespace

FrameBuffer::FrameBuffer(int width,
                         int height,
                         int samples,
                         GLenum colorFormat,
                         GLenum depthFormat,
                         std::string_view label)
    : _width(width),
      _height(height),
      _colorTex(createAttachment(width, height, samples, colorFormat)),
      _depthTex(createAttachment(width, height, samples, depthFormat)),
      _fbo(gl::createFramebuffer())
{
    glNamedFramebufferTexture(_fbo.id(), GL_COLOR_ATTACHMENT0, _colorTex.id(), 0);
    glNamedFramebufferTexture(_fbo.id(), GL_DEPTH_ATTACHMENT, _depthTex.id(), 0);

    std::string name(label);
    gl::setLabel(GL_FRAMEBUFFER, _fbo.id(), name);
    gl::setLabel(GL_TEXTURE, _colorTex.id(), name + " color");
    gl::setLabel(GL_TEXTURE, _depthTex.id(), name + " depth");

    GLenum status = glCheckNamedFramebufferStatus(_fbo.id(), GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << name << " framebuffer incomplete: 0x" << std::hex << status << std::dec
                  << '\n';
}
