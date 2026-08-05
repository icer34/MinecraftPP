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

    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);

    for (int lvl = 0; lvl < MIPMAP_LEVELS; lvl++)
    {
        glTexImage2D(GL_TEXTURE_2D,
                     lvl,
                     GL_RGBA8,
                     ATLAS_SIZE >> lvl,
                     ATLAS_SIZE >> lvl,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_LEVELS - 1);

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

        // create the mipmaps manually for every texture tile
        std::vector<unsigned char> lvlData(data, data + (width * height * 4));
        unsigned int lvlDataSize = width * height;
        for (int lvl = 0; lvl < MIPMAP_LEVELS; lvl++)
        {
            int lvlW = std::max(1, width >> lvl);
            int lvlH = std::max(1, height >> lvl);
            lvlDataSize = lvlW * lvlH;

            // create a padded version of the texture with the appropriated padding
            for (int py = 0; py < CELL_STRIDE >> lvl; py++)
            {
                int srcY = std::clamp(py - (PADDING >> lvl), 0, lvlH - 1);
                for (int px = 0; px < CELL_STRIDE >> lvl; px++)
                {
                    int srcX = std::clamp(px - (PADDING >> lvl), 0, lvlW - 1);
                    for (int c = 0; c < 4; c++)
                    {
                        padded[(px + py * (CELL_STRIDE >> lvl)) * 4 + c]
                            = lvlData[(srcX + srcY * lvlW) * 4 + c];
                    }
                }
            }

            // upload the texture to openGL
            glTexSubImage2D(GL_TEXTURE_2D,
                            lvl,
                            col * (CELL_STRIDE >> lvl),
                            row * (CELL_STRIDE >> lvl),
                            CELL_STRIDE >> lvl,
                            CELL_STRIDE >> lvl,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            padded.data());

            if (lvlDataSize <= 1)
                continue;

            lvlData = downsample(lvlData, lvlW, lvlH);
        }

        stbi_image_free(data);

        m_nameToIndex[fileName] = row * texPerCol + col;

        col++;
        if (col >= texPerCol)
        {
            col = 0;
            row++;
        }
    }
}

std::vector<unsigned char> downsample(const std::vector<unsigned char> &src, int w, int h)
{
    int newW = w / 2;
    int newH = h / 2;
    std::vector<unsigned char> newBuffer(4 * newW * newH);

    for (size_t x = 0; x < newW; x++)
    {
        for (size_t y = 0; y < newH; y++)
        {
            for (size_t c = 0; c < 4; c++)
            {
                auto p1 = src[((2 * x) + (2 * y) * w) * 4 + c];
                auto p2 = src[((2 * x + 1) + (2 * y) * w) * 4 + c];
                auto p3 = src[((2 * x) + (2 * y + 1) * w) * 4 + c];
                auto p4 = src[((2 * x + 1) + (2 * y + 1) * w) * 4 + c];
                auto avg = (p1 + p2 + p3 + p4) / 4;

                newBuffer[(x + y * newW) * 4 + c] = avg;
            }
        }
    }

    return newBuffer;
}

uint16_t BlockTextureAtlas::getIndex(const std::string &fileName) const
{
    return m_nameToIndex.at(fileName);
}

unsigned int BlockTextureAtlas::getID() const { return m_textureID; }
