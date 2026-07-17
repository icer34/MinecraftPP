#include "block_texture_atlas.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <vector>
namespace fs = std::filesystem;

BlockTextureAtlas::BlockTextureAtlas()
{
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, ATLAS_SIZE, ATLAS_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void BlockTextureAtlas::loadAllTextures()
{
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    int row = 0, col = 0;
    const int texPerCol = ATLAS_COLUMNS;

    //* stbi loads the image from top left to bottom right but openGL excpects it from bottom left
    //* so we flip the vertical loading done by stbi
    stbi_set_flip_vertically_on_load(true);

    std::vector<unsigned char> padded(CELL_STRIDE * CELL_STRIDE * 4);

    for (const auto &entry : fs::directory_iterator("assets/textures/block"))
    {
        // remove other directories and files
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".png")
            continue;

        std::string fileName = entry.path().stem().string();
        std::string filePath = entry.path().string();

        int width, height, channels;
        unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
        if (!data)
        {
            std::cout << "ERROR::FAILED_TO_LOAD_TEXTURE : " << fileName << std::endl;
            continue;
        }

        // build a padded copy: the real texture in the middle, with its edge pixels
        // duplicated into a 1px border -- stops mipmap generation from bleeding a tile's
        // color into its neighbor's atlas cell (cells are packed with no gap otherwise)
        for (int py = 0; py < CELL_STRIDE; py++)
        {
            int srcY = std::clamp(py - PADDING, 0, height - 1);
            for (int px = 0; px < CELL_STRIDE; px++)
            {
                int srcX = std::clamp(px - PADDING, 0, width - 1);
                for (int c = 0; c < 4; c++)
                {
                    padded[(py * CELL_STRIDE + px) * 4 + c] = data[(srcY * width + srcX) * 4 + c];
                }
            }
        }
        stbi_image_free(data);

        int x = col * CELL_STRIDE;
        int y = row * CELL_STRIDE;
        glTexSubImage2D(
            GL_TEXTURE_2D, 0, x, y, CELL_STRIDE, CELL_STRIDE, GL_RGBA, GL_UNSIGNED_BYTE, padded.data());

        m_nameToIndex[fileName] = row * texPerCol + col;

        col++;
        if (col >= texPerCol)
        {
            col = 0;
            row++;
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D);
}

uint16_t BlockTextureAtlas::getIndex(const std::string &fileName) const
{
    return m_nameToIndex.at(fileName);
}

unsigned int BlockTextureAtlas::getID() const { return m_textureID; }
