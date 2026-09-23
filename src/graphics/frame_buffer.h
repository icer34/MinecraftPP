/**
 * @file frame_buffer.h
 * @brief Off-screen framebuffer with a color and a depth texture attachment.
 */

#pragma once

/**
 * @brief Off-screen render target whose color and depth are both readable as textures.
 *
 * Used to keep a copy of the opaque scene so that the water pass can sample it (refraction
 * and depth-based effects).
 */
class FrameBuffer
{
public:
    /**
     * @brief Creates the framebuffer and its two attachments.
     *
     * @param screenW width of the attachments in pixels
     * @param screenH height of the attachments in pixels
     */
    FrameBuffer(int screenW, int screenH);

    /**
     * @brief Deletes the framebuffer and its textures. Needs a live GL context.
     */
    ~FrameBuffer();

    /** @brief OpenGL name of the framebuffer object. */
    unsigned int getFrameBufferID() const { return _fboID; }
    /** @brief OpenGL name of the color attachment texture. */
    unsigned int getColorTextureID() const { return _colorTexID; }
    /** @brief OpenGL name of the depth attachment texture. */
    unsigned int getDepthTextureID() const { return _depthTexID; }

private:
    unsigned int TEXTURE_SIZE;

    unsigned int _colorTexID;
    unsigned int _depthTexID;
    unsigned int _fboID;
};