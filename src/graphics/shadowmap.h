#pragma once

#include <glad/glad.h>

class ShadowMap
{
  public:
    ShadowMap()
    {
        glGenTextures(1, &m_texID);
        glBindTexture(GL_TEXTURE_2D, m_texID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        float borderColor[4] = {1.0, 1.0, 1.0, 1.0};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_DEPTH_COMPONENT,
                     TEXTURE_SIZE,
                     TEXTURE_SIZE,
                     0,
                     GL_DEPTH_COMPONENT,
                     GL_FLOAT,
                     nullptr);

        glGenFramebuffers(1, &m_fboID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texID, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "SHADOW FBO INCOMPLETE: " << status << std::endl;

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~ShadowMap()
    {
        glDeleteTextures(1, &m_texID);
        glDeleteFramebuffers(1, &m_fboID);
    }

    unsigned int size() const { return TEXTURE_SIZE; }
    unsigned int getFrameBufferID() const { return m_fboID; }
    unsigned int getTextureID() const { return m_texID; }

  private:
    unsigned int m_texID;
    unsigned int m_fboID;
    unsigned int TEXTURE_SIZE = 6000;
};