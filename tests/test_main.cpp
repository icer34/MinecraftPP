// exécutable de test séparé : #include n'importe quel header de src/ ici pour
// appeler et vérifier une fonction individuellement, sans passer par le main()
// du jeu (pas de fenêtre de gameplay, pas de boucle).

#include <iostream>
#include <string>
#include <vector>

#include <glad/glad.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "graphics/block_texture_atlas.h"
#include "util/window.h"

int main()
{
    // un Window suffit à obtenir un contexte GL valide (glad + GLFW), même si on
    // n'affiche rien dedans -- BlockTextureAtlas a besoin de ce contexte pour
    // pouvoir appeler glGenTextures/glTexImage2D etc.
    Window window(400, 300, "Mipmap Debug", false);

    auto &atlas = BlockTextureAtlas::instance();
    atlas.loadAllTextures();

    std::cout << "stone index: " << atlas.getIndex("stone") << std::endl;

    glBindTexture(GL_TEXTURE_2D, atlas.getID());

    int maxLevel = 0;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLevel);

    for (int lvl = 0; lvl <= maxLevel; lvl++)
    {
        int width = 0, height = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, lvl, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, lvl, GL_TEXTURE_HEIGHT, &height);

        if (width == 0 || height == 0)
        {
            std::cout << "Level " << lvl << " is empty, stopping." << std::endl;
            break;
        }

        std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);
        glGetTexImage(GL_TEXTURE_2D, lvl, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        std::string path
            = std::string(PROJECT_ROOT_DIR) + "/atlas_mip" + std::to_string(lvl) + ".png";
        stbi_write_png(path.c_str(), width, height, 4, pixels.data(), 0);

        std::cout << "Wrote level " << lvl << " (" << width << "x" << height << ") to " << path
                  << std::endl;
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return 0;
}
