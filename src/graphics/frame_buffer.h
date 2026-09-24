/**
 * @file frame_buffer.h
 * @brief Off-screen framebuffer with a color and a depth texture attachment.
 */

#pragma once

#include <string_view>

#include "graphics/gl/gl_objects.h"

/**
 * @brief Off-screen render target with one color and one depth texture.
 *
 * With `samples > 1`, the attachments are multisampled textures: they cannot be sampled as
 * regular textures, and must be resolved first by blitting into a single-sampled framebuffer.
 *
 * Move-only. Must be created and destroyed while a GL context is current.
 */
class FrameBuffer
{
public:
    /**
     * @brief Creates the framebuffer and its two attachments.
     *
     * @param width width of the attachments in pixels
     * @param height height of the attachments in pixels
     * @param samples samples per pixel, 1 for a regular (single-sampled) framebuffer
     * @param colorFormat internal format of the color attachment (e.g. `GL_RGBA16F`)
     * @param depthFormat internal format of the depth attachment (e.g. `GL_DEPTH_COMPONENT32F`)
     * @param label name given to the GL objects, for the debug output and RenderDoc
     */
    FrameBuffer(int width,
                int height,
                int samples,
                GLenum colorFormat,
                GLenum depthFormat,
                std::string_view label);

    /** @brief OpenGL name of the framebuffer object. */
    unsigned int getFrameBufferID() const { return _fbo.id(); }
    /** @brief OpenGL name of the color attachment texture. */
    unsigned int getColorTextureID() const { return _colorTex.id(); }
    /** @brief OpenGL name of the depth attachment texture. */
    unsigned int getDepthTextureID() const { return _depthTex.id(); }

    /** @brief Width of the attachments, in pixels. */
    int getWidth() const { return _width; }
    /** @brief Height of the attachments, in pixels. */
    int getHeight() const { return _height; }

private:
    int _width;
    int _height;
    GLTexture _colorTex;
    GLTexture _depthTex;
    GLFramebuffer _fbo;
};
