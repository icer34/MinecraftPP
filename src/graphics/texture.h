#pragma once

#include <glad/glad.h>
#include <stdexcept>
#include <string>

#include "stb_image.h"

class Texture
{
  public:
    Texture() = default;

    // loads a PNG/JPG file from disk, always as RGBA
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

    // builds directly from an in-memory buffer (e.g. a procedurally generated image) --
    // internalFormat/format must be a matching pair (GL_RGBA8/GL_RGBA, GL_R8/GL_RED, ...)
    Texture(const unsigned char *data, int width, int height, GLenum internalFormat, GLenum format)
    {
        create(data, width, height, internalFormat, format);
    }

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    Texture(Texture &&other) noexcept
        : m_id(other.m_id)
    {
        other.m_id = 0;
    }
    Texture &operator=(Texture &&other) noexcept
    {
        if (this != &other)
        {
            glDeleteTextures(1, &m_id);
            m_id = other.m_id;
            other.m_id = 0;
        }
        return *this;
    }

    ~Texture() { glDeleteTextures(1, &m_id); }

    unsigned int getID() const { return m_id; }

  private:
    unsigned int m_id;

    void
    create(const unsigned char *data, int width, int height, GLenum internalFormat, GLenum format)
    {
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);

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
