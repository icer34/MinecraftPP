#include "texture_atlas.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

TextureAtlas::TextureAtlas()
{
    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        ATLAS_SIZE,
        ATLAS_SIZE,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
        
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureAtlas::loadAllTextures()
{
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    
    int row = 0, col = 0;
    const int texPerCol = ATLAS_SIZE / TEXTURE_SIZE;

    //* stbi loads the image from top left to bottom right but openGL excpects it from bottom left
    //* so we flip the vertical loading done by stbi
    stbi_set_flip_vertically_on_load(true);

    for (const auto& entry : fs::directory_iterator("assets/textures/block"))
    {
        //remove other directories and files
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".png") continue;

        std::string fileName = entry.path().stem().string();
        std::string filePath = entry.path().string();
        
        int width, height, channels;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
        if (!data)
        {
            std::cout << "ERROR::FAILED_TO_LOAD_TEXTURE : " << fileName << std::endl; 
            continue;
        }

        int x = col * TEXTURE_SIZE;
        int y = row * TEXTURE_SIZE;
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height,
                         GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);

        m_nameToUV[fileName] = UVRect {
            static_cast<float>(x) / ATLAS_SIZE,
            static_cast<float>(x + width) / ATLAS_SIZE,
            static_cast<float>(y) / ATLAS_SIZE,
            static_cast<float>(y + height) / ATLAS_SIZE
        };

        col++;
        if (col >= texPerCol)
        {
            col = 0;
            row++;
        }
    }

    glGenerateMipmap(GL_TEXTURE_2D);
}

UVRect TextureAtlas::getUV(const std::string& fileName) const
{
    return m_nameToUV.at(fileName);
}

unsigned int TextureAtlas::getID() const
{
    return m_textureID;
}
