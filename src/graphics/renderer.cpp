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
    : _blockTintTexture(Texture("assets/textures/colormap/grass.png")),
      _window(window),
      _world(world),
      _blockOutline(BlockOutline())
{
    auto &textureAtlas = BlockTextureAtlas::instance();
    textureAtlas.loadAllTextures();

    _blockShader = std::make_unique<Shader>("shaders/block_vert.glsl", "shaders/block_frag.glsl");
    _blockShader->use();
    _blockShader->setVec3("lightDir", _lightDir);

    _waterShader = std::make_unique<Shader>("shaders/water_vert.glsl", "shaders/water_frag.glsl");
    _waterShader->use();
    _waterShader->setVec3("lightDir", _lightDir);

    _skyShader = std::make_unique<Shader>("shaders/sky_vert.glsl", "shaders/sky_frag.glsl");
    _skyShader->use();
    _skyShader->setVec3("lightDir", _lightDir);
    glGenVertexArrays(1, &_skyVAO);

    _depthShader = std::make_unique<Shader>("shaders/depth_vert.glsl", "shaders/depth_frag.glsl");
    _depthShader->addGeometryShader("shaders/depth_geom.glsl");
    _shadowMap = std::make_unique<CascadedShadowMap>();

    _frameBuffer = std::make_unique<FrameBuffer>(_window.getWidth(), _window.getHeight());

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
    cam.setAspectRatio(_window.getAspectRatio());

    Frustum frustum = Frustum(cam);

    _loadedChunks = _world.getChunks().size();
    _camPos = cam.getPos();

    //* ========== PRE PROCESSING - SHADOW PASS ==========
    _shadowMap->update(cam, _lightDir);

    glViewport(0, 0, _shadowMap->size(), _shadowMap->size());
    glBindFramebuffer(GL_FRAMEBUFFER, _shadowMap->getFrameBufferID());
    glClear(GL_DEPTH_BUFFER_BIT);
    _depthShader->use();
    _depthShader->setMat4Array("lightSpaceMatrices", _shadowMap->getLightVPMatrices());
    for (auto &mesh : _world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        _depthShader->setMat4("model", model);
        mesh->drawSolid();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, _window.getWidth(), _window.getHeight());

    //* ========== SECOND PASS - ACTUAL RENDERING ==========
    //* First draw the solid meshes
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    _blockShader->use();
    _blockShader->setMat4("view", cam.getViewMatrix());
    _blockShader->setMat4("projection", cam.getProjectionMatrix());
    _blockShader->setMat4Array("lightSpaceMatrices", _shadowMap->getLightVPMatrices());
    _blockShader->setFloatArray("cutoffDist", _shadowMap->getCutoffDists());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BlockTextureAtlas::instance().getID());
    _blockShader->setInt("atlas", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _blockTintTexture.getID());
    _blockShader->setInt("colormap", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _shadowMap->getTextureID());
    _blockShader->setInt("shadowMap", 2);

    _renderedChunks = 0;
    for (auto &mesh : _world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        if (!frustum.isChunkInside(coord))
        {
            continue;
        }

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        _blockShader->setMat4("model", model);
        mesh->drawSolid();
        _renderedChunks++;
    }

    // copy the solid rendering in a frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _frameBuffer->getFrameBufferID());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBlitFramebuffer(0,
                      0,
                      _window.getWidth(),
                      _window.getHeight(),
                      0,
                      0,
                      _window.getWidth(),
                      _window.getHeight(),
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                      GL_NEAREST);
    // rebind the default frame buffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    //* then draw the water meshes
    _waterShader->use();

    _waterShader->setMat4("view", cam.getViewMatrix());
    _waterShader->setMat4("projection", cam.getProjectionMatrix());
    _waterShader->setFloat("time", _window.getTime());
    _waterShader->setVec3("camPos", cam.getPos());
    _waterShader->setFloat("zNear", cam.getZNear());
    _waterShader->setFloat("zFar", cam.getZFar());

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, _frameBuffer->getColorTextureID());
    _waterShader->setInt("solidColor", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, _frameBuffer->getDepthTextureID());
    _waterShader->setInt("solidDepth", 4);

    for (auto &mesh : _world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        if (!frustum.isChunkInside(coord))
        {
            continue;
        }

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        _waterShader->setMat4("model", model);
        mesh->drawWater();
    }

    //* then render the sky
    _skyShader->use();
    _skyShader->setMat4("invProjection", glm::inverse(cam.getProjectionMatrix()));
    _skyShader->setMat4("invView", glm::inverse(cam.getViewMatrix()));
    glBindVertexArray(_skyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Renderer::renderBlockOutline(const RayCastResult &result, const Camera &cam)
{
    _blockOutline.draw(result.targetPos, cam);
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

void Renderer::renderDebug()
{
    //* ===== BASIC DEBUG STATS =====
    ImGui::Begin("Debug pannel");
    ImGui::Text("FPS: %.1f", _fps);
    ImGui::Text("ms per frame: %.3f", _msPerFrame);
    ImGui::Text("x:%.2f y:%.2f z:%.2f", _camPos.x, _camPos.y, _camPos.z);
    ImGui::Text("Loaded chunks: %d", _loadedChunks);
    ImGui::Text("Rendered chunks: %d", _renderedChunks);

    auto &terrainGen = TerrainGenerator::instance();
    ImGui::Text("PV: %.3f", terrainGen.getPvNoise().sample(_camPos.x, _camPos.z));
    ImGui::Text("Erosion: %.3f", terrainGen.getErosionNoise().sample(_camPos.x, _camPos.z));
    ImGui::Text("Continentalness: %.3f",
                terrainGen.getContinentalnessNoise().sample(_camPos.x, _camPos.z));

    ImGui::End();
}

void Renderer::updateFPS(float dt)
{
    _frameCount++;
    _fpsTimer += dt;

    if (_fpsTimer >= 1.0f)
    {
        _fps = static_cast<float>(_frameCount) / _fpsTimer;
        _frameCount = 0;
        _fpsTimer -= 1.0f;
        // average over the same window as _fps, not just the last frame of it
        _msPerFrame = 1000.0f / _fps;
    }
}

bool Renderer::requestWorldRegeneration()
{
    bool result = _shouldRegenerateWorld;
    _shouldRegenerateWorld = false;
    return result;
}
