/**
 * @file texture.h
 * @brief Owning wrapper around an OpenGL 2D texture.
 */

#pragma once

#include <glad/glad.h>
#include <stdexcept>
#include <string>
#include <string_view>

#include "stb_image.h"

#include "graphics/gl/gl_debug.h"
#include "graphics/gl/gl_objects.h"

/**
 * @brief Owns an OpenGL 2D texture. Movable but not copyable.
 *
 * Textures are created with clamp-to-edge wrapping and linear filtering (see setFilters()).
 * Must be created and destroyed while a GL context is current.
 */
class Texture
{
public:
    /**
     * @brief Creates an empty handle that owns no texture.
     */
    Texture() = default;

    /**
     * @brief Loads an image file (PNG, JPG...) from disk into an RGBA texture.
     *
     * @param fileName path to the image file
     * @throws std::runtime_error if the image cannot be loaded
     */
    explicit Texture(const std::string &fileName)
    {
        int width, height, nChannels;
        unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &nChannels, 4);
        if (!data)
        {
            throw std::runtime_error("TEXTURE::FAILED TO LOAD IMAGE DATA: " + fileName);
        }

        create(data, width, height, GL_RGBA8, GL_RGBA);
        stbi_image_free(data);
    }

    /**
     * @brief Creates a texture from an in-memory buffer (e.g. a procedurally generated image).
     *
     * Single-channel (`GL_RED`) textures are swizzled to read back as grayscale instead of red.
     *
     * @param data pixel data, 8 bits per channel, or nullptr to allocate without uploading
     * @param width width in pixels
     * @param height height in pixels
     * @param internalFormat GPU storage format (e.g. `GL_RGBA8`, `GL_R8`)
     * @param format layout of `data`; must match internalFormat (e.g. `GL_RGBA`, `GL_RED`)
     */
    Texture(const unsigned char *data, int width, int height, GLenum internalFormat, GLenum format)
    {
        create(data, width, height, internalFormat, format);
    }

    /** @brief OpenGL name of the texture. */
    unsigned int getID() const { return _tex.id(); }

    /**
     * @brief Overwrites a rectangular region of the texture (mip level 0).
     *
     * @param x left of the region, in pixels
     * @param y bottom of the region, in pixels
     * @param w width of the region, in pixels
     * @param h height of the region, in pixels
     * @param data pixel data for the region, 8 bits per channel
     * @param format layout of `data`
     */
    void addSubImage(int x, int y, int w, int h, unsigned char *data, GLenum format = GL_RGBA)
    {
        glTextureSubImage2D(_tex.id(), 0, x, y, w, h, format, GL_UNSIGNED_BYTE, data);
    }

    /**
     * @brief Names the texture, for the debug output and RenderDoc.
     */
    void setLabel(std::string_view label) { gl::setLabel(GL_TEXTURE, _tex.id(), label); }

    /**
     * @brief Sets the minification and magnification filters (e.g. `GL_NEAREST` for pixel art).
     */
    void setFilters(GLenum minFilter, GLenum magFilter)
    {
        glTextureParameteri(_tex.id(), GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(_tex.id(), GL_TEXTURE_MAG_FILTER, magFilter);
    }

private:
    GLTexture _tex;

    void create(
        const unsigned char *data, int width, int height, GLenum internalFormat, GLenum format)
    {
        _tex = gl::createTexture(GL_TEXTURE_2D);
        glTextureStorage2D(_tex.id(), 1, internalFormat, width, height);

        glTextureParameteri(_tex.id(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_tex.id(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_tex.id(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_tex.id(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // a single-channel texture defaults to (r, 0, 0, 1) when sampled -- swizzle it to
        // read back as grayscale (r, r, r, 1) instead of showing up tinted red
        if (format == GL_RED)
        {
            GLint swizzle[4] = {GL_RED, GL_RED, GL_RED, GL_ONE};
            glTextureParameteriv(_tex.id(), GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        if (data != nullptr)
            glTextureSubImage2D(_tex.id(), 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
    }
};
