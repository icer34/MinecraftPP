// exécutable de test séparé : #include n'importe quel header de src/ ici pour
// appeler et vérifier une fonction individuellement, sans passer par le main()
// du jeu (pas de fenêtre de gameplay, pas de boucle).

#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>

// implementation lives in src/util/stb_image_write_impl.cpp (engine_lib), so it's not
// redefined here too -- would be a duplicate symbol at link time otherwise
#include "stb_image_write.h"

#include "graphics/block_texture_atlas.h"
#include "util/window.h"

int main()
{
    // un Window suffit à obtenir un contexte GL valide (glad + GLFW), nécessaire pour que
    // BlockTextureAtlas puisse appeler glGenTextures/glTexImage2D etc.
    Window window(1600, 900, "Block Atlas Dump", false);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    BlockTextureAtlas::instance().loadAllTextures();
    unsigned int atlasID = BlockTextureAtlas::instance().getID();

    glBindTexture(GL_TEXTURE_2D, atlasID);
    int width = 0, height = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    std::vector<unsigned char> pixels(width * height * 4);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    std::string outPath = std::string(PROJECT_ROOT_DIR) + "/block_atlas_dump.png";
    if (stbi_write_png(outPath.c_str(), width, height, 4, pixels.data(), width * 4))
        std::cout << "Block atlas (" << width << "x" << height << ") dumped to " << outPath
                  << std::endl;
    else
        std::cout << "BLOCK_ATLAS_DUMP_FAILURE::stbi_write_png failed" << std::endl;

    while (!window.shouldClose())
    {
        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}
