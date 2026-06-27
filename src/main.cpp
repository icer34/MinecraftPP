// main.cpp — Test OpenGL + GLFW + ImGui + GLM
//
// Compatible avec ImGui (backends glfw + opengl3) et un loader OpenGL.
// Ici on suppose l'usage de GLAD. Si tu utilises GLEW ou un autre loader,
// remplace l'include et l'init correspondants (voir commentaires plus bas).

#include <glad/glad.h>      // <-- loader OpenGL (remplace par <GL/glew.h> si GLEW)
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>

static void glfw_error_callback(int error, const char* description)
{
    std::fprintf(stderr, "Erreur GLFW %d: %s\n", error, description);
}

int main()
{
    // --- Initialisation GLFW ---
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
    {
        std::fprintf(stderr, "Echec de l'initialisation de GLFW\n");
        return 1;
    }

    // OpenGL 3.3 Core Profile
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // requis sur macOS
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Test OpenGL/GLFW/ImGui/GLM", nullptr, nullptr);
    if (!window)
    {
        std::fprintf(stderr, "Echec de la creation de la fenetre GLFW\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    // --- Chargement des fonctions OpenGL ---
    // Avec GLAD :
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::fprintf(stderr, "Echec du chargement d'OpenGL (GLAD)\n");
        return 1;
    }
    // Avec GLEW, remplace le bloc ci-dessus par :
    //   glewExperimental = GL_TRUE;
    //   if (glewInit() != GLEW_OK) { ... }

    std::printf("OpenGL version : %s\n", glGetString(GL_VERSION));
    std::printf("GPU            : %s\n", glGetString(GL_RENDERER));

    // --- Test GLM ---
    {
        glm::vec3 a(1.0f, 0.0f, 0.0f);
        glm::vec3 b(0.0f, 1.0f, 0.0f);
        glm::vec3 c = glm::cross(a, b);
        std::printf("GLM cross((1,0,0),(0,1,0)) = (%.1f, %.1f, %.1f)\n", c.x, c.y, c.z);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 3.0f, 4.0f));
        glm::vec4 p = model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        std::printf("GLM translate origine -> (%.1f, %.1f, %.1f)\n", p.x, p.y, p.z);
    }

    // --- Initialisation ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0.10f, 0.12f, 0.15f, 1.00f);
    float rotation = 0.0f;

    // --- Boucle principale ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Fenetre de test ImGui
        {
            ImGui::Begin("Panneau de test");
            ImGui::Text("Tout fonctionne !");
            ImGui::Text("OpenGL : %s", glGetString(GL_VERSION));
            ImGui::ColorEdit3("Couleur de fond", (float*)&clear_color);
            ImGui::SliderFloat("Rotation (GLM)", &rotation, 0.0f, 360.0f);

            // Petit calcul GLM live
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f),
                                        glm::radians(rotation),
                                        glm::vec3(0.0f, 0.0f, 1.0f));
            glm::vec4 v = rot * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            ImGui::Text("Vecteur (1,0) tourne : (%.2f, %.2f)", v.x, v.y);

            ImGui::Text("FPS : %.1f", io.Framerate);
            ImGui::End();
        }

        // Rendu
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // --- Nettoyage ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}