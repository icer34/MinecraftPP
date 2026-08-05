#include "renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <implot.h>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

#include "block_texture_atlas.h"
#include "camera.h"
#include "cascaded_shadow_map.h"
#include "frame_buffer.h"
#include "game/chunk.h"
#include "game/world.h"
#include "mesh/block_outline.h"
#include "shader.h"

#include "util/frustum.h"
#include "util/perlin_noise.h"
#include "util/raycaster.h"
#include "util/window.h"

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

Renderer::Renderer(const Window &window, const World &world)
    : m_blockTintTexture(Texture("assets/textures/colormap/grass.png")),
      m_window(window),
      m_world(world),
      m_blockOutline(BlockOutline())
{
    auto &textureAtlas = BlockTextureAtlas::instance();
    textureAtlas.loadAllTextures();

    m_blockShader = std::make_unique<Shader>("shaders/block_vert.glsl", "shaders/block_frag.glsl");
    m_blockShader->use();
    m_blockShader->setVec3("lightDir", m_lightDir);

    m_waterShader = std::make_unique<Shader>("shaders/water_vert.glsl", "shaders/water_frag.glsl");
    m_waterShader->use();
    m_waterShader->setVec3("lightDir", m_lightDir);

    m_depthShader = std::make_unique<Shader>("shaders/depth_vert.glsl", "shaders/depth_frag.glsl");
    m_depthShader->addGeometryShader("shaders/depth_geom.glsl");
    m_shadowMap = std::make_unique<CascadedShadowMap>();

    m_frameBuffer = std::make_unique<FrameBuffer>(m_window.getWidth(), m_window.getHeight());

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Renderer::~Renderer() = default;

void Renderer::renderWorld(Camera &cam)
{
    cam.setAspectRatio(m_window.getAspectRatio());

    Frustum frustum = Frustum(cam);

    m_loadedChunks = m_world.getChunks().size();
    m_camPos = cam.getPos();

    //* ========== PRE PROCESSING - SHADOW PASS ==========
    m_shadowMap->update(cam, m_lightDir);

    glViewport(0, 0, m_shadowMap->size(), m_shadowMap->size());
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMap->getFrameBufferID());
    glClear(GL_DEPTH_BUFFER_BIT);
    m_depthShader->use();
    m_depthShader->setMat4Array("lightSpaceMatrices", m_shadowMap->getLightVPMatrices());
    for (auto &mesh : m_world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        m_depthShader->setMat4("model", model);
        mesh->drawSolid();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());

    //* ========== SECOND PASS - ACTUAL RENDERING ==========
    //* First draw the solid meshes
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    m_blockShader->use();
    m_blockShader->setMat4("view", cam.getViewMatrix());
    m_blockShader->setMat4("projection", cam.getProjectionMatrix());
    m_blockShader->setMat4Array("lightSpaceMatrices", m_shadowMap->getLightVPMatrices());
    m_blockShader->setFloatArray("cutoffDist", m_shadowMap->getCutoffDists());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BlockTextureAtlas::instance().getID());
    m_blockShader->setInt("atlas", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blockTintTexture.getID());
    m_blockShader->setInt("colormap", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowMap->getTextureID());
    m_blockShader->setInt("shadowMap", 2);

    m_renderedChunks = 0;
    for (auto &mesh : m_world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        if (!frustum.isChunkInside(coord))
        {
            continue;
        }

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        m_blockShader->setMat4("model", model);
        mesh->drawSolid();
        m_renderedChunks++;
    }

    //* then draw the water meshes
    // copy the solid rendering in a frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_frameBuffer->getFrameBufferID());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBlitFramebuffer(0,
                      0,
                      m_window.getWidth(),
                      m_window.getHeight(),
                      0,
                      0,
                      m_window.getWidth(),
                      m_window.getHeight(),
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                      GL_NEAREST);
    // rebind the default frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    m_waterShader->use();

    m_waterShader->setMat4("view", cam.getViewMatrix());
    m_waterShader->setMat4("projection", cam.getProjectionMatrix());
    m_waterShader->setFloat("time", m_window.getTime());
    m_waterShader->setVec3("camPos", cam.getPos());
    m_waterShader->setFloat("zNear", cam.getZNear());
    m_waterShader->setFloat("zFar", cam.getZFar());

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_frameBuffer->getColorTextureID());
    m_waterShader->setInt("solidColor", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_frameBuffer->getDepthTextureID());
    m_waterShader->setInt("solidDepth", 4);

    for (auto &mesh : m_world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        if (!frustum.isChunkInside(coord))
        {
            continue;
        }

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        m_waterShader->setMat4("model", model);
        mesh->drawWater();
    }
}

void Renderer::renderBlockOutline(const RayCastResult &result, const Camera &cam)
{
    m_blockOutline.draw(result.targetPos, cam);
}

void Renderer::beginUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Renderer::endUI()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::renderDebug(float dt)
{
    updateFPS(dt);

    //* ===== BASIC DEBUG STATS =====
    ImGui::Begin("Debug pannel");
    ImGui::Text("FPS: %.1f", m_fps);
    ImGui::Text("ms per frame: %.3f", m_msPerFrame);
    ImGui::Text("x:%.2f y:%.2f z:%.2f", m_camPos.x, m_camPos.y, m_camPos.z);
    ImGui::Text("Loaded chunks: %d", m_loadedChunks);
    ImGui::Text("Rendered chunks: %d", m_renderedChunks);

    auto &terrainGen = TerrainGenerator::instance();
    ImGui::Text("PV: %.3f", terrainGen.getPvNoise().sample(m_camPos.x, m_camPos.z));
    ImGui::Text("Erosion: %.3f", terrainGen.getErosionNoise().sample(m_camPos.x, m_camPos.z));
    ImGui::Text("Continentalness: %.3f",
                terrainGen.getContinentalnessNoise().sample(m_camPos.x, m_camPos.z));

    ImGui::End();
}

void Renderer::updateFPS(float dt)
{
    m_frameCount++;
    m_fpsTimer += dt;

    if (m_fpsTimer >= 1.0f)
    {
        m_fps = static_cast<float>(m_frameCount) / m_fpsTimer;
        m_frameCount = 0;
        m_fpsTimer -= 1.0f;
        m_msPerFrame = 1000.0f * dt;
    }
}

bool Renderer::requestWorldRegeneration()
{
    bool result = m_shouldRegenerateWorld;
    m_shouldRegenerateWorld = false;
    return result;
}
