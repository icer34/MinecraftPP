// exécutable de test séparé : #include n'importe quel header de src/ ici pour
// appeler et vérifier une fonction individuellement, sans passer par le main()
// du jeu (pas de fenêtre de gameplay, pas de boucle).

#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "graphics/camera.h"
#include "graphics/frame_data.h"
#include "graphics/mesh/mesh.h"
#include "graphics/shader.h"
#include "util/key_codes.h"
#include "util/window.h"

// mirrors the packing format ChunkMesher writes and water_vert.glsl reads -- see
// chunk_mesher.cpp for the authoritative version. Only what's needed for a flat top-face
// water plane: TOP_FACE_IDX (4) as normalIdx, full brightness AO (0).
namespace
{
constexpr uint32_t TOP_FACE_IDX = 4u;

uint32_t packData1(int x, int y, int z, uint32_t normalIdx)
{
    return (uint32_t(x) << 28) | (uint32_t(y) << 20) | (uint32_t(z) << 16) | (normalIdx << 13);
}

uint32_t packData2(uint32_t cornerIdx, uint32_t aoValue)
{
    return (cornerIdx << 14) | (aoValue << 12);
}

// builds a flat SIZE x SIZE grid of top-face water quads at y=0, one "block" per unit --
// same shape ChunkMesher would emit for a slab of water blocks, just without a Chunk/World
// behind it.
MeshData buildWaterPlane(int size)
{
    MeshData data;

    for (int x = 0; x < size; x++)
    {
        for (int z = 0; z < size; z++)
        {
            unsigned int base = (unsigned int)(data.vertices.size() / 2);

            for (uint32_t cornerIdx = 0; cornerIdx < 4; cornerIdx++)
            {
                data.vertices.push_back(packData1(x, 0, z, TOP_FACE_IDX));
                data.vertices.push_back(packData2(cornerIdx, 0));
            }

            data.indices.insert(data.indices.end(),
                                {base, base + 1, base + 2, base, base + 2, base + 3});
        }
    }

    return data;
}
} // namespace

int main()
{
    Window window(1600, 900, "Water Shader Debug", false);
    glClearColor(0.55f, 0.7f, 0.85f, 1.0f); // sky-ish blue so the water reads clearly against it

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    // reverse-Z projection (see Camera::getProjectionMatrix): clear to 0, closer is greater
    glClearDepth(0.0);
    glDepthFunc(GL_GEQUAL);

    Camera camera(glm::vec3(8.0f, 4.0f, -8.0f));

    GLVertexArray meshVao = Mesh::createVertexArray();
    Mesh waterMesh;
    waterMesh.update(buildWaterPlane(16));

    Shader waterShader("shaders/water_vert.glsl", "shaders/water_frag.glsl");
    FrameDataBuffer frameData;

    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.85f, -0.15f, -0.5f));

    constexpr float MOVE_SPEED = 6.0f;
    float lastFrameTime = (float)window.getTime();

    while (!window.shouldClose())
    {
        window.pollEvents();

        float currentTime = (float)window.getTime();
        float dt = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        camera.rotate((float)window.consumeDx(), (float)window.consumeDy());

        glm::vec3 move(0.0f);
        if (window.isKeyPressed(Key::W))
            move += camera.getFront();
        if (window.isKeyPressed(Key::S))
            move -= camera.getFront();
        if (window.isKeyPressed(Key::A))
            move -= camera.getRight();
        if (window.isKeyPressed(Key::D))
            move += camera.getRight();
        if (window.isKeyPressed(Key::Space))
            move += camera.getUp();
        if (glm::length(move) > 0.0f)
            camera.move(glm::normalize(move) * MOVE_SPEED * dt);

        camera.setAspectRatio(window.getAspectRatio());

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        FrameData data{};
        data.view = camera.getViewMatrix();
        data.projection = camera.getProjectionMatrix();
        data.invView = glm::inverse(data.view);
        data.invProjection = glm::inverse(data.projection);
        data.lightDir = lightDir;
        data.time = currentTime;
        data.camPos = camera.getPos();
        data.zNear = camera.getZNear();
        data.screenSize = glm::vec2(window.getWidth(), window.getHeight());
        data.zFar = camera.getZFar();
        frameData.upload(data);

        waterShader.use();
        waterShader.setVec3(0, glm::vec3(0.0f)); // chunkOffset, layout(location = 0)

        waterMesh.draw(meshVao);

        window.swapBuffers();
    }

    return 0;
}
