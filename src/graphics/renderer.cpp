#include "renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <implot.h>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

#include "block_texture_atlas.h"
#include "camera.h"
#include "cascaded_shadow_map.h"
#include "game/chunk.h"
#include "game/world.h"
#include "shader.h"

#include "util/frustum.h"
#include "util/perlin_noise.h"
#include "util/window.h"

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

namespace
{
void helpMarker(const char *desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void plotSpline(Spline &spline, const char *label, ImVec2 size, const char *description = nullptr)
{
    vec2 xBounds = spline.getXBounds();
    vec2 yBounds = spline.getYBounds();

    std::string childLabel = std::string(label) + "##spline";

    ImGui::BeginChild(childLabel.c_str(), ImVec2(size.x, size.y + 70));

    ImGui::Text("%s", label);
    if (description)
    {
        ImGui::SameLine();
        helpMarker(description);
    }

    if (ImPlot::BeginPlot("##plot", size, ImPlotFlags_NoLegend))
    {
        ImPlot::SetupAxis(ImAxis_X1, "x", ImPlotAxisFlags_NoLabel);
        ImPlot::SetupAxis(ImAxis_Y1, "y ", ImPlotAxisFlags_NoLabel);
        ImPlot::SetupAxesLimits(
            xBounds.x * 1.1, xBounds.y * 1.1, yBounds.x, yBounds.y * 1.1, ImPlotCond_Always);

        const auto &xs = spline.getXValues();
        const auto &ys = spline.getYValues();

        ImPlot::PlotLine("curve", xs.data(), ys.data(), static_cast<int>(xs.size()));

        int draggedIndex = -1;
        float draggedX = 0.0f, draggedY = 0.0f;

        for (size_t i = 0; i < xs.size(); i++)
        {
            double x = xs[i];
            double y = ys[i];

            if (ImPlot::DragPoint(static_cast<int>(i), &x, &y, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)))
            {
                draggedIndex = static_cast<int>(i);
                draggedX = static_cast<float>(x);
                draggedY = static_cast<float>(y);
            }
        }

        // apply after the loop: setPoint() may re-sort the spline's internal vectors,
        // which would invalidate xs/ys (still referenced above) if done mid-loop
        if (draggedIndex >= 0)
        {
            spline.setPoint(static_cast<size_t>(draggedIndex), draggedX, draggedY);
        }

        ImPlot::EndPlot();
    }

    static float newPoint[2] = {0.0f, 0.0f};
    if (ImGui::Button("Add"))
    {
        spline.addPoint(newPoint[0], newPoint[1]);
        newPoint[0] = 0.0f;
        newPoint[1] = 0.0f;
    }
    ImGui::SameLine();
    ImGui::InputFloat2("new point", newPoint, "%.2f");

    static int toRemove = 0;
    if (ImGui::Button("Remove") && toRemove > 0)
    {
        spline.removePoint(toRemove);
        toRemove = 0;
    }
    ImGui::SameLine();
    ImGui::InputInt("remove point", &toRemove, 0);

    ImGui::EndChild();
}

mat4 getLightVPMatrix(const vec3 &lightDir,
                      const mat4 &camView,
                      const float fov,
                      const float aspectRatio,
                      float zNear,
                      float zFar)
{
    auto camProj = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);

    auto inv = glm::inverse(camProj * camView);

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

    // get the lightView matrix
    mat4 lightView = glm::lookAt(center - lightDir, center, vec3(0.0f, 1.0f, 0.0f));

    // get the limits of the box in the light space to get the left/right top/bottom near/far args
    // for the orthogonal projection
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto &corner : frustumCornersWorld)
    {
        const auto transfo = lightView * corner;
        minX = std::min(minX, transfo.x);
        maxX = std::max(maxX, transfo.x);
        minY = std::min(minY, transfo.y);
        maxY = std::max(maxY, transfo.y);
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

    mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

    return lightProjection * lightView;
}
} // namespace

Renderer::Renderer(const Window &window, const World &world)
    : m_blockTintTexture(Texture("assets/textures/colormap/grass.png")),
      m_window(window),
      m_world(world)
{
    auto &textureAtlas = BlockTextureAtlas::instance();
    textureAtlas.loadAllTextures();

    m_blockShader = std::make_unique<Shader>("shaders/block_vert.glsl", "shaders/block_frag.glsl");
    m_blockShader->use();
    m_blockShader->setVec3("lightDir", m_lightDir);

    m_depthShader = std::make_unique<Shader>("shaders/depth_vert.glsl", "shaders/depth_frag.glsl");
    m_depthShader->addGeometryShader("shaders/depth_geom.glsl");
    m_shadowMap = std::make_unique<CascadedShadowMap>();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //* ===== LOAD TERRAIN GEN SETTINGS =====
    // get the noise textures
    auto &terrainGen = TerrainGenerator::instance();
    std::array<PerlinNoise, 5> noises{terrainGen.getContinentalnessNoise(),
                                      terrainGen.getErosionNoise(),
                                      terrainGen.getPvNoise(),
                                      terrainGen.getTemperatureNoise(),
                                      terrainGen.getHumidityNoise()};

    std::vector<unsigned char> buffer(256 * 256);
    for (size_t i = 0; i < 5; i++)
    {
        auto noise = noises.at(i);
        noise.generateImage(buffer.data(), 256, 256, 1024);
        m_noiseTextures.push_back(Texture(buffer.data(), 256, 256, GL_R8, GL_RED));
        buffer.clear();
    }

    // get the splines
    m_terrainGenSplines.push_back(&terrainGen.getContinentalnessSpline());
    m_terrainGenSplines.push_back(&terrainGen.getErosionSpline());
    m_terrainGenSplines.push_back(&terrainGen.getPvSpline());
}

Renderer::~Renderer() = default;

void Renderer::renderWorld(const Camera &cam)
{
    Frustum frustum = Frustum(cam, m_window.getAspectRatio());

    m_loadedChunks = m_world.getChunks().size();
    m_camPos = cam.getPos();

    //* ========== FIRST PASS - SHADOW PASS ==========
    float near = cam.getZNear();
    float far = cam.getZFar();
    size_t cascadeCount = m_shadowMap->depth();

    // practical split scheme: blend of logarithmic and uniform splits. Pure uniform wastes
    // precision on the near cascade (which needs it most -- nearby geometry covers far more
    // screen space than distant geometry). Pure logarithmic gives razor-thin, imprecise far
    // cascades. lambda blends the two; 0.5 is a common middle ground.
    constexpr float lambda = 0.7f;
    std::vector<float> splits(cascadeCount + 1);
    splits[0] = near;
    for (size_t i = 1; i <= cascadeCount; i++)
    {
        float p = float(i) / float(cascadeCount);
        float logSplit = near * std::pow(far / near, p);
        float uniformSplit = near + (far - near) * p;
        splits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
    }

    for (size_t i = 0; i < cascadeCount; i++)
    {
        float splitNear = splits[i];
        float splitFar = splits[i + 1];
        m_shadowMap->updateCutoffDist(i, splitFar);

        // compute the light VP matrix for that cascade
        mat4 lightVPMatrix = getLightVPMatrix(m_lightDir,
                                              cam.getViewMatrix(),
                                              cam.getFOV(),
                                              m_window.getAspectRatio(),
                                              splitNear,
                                              splitFar * 1.1f);
        m_shadowMap->updateLightVPMatrix(i, lightVPMatrix);
    }

    glViewport(0, 0, m_shadowMap->size(), m_shadowMap->size());
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMap->getFrameBufferID());
    glClear(GL_DEPTH_BUFFER_BIT);
    m_depthShader->use();
    m_depthShader->setMat4Array("lightSpaceMatrices", m_shadowMap->getLightVPMatrices());
    for (auto &mesh : m_world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();

        if (!frustum.isChunkInside(coord))
            continue;

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        m_depthShader->setMat4("model", model);
        mesh->draw();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_window.getWidth(), m_window.getHeight());

    //* ========== SECOND PASS - ACTUAL RENDERING ==========
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    m_blockShader->use();
    m_blockShader->setMat4("view", cam.getViewMatrix());
    m_blockShader->setMat4("projection", cam.getProjectionMatrix(m_window.getAspectRatio()));
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
            continue;

        mat4 model = glm::translate(mat4(1.0f), vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        m_blockShader->setMat4("model", model);
        mesh->draw();
        m_renderedChunks++;
    }
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
    m_continentalness = terrainGen.getContinentalnessNoise().sample(m_camPos.x, m_camPos.z);
    m_erosion = terrainGen.getErosionNoise().sample(m_camPos.x, m_camPos.z);
    m_pv = terrainGen.getPvNoise().sample(m_camPos.x, m_camPos.z);

    ImGui::Text("PV: %.3f", m_pv);
    ImGui::Text("Erosion: %.3f", m_erosion);
    ImGui::Text("Continentalness: %.3f", m_continentalness);

    ImGui::End();
}

void Renderer::renderSettings()
{
    ImGui::Begin("Settings");
    ImGui::BeginTabBar("##settings");

    if (ImGui::BeginTabItem("Terrain Generation"))
    {
        std::string noiseNames[5]{
            "Continentalness", "Erosion", "Peaks & Valleys", "Temperature", "Humidity"};
        for (size_t i = 0; i < m_noiseTextures.size(); i++)
        {
            if (i != 0)
                ImGui::SameLine();

            ImGui::BeginChild(noiseNames[i].c_str(), ImVec2(256, 280));
            ImGui::Text("%s - 1024x1024 blocks", noiseNames[i].c_str());
            ImGui::Image((ImTextureID)(intptr_t)m_noiseTextures[i].getID(), ImVec2(256, 256));
            ImGui::EndChild();
        }

        std::string splineNames[3]{"Continentalness", "Erosion", "Peaks & Valleys"};
        plotSpline(
            *m_terrainGenSplines[0],
            splineNames[0].c_str(),
            ImVec2(450, 300),
            "Base terrain height from the overall land shape (ocean vs. plains vs. mountains).");
        ImGui::SameLine();
        plotSpline(
            *m_terrainGenSplines[1],
            splineNames[1].c_str(),
            ImVec2(450, 300),
            "0..1 factor: how much Peaks & Valleys is allowed to pull the terrain away from the "
            "Continentalness base height (0 = flat, 1 = full jaggedness).");
        ImGui::SameLine();
        plotSpline(*m_terrainGenSplines[2],
                   splineNames[2].c_str(),
                   ImVec2(450, 300),
                   "Raw peaks/valleys shape, blended in according to the Erosion factor.");

        if (ImGui::Button("Regenerate"))
        {
            m_shouldRegenerateWorld = true;
        }

        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Graphics"))
    {
        ImGui::Text("graphics settings");

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
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
