#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <iostream>
#include <vector>

#include "camera.h"

class CascadedShadowMap
{
public:
    CascadedShadowMap();
    ~CascadedShadowMap();

    void update(const Camera &cam, const glm::vec3 &lightDir);

    const std::vector<glm::mat4> &getLightVPMatrices() const { return m_lightVPMatrices; }
    const std::vector<float> &getCutoffDists() const { return m_cutoffDist; }

    unsigned int size() const { return m_TEXTURE_SIZE; }
    unsigned int depth() const { return m_DEPTH; }
    unsigned int getFrameBufferID() const { return m_fboID; }
    unsigned int getTextureID() const { return m_texID; }

private:
    glm::mat4
    getLightVPMatrix(const Camera &cam, float zNear, float zFar, const glm::vec3 &lightDir);

    unsigned int m_DEPTH = 5;
    unsigned int m_TEXTURE_SIZE = 4096;

    std::vector<glm::mat4> m_lightVPMatrices;
    std::vector<float> m_cutoffDist;

    unsigned int m_texID;
    unsigned int m_fboID;
};