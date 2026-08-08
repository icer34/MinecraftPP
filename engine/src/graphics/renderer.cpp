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
#include "core/chunk.h"
#include "core/world.h"
#include "frame_buffer.h"
#include "mesh/block_outline.h"
#include "mesh/chunk_mesh.h"
#include "shader.h"
#include "uniform_manager.h"

#include "util/frustum.h"
#include "util/perlin_noise.h"
#include "util/raycaster.h"
#include "util/window.h"

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

Renderer::Renderer(const Window &window, const World &world)
    : m_window(window),
      m_blockTintTexture(Texture("game/assets/textures/colormap/grass.png")),
      m_world(world),
      m_blockOutline(BlockOutline())
{
    auto &textureAtlas = BlockTextureAtlas::instance();
    textureAtlas.loadAllTextures();

    auto &uniforms = UniformManager::instance();
    uniforms.setValue("lightDir", m_lightDir);

    m_blockShader.load("block");
    m_waterShader.load("water");
    m_skyShader.load("sky");
    m_depthShader.load("depth");

    glGenVertexArrays(1, &m_skyVAO);

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

    auto &uniforms = UniformManager::instance();

    //* ========== PRE PROCESSING - SHADOW PASS ==========
    m_shadowMap->update(cam, m_lightDir);
    uniforms.setValue("lightSpaceMatrices", m_shadowMap->getLightVPMatrices());
    uniforms.setValue("cutoffDist", m_shadowMap->getCutoffDists());

    glViewport(0, 0, m_shadowMap->size(), m_shadowMap->size());
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMap->getFrameBufferID());
    glClear(GL_DEPTH_BUFFER_BIT);
    m_depthShader.bind();
    for (auto &mesh : m_world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        uniforms.setValue("chunkModel", model);
        uniforms.applyTo(m_depthShader);
        mesh->drawSolid();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());

    //* ========== SECOND PASS - ACTUAL RENDERING ==========
    //* First draw the solid meshes
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    uniforms.setValue("view", cam.getViewMatrix());
    uniforms.setValue("perspProj", cam.getProjectionMatrix());

    m_blockShader.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BlockTextureAtlas::instance().getID());
    uniforms.setValue("blockAtlas", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blockTintTexture.getID());
    uniforms.setValue("colormap", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_shadowMap->getTextureID());
    uniforms.setValue("shadowMap", 2);

    m_renderedChunks = 0;
    for (auto &mesh : m_world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        if (!frustum.isChunkInside(coord))
        {
            continue;
        }

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        uniforms.setValue("chunkModel", model);
        uniforms.applyTo(m_blockShader);
        mesh->drawSolid();
        m_renderedChunks++;
    }

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

    //* then draw the water meshes
    m_waterShader.bind();

    uniforms.setValue("time", m_window.getTime());
    uniforms.setValue("camPos", cam.getPos());
    uniforms.setValue("zNear", cam.getZNear());
    uniforms.setValue("zFar", cam.getZFar());

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_frameBuffer->getColorTextureID());
    uniforms.setValue("solidColor", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_frameBuffer->getDepthTextureID());
    uniforms.setValue("solidDepth", 4);

    for (auto &mesh : m_world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        if (!frustum.isChunkInside(coord))
        {
            continue;
        }

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        uniforms.setValue("chunkModel", model);
        uniforms.applyTo(m_waterShader);
        mesh->drawWater();
    }

    //* then render the sky
    uniforms.setValue("invPerspProj", glm::inverse(cam.getProjectionMatrix()));
    uniforms.setValue("invView", glm::inverse(cam.getViewMatrix()));

    m_skyShader.bind();
    uniforms.applyTo(m_skyShader);
    glBindVertexArray(m_skyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
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

    // PV/Erosion/Continentalness debug lines removed here -- they depended directly on
    // TerrainGenerator (game-side), which engine code can no longer see. Bring them back
    // via the generic DebugPanel stat registry noted in TODO.md instead of a direct call.

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
