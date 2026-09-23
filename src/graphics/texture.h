/**
 * @file texture.h
 * @brief Owning wrapper around an OpenGL 2D texture.
 */

#pragma once

#include <glad/glad.h>
#include <stdexcept>
#include <string>

#include "stb_image.h"

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

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    /** @brief Takes ownership of `other`'s texture, leaving `other` empty. */
    Texture(Texture &&other) noexcept
        : _id(other._id)
    {
        other._id = 0;
    }
    /** @brief Deletes the current texture and takes ownership of `other`'s texture. */
    Texture &operator=(Texture &&other) noexcept
    {
        if (this != &other)
        {
            glDeleteTextures(1, &_id);
            _id = other._id;
            other._id = 0;
        }
        return *this;
    }

    /** @brief Deletes the texture. */
    ~Texture() { glDeleteTextures(1, &_id); }

    /** @brief OpenGL name of the texture. */
    unsigned int getID() const { return _id; }

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
        glBindTexture(GL_TEXTURE_2D, _id);
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, format, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /**
     * @brief Sets the minification and magnification filters (e.g. `GL_NEAREST` for pixel art).
     */
    void setFilters(GLenum minFilter, GLenum magFilter)
    {
        glBindTexture(GL_TEXTURE_2D, _id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

private:
    unsigned int _id = 0;

    void create(
        const unsigned char *data, int width, int height, GLenum internalFormat, GLenum format)
    {
        glGenTextures(1, &_id);
        glBindTexture(GL_TEXTURE_2D, _id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // a single-channel texture defaults to (r, 0, 0, 1) when sampled -- swizzle it to
        // read back as grayscale (r, r, r, 1) instead of showing up tinted red
        if (format == GL_RED)
        {
            GLint swizzle[4] = {GL_RED, GL_RED, GL_RED, GL_ONE};
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        }

        glTexImage2D(
            GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
};
