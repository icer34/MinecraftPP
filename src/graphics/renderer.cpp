#include "renderer.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "game/chunk.h"
#include "game/world.h"
#include "shader.h"
#include "texture_atlas.h"

Renderer::Renderer()
{
    auto &textureAtlas = TextureAtlas::instance();
    textureAtlas.loadAllTextures();

    m_shader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Renderer::~Renderer() = default;

void Renderer::renderWorld(const World &world, const Camera &cam)
{
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    m_shader->use();
    m_shader->setMat4("view", cam.getViewMatrix());
    m_shader->setMat4("projection", cam.getProjectionMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, TextureAtlas::instance().getID());
    m_shader->setInt("atlas", 0);

    for (auto &mesh : world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();
        glm::mat4 model =
            glm::translate(glm::mat4(1.0f), glm::vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        m_shader->setMat4("model", model);
        mesh->draw();
    }
}
