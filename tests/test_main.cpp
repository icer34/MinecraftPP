// exécutable de test séparé : #include n'importe quel header de src/ ici pour
// appeler et vérifier une fonction individuellement, sans passer par le main()
// du jeu (pas de fenêtre de gameplay, pas de boucle).

#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "graphics/hud/hud_renderer.h"
#include "util/window.h"

int main()
{
    // un Window suffit à obtenir un contexte GL valide (glad + GLFW), même si on
    // n'affiche rien dedans -- HudRenderer a besoin de ce contexte pour pouvoir
    // appeler glGenTextures/glTexImage2D etc.
    Window window(1600, 900, "Hud Atlas Debug", false);

    HudRenderer hud;

    while (!window.shouldClose())
    {
        window.pollEvents();
        glClear(GL_COLOR_BUFFER_BIT);

        hud.begin();
        std::string text = "je m'appelle oumar et je suis noir";
        float scale = 2.7f;
        hud.drawQuad(glm::vec2(0.0f),
                     glm::vec2(hud.textWidth(text) * scale, hud.TEXT_HEIGHT * scale),
                     glm::vec4(0.5));
        hud.drawText(text, glm::vec2(0.0f), scale, glm::vec4(1.0f));
        hud.drawIcon("crosshair", glm::vec2(800, 450), glm::vec2(30.0f));
        hud.end(1600, 900);

        window.swapBuffers();
    }

    return 0;
}
