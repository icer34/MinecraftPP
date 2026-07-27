#include "cascaded_shadow_map.h"

#include <glm/gtc/matrix_transform.hpp>
using glm::mat4;
using glm::vec3;
using glm::vec4;

CascadedShadowMap::CascadedShadowMap()
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

CascadedShadowMap::~CascadedShadowMap()
{
    glDeleteTextures(1, &m_texID);
    glDeleteFramebuffers(1, &m_fboID);
}

void CascadedShadowMap::update(const Camera &cam, float aspectRatio, const glm::vec3 &lightDir)
{
    float near = cam.getZNear();
    float far = cam.getZFar();

    constexpr float lambda = 0.8f;
    std::vector<float> splits(m_DEPTH + 1);
    splits[0] = near;
    for (size_t i = 1; i <= m_DEPTH; i++)
    {
        float p = float(i) / float(m_DEPTH);
        float logSplit = near * std::pow(far / near, p);
        float uniformSplit = near + (far - near) * p;
        splits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
    }

    for (size_t i = 0; i < m_DEPTH; i++)
    {
        float splitNear = splits[i];
        float splitFar = splits[i + 1];
        m_cutoffDist[i] = splitFar;

        // compute the light VP matrix for that cascade
        m_lightVPMatrices[i]
            = getLightVPMatrix(cam, aspectRatio, splitNear, splitFar * 1.1f, lightDir);
    }
}

mat4 CascadedShadowMap::getLightVPMatrix(
    const Camera &cam, float aspectRatio, float zNear, float zFar, const glm::vec3 &lightDir)
{
    auto camProj = glm::perspective(glm::radians(cam.getFOV()), aspectRatio, zNear, zFar);

    auto inv = glm::inverse(camProj * cam.getViewMatrix());

    std::vector<vec4> frustumCornersWorld;
    for (int x = 0; x < 2; x++)
    {
        for (int y = 0; y < 2; y++)
        {
            for (int z = 0; z < 2; z++)
            {
                vec4 point = inv * vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                frustumCornersWorld.push_back(point / point.w);
            }
        }
    }

    // get the center of the frustum for the viewMatrix center
    vec3 center = vec3(0.0, 0.0, 0.0);
    for (const auto &corner : frustumCornersWorld)
    {
        center += vec3(corner);
    }
    center /= frustumCornersWorld.size();

    // get the distance to the farthest corner from the center
    float radius = 0.0f;
    for (const auto &corner : frustumCornersWorld)
    {
        float distance = glm::distance(vec3(corner), center);
        radius = distance > radius ? distance : radius;
    }

    // get the lightView matrix
    vec3 lightRight = glm::normalize(glm::cross(vec3(0.0f, 1.0f, 0.0f), lightDir));
    vec3 lightUp = glm::normalize(glm::cross(lightDir, lightRight));
    // first we snap the center onto the texel grid
    float texelSize = (radius * 2) / m_TEXTURE_SIZE;
    float projRight = glm::dot(center, lightRight);
    float projUp = glm::dot(center, lightUp);
    // round to the closest multiple of texelSize
    float projRightRound = texelSize * std::round(projRight / texelSize);
    float projUpRound = texelSize * std::round(projUp / texelSize);

    float deltaRight = projRightRound - projRight;
    float deltaUp = projUpRound - projUp;

    // shift the center so now it can only move by jumps of at least texelSize
    center += deltaRight * lightRight + deltaUp * lightUp;

    mat4 lightView = glm::lookAt(center - lightDir, center, vec3(0.0f, 1.0f, 0.0f));

    // the CSM geometry is a sphere that englobes the frustum of it's repective lvl so that the
    // shadows are not recalculated when rotating the camera, only when the position moves.
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto &corner : frustumCornersWorld)
    {
        const auto transfo = lightView * corner;
        minZ = std::min(minZ, transfo.z);
        maxZ = std::max(maxZ, transfo.z);
    }

    // multiply near/far by a constant factor to make the boxes overlap
    constexpr float zMult = 10.0f;
    if (minZ < 0)
    {
        minZ *= zMult;
    }
    else
    {
        minZ /= zMult;
    }
    if (maxZ < 0)
    {
        maxZ /= zMult;
    }
    else
    {
        maxZ *= zMult;
    }

    mat4 lightProjection = glm::ortho(-radius, radius, -radius, radius, minZ, maxZ);

    return lightProjection * lightView;
}