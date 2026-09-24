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
#include "frame_data.h"
#include "game/chunk.h"
#include "game/world.h"
#include "gl/gl_debug.h"
#include "gl/texture_units.h"
#include "mesh/block_outline.h"
#include "mesh/chunk_mesh.h"
#include "shader.h"

#include "util/frustum.h"
#include "util/perlin_noise.h"
#include "util/raycaster.h"
#include "util/window.h"

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

namespace
{
// layout(location = 0) of chunkOffset in block_vert, water_vert and depth_vert
constexpr GLint CHUNK_OFFSET_LOCATION = 0;

constexpr int SCENE_SAMPLES = 4;
constexpr GLenum SCENE_COLOR_FORMAT = GL_RGBA16F;
// floating point depth: required by reverse-Z to spread the precision over the distance
constexpr GLenum SCENE_DEPTH_FORMAT = GL_DEPTH_COMPONENT32F;

constexpr float CLEAR_COLOR[4] = {0.2f, 0.2f, 0.2f, 1.0f};
constexpr float REVERSE_Z_FAR_DEPTH = 0.0f;
constexpr float SHADOW_FAR_DEPTH = 1.0f;

vec3 chunkOffset(const ChunkMesh &mesh)
{
    ChunkCoord coord = mesh.getCoords();
    return vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE);
}
} // namespace

Renderer::Renderer(const Window &window, const World &world, const BlockTextureAtlas &blockAtlas)
    : _window(window),
      _world(world),
      _blockAtlas(blockAtlas),
      _blockShader(std::make_unique<Shader>("shaders/block_vert.glsl", "shaders/block_frag.glsl")),
      _depthShader(std::make_unique<Shader>(
          "shaders/depth_vert.glsl", "shaders/depth_frag.glsl", "shaders/depth_geom.glsl")),
      _waterShader(std::make_unique<Shader>("shaders/water_vert.glsl", "shaders/water_frag.glsl")),
      _skyShader(std::make_unique<Shader>("shaders/sky_vert.glsl", "shaders/sky_frag.glsl")),
      _shadowMap(std::make_unique<CascadedShadowMap>()),
      _frameData(std::make_unique<FrameDataBuffer>()),
      _blockTintTexture(Texture("assets/textures/colormap/grass.png")),
      _chunkVao(Mesh::createVertexArray()),
      _skyVao(gl::createVertexArray())
{
    _blockTintTexture.setLabel("Grass colormap");
    gl::setLabel(GL_VERTEX_ARRAY, _skyVao.id(), "Sky (empty)");

    updateFramebufferSize();

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Renderer::~Renderer() = default;

void Renderer::updateFramebufferSize()
{
    int w = _window.getWidth();
    int h = _window.getHeight();
    if (_sceneFbo && _sceneFbo->getWidth() == w && _sceneFbo->getHeight() == h)
        return;

    _sceneFbo = std::make_unique<FrameBuffer>(
        w, h, SCENE_SAMPLES, SCENE_COLOR_FORMAT, SCENE_DEPTH_FORMAT, "Scene");
    // same formats as the scene: blitting depth requires identical depth formats
    _resolvedFbo = std::make_unique<FrameBuffer>(
        w, h, 1, SCENE_COLOR_FORMAT, SCENE_DEPTH_FORMAT, "Resolved scene");
}

void Renderer::uploadFrameData(const Camera &cam)
{
    FrameData data{};
    data.view = cam.getViewMatrix();
    data.projection = cam.getProjectionMatrix();
    data.invView = glm::inverse(data.view);
    data.invProjection = glm::inverse(data.projection);

    auto lightMatrices = _shadowMap->getLightVPMatrices();
    std::copy(lightMatrices.begin(), lightMatrices.end(), data.lightSpaceMatrices);
    auto cutoffs = _shadowMap->getCutoffDists();
    for (size_t i = 0; i < cutoffs.size(); i++)
        data.cutoffDist[i / 4][i % 4] = cutoffs[i];

    data.lightDir = _lightDir;
    data.time = _window.getTime();
    data.camPos = cam.getPos();
    data.zNear = cam.getZNear();
    data.screenSize = vec2(_window.getWidth(), _window.getHeight());
    data.zFar = cam.getZFar();

    _frameData->upload(data);
}

void Renderer::renderWorld(Camera &cam)
{
    cam.setAspectRatio(_window.getAspectRatio());
    updateFramebufferSize();

    const int width = _window.getWidth();
    const int height = _window.getHeight();

    _loadedChunks = _world.getChunks().size();
    _camPos = cam.getPos();

    _shadowMap->update(cam, _lightDir);
    uploadFrameData(cam);

    std::vector<ChunkMesh *> meshes = _world.getChunkMeshes();

    Frustum frustum = Frustum(cam);
    _visibleMeshes.clear();
    for (ChunkMesh *mesh : meshes)
    {
        if (frustum.isChunkInside(mesh->getCoords()))
            _visibleMeshes.push_back(mesh);
    }
    _renderedChunks = static_cast<int>(_visibleMeshes.size());

    //* ========== PRE PROCESSING - SHADOW PASS ==========
    {
        gl::DebugGroup group("Shadow pass");

        GLuint shadowFbo = _shadowMap->getFrameBufferID();
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
        glViewport(0, 0, CascadedShadowMap::TEXTURE_SIZE, CascadedShadowMap::TEXTURE_SIZE);
        // the shadow maps use a regular depth, not reverse-Z
        glClearNamedFramebufferfv(shadowFbo, GL_DEPTH, 0, &SHADOW_FAR_DEPTH);
        glDepthFunc(GL_LESS);

        _depthShader->use();
        for (ChunkMesh *mesh : meshes)
        {
            _depthShader->setVec3(CHUNK_OFFSET_LOCATION, chunkOffset(*mesh));
            mesh->drawSolid(_chunkVao);
        }
    }

    //* ========== SECOND PASS - ACTUAL RENDERING ==========
    GLuint sceneFbo = _sceneFbo->getFrameBufferID();
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    glViewport(0, 0, width, height);
    glClearNamedFramebufferfv(sceneFbo, GL_COLOR, 0, CLEAR_COLOR);
    glClearNamedFramebufferfv(sceneFbo, GL_DEPTH, 0, &REVERSE_Z_FAR_DEPTH);
    // reverse-Z: closer is greater. GEQUAL rather than GREATER so that the sky, drawn at the
    // far depth, still passes where nothing else was drawn
    glDepthFunc(GL_GEQUAL);

    //* First draw the solid meshes
    {
        gl::DebugGroup group("Opaque pass");

        glBindTextureUnit(TextureUnit::BLOCK_ATLAS, _blockAtlas.getID());
        glBindTextureUnit(TextureUnit::BLOCK_COLORMAP, _blockTintTexture.getID());
        glBindTextureUnit(TextureUnit::SHADOW_MAP, _shadowMap->getTextureID());

        _blockShader->use();
        for (ChunkMesh *mesh : _visibleMeshes)
        {
            _blockShader->setVec3(CHUNK_OFFSET_LOCATION, chunkOffset(*mesh));
            mesh->drawSolid(_chunkVao);
        }
    }

    // copy the solid rendering for the water pass -- the blit also resolves the multisampling
    {
        gl::DebugGroup group("Opaque copy");
        glBlitNamedFramebuffer(sceneFbo,
                               _resolvedFbo->getFrameBufferID(),
                               0,
                               0,
                               width,
                               height,
                               0,
                               0,
                               width,
                               height,
                               GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                               GL_NEAREST);
    }

    //* then draw the water meshes
    {
        gl::DebugGroup group("Water pass");

        glBindTextureUnit(TextureUnit::SOLID_COLOR, _resolvedFbo->getColorTextureID());
        glBindTextureUnit(TextureUnit::SOLID_DEPTH, _resolvedFbo->getDepthTextureID());

        _waterShader->use();
        for (ChunkMesh *mesh : _visibleMeshes)
        {
            _waterShader->setVec3(CHUNK_OFFSET_LOCATION, chunkOffset(*mesh));
            mesh->drawWater(_chunkVao);
        }
    }

    //* then render the sky
    {
        gl::DebugGroup group("Sky");
        _skyShader->use();
        glBindVertexArray(_skyVao.id());
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

void Renderer::renderBlockOutline(const RayCastResult &result)
{
    gl::DebugGroup group("Block outline");
    glBindFramebuffer(GL_FRAMEBUFFER, _sceneFbo->getFrameBufferID());
    _blockOutline.draw(result.targetPos);
}

void Renderer::presentScene()
{
    gl::DebugGroup group("Present");

    const int width = _window.getWidth();
    const int height = _window.getHeight();

    // resolve the multisampling first: a multisampled blit into a framebuffer of another
    // format (the RGBA8 screen) is not allowed
    glBlitNamedFramebuffer(_sceneFbo->getFrameBufferID(),
                           _resolvedFbo->getFrameBufferID(),
                           0,
                           0,
                           width,
                           height,
                           0,
                           0,
                           width,
                           height,
                           GL_COLOR_BUFFER_BIT,
                           GL_NEAREST);
    // bind the default framebuffer BEFORE blitting into it, even though the blit is DSA: after a
    // buffer swap, Mesa only fetches the window's new back buffer when framebuffer 0 gets bound.
    // Blitting first sends the scene into a stale buffer, with no GL error, and the HUD then
    // gets drawn over an old frame (it flickers).
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 0 = the default framebuffer. The HDR values above 1 are clamped here: this is where a
    // tone mapping pass will go
    glBlitNamedFramebuffer(_resolvedFbo->getFrameBufferID(),
                           0,
                           0,
                           0,
                           width,
                           height,
                           0,
                           0,
                           width,
                           height,
                           GL_COLOR_BUFFER_BIT,
                           GL_NEAREST);
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
