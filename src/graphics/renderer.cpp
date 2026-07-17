#include "renderer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <implot.h>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "block_texture_atlas.h"
#include "camera.h"
#include "game/chunk.h"
#include "game/world.h"
#include "shader.h"

#include "util/perlin_noise.h"

namespace
{
void plotSpline(Spline &spline, const char *label, ImVec2 size)
{
    glm::vec2 xBounds = spline.getXBounds();
    glm::vec2 yBounds = spline.getYBounds();

    std::string childLabel = std::string(label) + "##spline";

    ImGui::BeginChild(childLabel.c_str(), ImVec2(size.x, size.y + 50));

    if (ImPlot::BeginPlot(label, size, ImPlotFlags_NoLegend))
    {
        ImPlot::SetupAxes("noise", "height");
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
} // namespace

Renderer::Renderer()
    : m_blockTintTexture(Texture("assets/textures/colormap/vanilla/grass.png"))
{
    auto &textureAtlas = BlockTextureAtlas::instance();
    textureAtlas.loadAllTextures();

    m_shader = std::make_unique<Shader>("shaders/vertex.glsl", "shaders/fragment.glsl");

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

void Renderer::renderWorld(const World &world, const Camera &cam)
{
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    m_shader->use();
    m_shader->setMat4("view", cam.getViewMatrix());
    m_shader->setMat4("projection", cam.getProjectionMatrix());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BlockTextureAtlas::instance().getID());
    m_shader->setInt("atlas", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_blockTintTexture.getID());
    m_shader->setInt("colormap", 1);

    for (auto &mesh : world.getChunkMeshes())
    {
        ChunkCoord coord = mesh->getCoords();
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(coord.x, 0.0f, coord.z) * float(Chunk::SIZE));
        m_shader->setMat4("model", model);
        mesh->draw();
    }

    //? ========== UPDATE DEBUG DATA ==========
    m_loadedChunks = world.getChunks().size();
    m_renderedChunks = world.getChunkMeshes().size();
}

void Renderer::renderDebug(float dt)
{
    updateFPS(dt);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //* ===== BASIC DEBUG STATS =====
    ImGui::Begin("Debug pannel");
    ImGui::Text("FPS: %.1f", m_fps);
    ImGui::Text("ms per frame: %.3f", dt * 1000.0);
    ImGui::Text("Loaded chunks: %d", m_loadedChunks);
    ImGui::Text("Rendered chunks: %d", m_renderedChunks);
    ImGui::End();

    //* ===== TERRAIN GENERATION DEBUG =====
    ImGui::Begin("Terrain Generation Debug");

    std::string noiseNames[5]{
        "Continentalness", "Erosion", "Peaks & Valleys", "Temperature", "Humidity"};
    for (size_t i = 0; i < m_noiseTextures.size(); i++)
    {
        ImGui::SameLine();

        ImGui::BeginChild(noiseNames[i].c_str(), ImVec2(256, 280));
        ImGui::Text("%s - 1024x1024 blocks", noiseNames[i].c_str());
        ImGui::Image((ImTextureID)(intptr_t)m_noiseTextures[i].getID(), ImVec2(256, 256));
        ImGui::EndChild();
    }

    std::string splineNames[3]{"Continentalness", "Erosion", "Peaks & Valleys"};
    plotSpline(*m_terrainGenSplines[0], splineNames[0].c_str(), ImVec2(400, 300));
    ImGui::SameLine();
    plotSpline(*m_terrainGenSplines[1], splineNames[1].c_str(), ImVec2(400, 300));
    ImGui::SameLine();
    plotSpline(*m_terrainGenSplines[2], splineNames[2].c_str(), ImVec2(400, 300));

    if (ImGui::Button("Regenerate"))
    {
        m_shouldRegenerateWorld = true;
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
    }
}
