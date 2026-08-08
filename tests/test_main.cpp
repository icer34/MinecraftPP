// exécutable de test séparé : sert de bac à sable pour tester les fonctionnalités du
// pipeline de shaders (includes, uniforms génériques, packing dynamique...) sans passer
// par le jeu -- juste une fenêtre, un triangle codé en dur, et un shader à charger.

#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/shader.h"
#include "graphics/uniform_manager.h"
#include "util/window.h"

namespace
{
// triangle plat en NDC, aucun besoin de caméra/projection pour ce test
constexpr float TRIANGLE_VERTS[] = {
    -0.5f,
    -0.5f,
    0.0f,
    0.5f,
    -0.5f,
    0.0f,
    0.0f,
    0.5f,
    0.0f,
};

unsigned int createTriangleVAO()
{
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE_VERTS), TRIANGLE_VERTS, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return vao;
}
} // namespace

int main()
{
    try
    {
        Window window(800, 600, "Shader Pipeline Test", false);
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);

        unsigned int triangleVAO = createTriangleVAO();

        Shader shader;
        auto &uniforms = UniformManager::instance();
        uniforms.setValue("model", glm::mat4(1.0f));

        shader.load("test_pipeline");

        while (!window.shouldClose())
        {
            window.pollEvents();

            glClear(GL_COLOR_BUFFER_BIT);

            float t = (float)window.getTime();
            uniforms.setValue("time", t);

            shader.bind();
            uniforms.applyTo(shader);

            glBindVertexArray(triangleVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            window.swapBuffers();
        }

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}
