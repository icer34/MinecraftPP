#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <iostream>
#include <vector>

class CascadedShadowMap
{
  public:
    /**
     * @param n Number of cascades / number of detail levels of the shadow map
     */
    CascadedShadowMap()
        : m_lightVPMatrices(m_DEPTH),
          m_cutoffDist(m_DEPTH)
    {
        glGenTextures(1, &m_texID);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_texID);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        float borderColor[4] = {1.0, 1.0, 1.0, 1.0};
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

        glTexImage3D(GL_TEXTURE_2D_ARRAY,
                     0,
                     GL_DEPTH_COMPONENT,
                     m_TEXTURE_SIZE,
                     m_TEXTURE_SIZE,
                     m_DEPTH,
                     0,
                     GL_DEPTH_COMPONENT,
                     GL_FLOAT,
                     nullptr);

        glGenFramebuffers(1, &m_fboID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_texID, 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "SHADOW FBO INCOMPLETE: " << status << std::endl;

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~CascadedShadowMap()
    {
        glDeleteTextures(1, &m_texID);
        glDeleteFramebuffers(1, &m_fboID);
    }

    void updateLightVPMatrix(size_t index, const glm::mat4 &matrix)
    {
        m_lightVPMatrices[index] = matrix;
    }

    void updateCutoffDist(size_t index, float dist) { m_cutoffDist[index] = dist; }

    const glm::mat4 &getLightVPMatrix(size_t index) const { return m_lightVPMatrices.at(index); }
    const std::vector<glm::mat4> &getLightVPMatrices() const { return m_lightVPMatrices; }
    const float getCutoffDist(size_t index) const { return m_cutoffDist.at(index); }
    const std::vector<float> &getCutoffDists() const { return m_cutoffDist; }
    unsigned int size() const { return m_TEXTURE_SIZE; }
    unsigned int depth() const { return m_DEPTH; }
    unsigned int getFrameBufferID() const { return m_fboID; }
    unsigned int getTextureID() const { return m_texID; }

  private:
    unsigned int m_DEPTH = 5;
    unsigned int m_TEXTURE_SIZE = 4096;

    std::vector<glm::mat4> m_lightVPMatrices;
    std::vector<float> m_cutoffDist;

    unsigned int m_texID;
    unsigned int m_fboID;
};