#pragma once

#include <unordered_map>
#include <string>

#include "util/directions.h"
#include "util/uv_rect.h"

class TextureAtlas
{
public:
    static TextureAtlas& instance()
    {
        static TextureAtlas atlas;
        return atlas;
    }

    void loadAllTextures();

    UVRect getUV(const std::string& fileName) const;
    unsigned int getID() const;

private:
    static constexpr int TEXTURE_SIZE = 16;
    static constexpr int ATLAS_SIZE = 1024;

    std::unordered_map<std::string, UVRect> m_nameToUV;

    unsigned int m_textureID;

    TextureAtlas();
    TextureAtlas(TextureAtlas& registry) = delete;
    TextureAtlas& operator=(const TextureAtlas&) = delete;
};