#include "hud_renderer.h"

#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"

#include "graphics/uniform_manager.h"

using glm::mat4;
using glm::vec2;
using glm::vec4;

HudRenderer::HudRenderer()
{
    m_shader.load("hud");

    loadFont();

    loadIconAtlas();

    setupBuffers(m_iconVao, m_iconVbo, m_iconEbo);
    setupBuffers(m_textVao, m_textVbo, m_textEbo);
}

HudRenderer &HudRenderer::instance()
{
    static HudRenderer renderer;
    return renderer;
}

void HudRenderer::setupBuffers(unsigned int &vao, unsigned int &vbo, unsigned int &ebo)
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_QUADS * 4 * sizeof(HudVertex), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_QUADS * 6 * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

    // pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void *)offsetof(HudVertex, pos));
    glEnableVertexAttribArray(0);

    // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void *)offsetof(HudVertex, uv));
    glEnableVertexAttribArray(1);

    // color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void *)offsetof(HudVertex, color));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void HudRenderer::loadFont()
{
    // create the font's texture and the lookup for the correct UVs
    int w, h, chan;
    stbi_set_flip_vertically_on_load(false);
    unsigned char *fontData = stbi_load("game/assets/textures/font/ascii.png", &w, &h, &chan, 4);
    if (!fontData)
    {
        std::cout << "HUD_RENDERER_FAILURE::COULD NOT LOAD THE FONT FILE" << std::endl;
        return;
    }
    m_fontTexture = Texture(fontData, w, h, GL_RGBA8, GL_RGBA);

    // every ascii char as an int is the index in the texture, and is in a 8x8 square
    for (int i = 32; i < 127; i++)
    {
        int CHAR_PER_ROW = FONT_TEXTURE_SIZE / FONT_CHAR_SIZE;
        int row = i / CHAR_PER_ROW;
        int col = i % CHAR_PER_ROW;

        // normalized to [0, 1] -- FONT_TEXTURE_SIZE is the atlas' real pixel size
        float x0 = (float)(col * FONT_CHAR_SIZE) / FONT_TEXTURE_SIZE;
        float x1 = (float)((col + 1) * FONT_CHAR_SIZE) / FONT_TEXTURE_SIZE;
        float y0 = (float)(row * FONT_CHAR_SIZE) / FONT_TEXTURE_SIZE;
        float y1 = (float)((row + 1) * FONT_CHAR_SIZE) / FONT_TEXTURE_SIZE;

        // scan the 8x8 region of each char to get its actual width in order to trim the excess
        // blank space
        int startX = col * FONT_CHAR_SIZE;
        int startY = row * FONT_CHAR_SIZE;

        int minX = -1, maxX = 0;
        for (int x = startX; x < startX + FONT_CHAR_SIZE; x++)
        {
            for (int y = startY; y < startY + FONT_CHAR_SIZE; y++)
            {
                unsigned char alpha = fontData[(x + y * FONT_TEXTURE_SIZE) * 4 + 3];

                if (alpha > 0 && minX == -1)
                    minX = x;
                if (alpha > 0 && x > maxX)
                    maxX = x;
            }
        }

        int charWidth = 0;
        if (minX == -1) // the char is empty (space)
        {
            charWidth = 4;
            x0 = (float)startX / FONT_TEXTURE_SIZE;
            x1 = (float)(startX + charWidth) / FONT_TEXTURE_SIZE;
        }
        else
        {
            charWidth = maxX - minX + 1;
            // recompute x0 x1 so the char is not squished during rendering
            x0 = (float)minX / FONT_TEXTURE_SIZE;
            x1 = (float)(maxX + 1) / FONT_TEXTURE_SIZE;
        }

        m_charWidth.insert({(char)i, charWidth});
        m_charUV.insert({(char)i, UVRect{x0, x1, y0, y1}});
    }

    m_fontTexture.setFilters(GL_NEAREST, GL_NEAREST);

    stbi_image_free(fontData);
}

void HudRenderer::loadIconAtlas()
{
    struct IconData
    {
        std::string name;
        std::vector<unsigned char> data;
        int w, h, x, y;
    };

    // 1. load all the png files
    std::vector<IconData> iconData;
    for (const auto &entry : fs::recursive_directory_iterator("game/assets/textures/gui"))
    {
        if (entry.path().extension() != ".png")
            continue;

        std::string fileName = entry.path().stem().string();
        std::string filePath = entry.path().string();

        int width, height, channels;
        unsigned char *fileData = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
        if (!fileData)
        {
            std::cout << "HUD_ERROR::FAILED TO LOAD IMAGE:: " << filePath << std::endl;
            continue;
        }

        // store the width / height
        iconData.push_back(IconData(
            fileName, std::vector<unsigned char>(fileData, fileData + (width * height * 4)), width, height, 0, 0));
        stbi_image_free(fileData);
    }
    // add a white pixel for the colored quads
    iconData.push_back(IconData{"white_pixel", std::vector<unsigned char>{255, 255, 255, 255}, 1, 1, 0, 0});

    // 2. sort the data obtained by height
    std::sort(iconData.begin(), iconData.end(), [](const IconData &a, const IconData &b) { return a.h < b.h; });

    // 3. pack the images in the atlas --> first look at how they are packed (assign each IconData
    // it's x, y)
    int curX = 0, curY = 0, rowHeight = 0;
    for (auto &icon : iconData)
    {
        if (curX + icon.w > ATLAS_WIDTH)
        {
            curY += rowHeight;
            curX = 0;
            rowHeight = 0;
        }

        // update the row height
        rowHeight = icon.h > rowHeight ? icon.h : rowHeight;

        icon.x = curX;
        icon.y = curY;

        curX += icon.w;
    }

    m_atlasHeight = curY + rowHeight;

    // 4. create the texture and add all the icons now that they're packed correctly
    m_iconAtlasTexture = Texture(nullptr, ATLAS_WIDTH, m_atlasHeight, GL_RGBA8, GL_RGBA);
    for (auto &icon : iconData)
    {
        m_iconAtlasTexture.addSubImage(icon.x, icon.y, icon.w, icon.h, icon.data.data());

        // save the icons UVs for later use -- normalized to [0, 1]
        m_iconUV.insert({icon.name,
                         UVRect{(float)icon.x / ATLAS_WIDTH,
                                (float)(icon.x + icon.w) / ATLAS_WIDTH,
                                (float)icon.y / m_atlasHeight,
                                (float)(icon.y + icon.h) / m_atlasHeight}});
    }
    m_iconAtlasTexture.setFilters(GL_NEAREST, GL_NEAREST);
}

void HudRenderer::begin()
{
    m_iconVertData.clear();
    m_iconIdxData.clear();
    m_textVertData.clear();
    m_textIdxData.clear();
}

void HudRenderer::flushBatch(unsigned int vao,
                             unsigned int vbo,
                             unsigned int ebo,
                             const std::vector<HudVertex> &vertData,
                             const std::vector<unsigned int> &idxData,
                             unsigned int textureID)
{
    if (idxData.empty())
        return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertData.size() * sizeof(HudVertex), vertData.data());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, idxData.size() * sizeof(unsigned int), idxData.data());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)idxData.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void checkGLError(const char *label)
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cout << "GL_ERROR [" << label << "]: " << err << std::endl;
    }
}

void HudRenderer::end(int screenWidth, int screenHeight, std::optional<glm::vec4> scissorRect)
{
    // hud elements are always on top, drawn front-to-back in the order the draw calls were
    // issued (no depth test), and can have transparent edges/backgrounds (blending)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (scissorRect.has_value())
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor((int)scissorRect->x,
                  screenHeight - (int)(scissorRect->y + scissorRect->w),
                  (int)scissorRect->z,
                  (int)scissorRect->w);
    }

    mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);

    auto &uniforms = UniformManager::instance();
    uniforms.setValue("orthProj", projection);
    uniforms.setValue("hudAtlas", 0);

    m_shader.bind();
    uniforms.applyTo(m_shader);

    flushBatch(m_iconVao, m_iconVbo, m_iconEbo, m_iconVertData, m_iconIdxData, m_iconAtlasTexture.getID());

    flushBatch(m_textVao, m_textVbo, m_textEbo, m_textVertData, m_textIdxData, m_fontTexture.getID());

    if (scissorRect.has_value())
        glDisable(GL_SCISSOR_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);

    // restore the state the 3D world renderer expects for the next frame -- these flags are
    // set once at Renderer init, not re-enabled every frame, so leaving them off here would
    // silently break depth testing/culling for the whole next frame's world render
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
}

void HudRenderer::drawIcon(const std::string &name, vec2 pos, vec2 size, vec4 color)
{
    UVRect uv = m_iconUV.at(name);
    drawQuad_h(pos, size, uv, color);
}

void HudRenderer::drawIconSliced(
    const std::string &name, vec2 pos, vec2 size, int borderPxNative, float pixelScale, vec4 color)
{
    UVRect uv = m_iconUV.at(name);

    float uBorder = borderPxNative / (float)ATLAS_WIDTH;
    float vBorder = borderPxNative / (float)m_atlasHeight;

    float u[4] = {uv.x0, uv.x0 + uBorder, uv.x1 - uBorder, uv.x1};
    float v[4] = {uv.y0, uv.y0 + vBorder, uv.y1 - vBorder, uv.y1};

    float screenBorder = borderPxNative * pixelScale;
    float x[4] = {pos.x, pos.x + screenBorder, pos.x + size.x - screenBorder, pos.x + size.x};
    float y[4] = {pos.y, pos.y + screenBorder, pos.y + size.y - screenBorder, pos.y + size.y};

    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 3; col++)
            drawQuad_h(vec2(x[col], y[row]),
                       vec2(x[col + 1] - x[col], y[row + 1] - y[row]),
                       UVRect(u[col], u[col + 1], v[row], v[row + 1]),
                       color);
}

void HudRenderer::drawText(const std::string &text, vec2 pos, float scale, vec4 color)
{
    // align the position on a pixel to avoid vuisual artefacts due to subpixel alignments in the
    // texture
    pos = glm::floor(pos);

    int xOffset = 0;
    for (size_t i = 0; i < text.length(); i++)
    {
        char c = text.at(i);
        UVRect uv = m_charUV.at(c);
        vec2 charPos = vec2(pos.x + xOffset * scale, pos.y);
        vec2 charSize = vec2(m_charWidth.at(c) * scale, FONT_CHAR_SIZE * scale);

        unsigned int addedVertices = (unsigned int)m_textVertData.size();

        // uv.y0 is the top of the glyph, uv.y1 the bottom (see loadFont/loadIconAtlas: y grows
        // downward, matching screen-space y, since the textures aren't flipped on load)
        m_textVertData.push_back(HudVertex{charPos, vec2(uv.x0, uv.y0), color}); // top left
        m_textVertData.push_back(
            HudVertex{vec2(charPos.x + charSize.x, charPos.y), vec2(uv.x1, uv.y0), color}); // top right
        m_textVertData.push_back(HudVertex{charPos + charSize, vec2(uv.x1, uv.y1), color}); // bot right
        m_textVertData.push_back(
            HudVertex{vec2(charPos.x, charPos.y + charSize.y), vec2(uv.x0, uv.y1), color}); // bot left

        m_textIdxData.insert(m_textIdxData.end(),
                             {
                                 addedVertices,
                                 addedVertices + 1,
                                 addedVertices + 2,
                                 addedVertices,
                                 addedVertices + 2,
                                 addedVertices + 3,
                             });

        xOffset += m_charWidth.at(c) + 1;
    }
}

void HudRenderer::drawShadowedText(const std::string &text, vec2 pos, float scale, vec4 color)
{
    vec4 shadowColor = vec4(color.r * 0.25f, color.g * 0.25f, color.b * 0.25f, color.a);
    drawText(text, pos + vec2(scale, scale), scale, shadowColor);
    drawText(text, pos, scale, color);
}

void HudRenderer::drawQuad_h(vec2 pos, vec2 size, UVRect uv, vec4 color)
{
    // align the position on a pixel to avoid vuisual artefacts due to subpixel alignments in the
    // texture
    pos = glm::floor(pos);

    unsigned int addedVertices = (unsigned int)m_iconVertData.size();

    // uv.y0 is the top of the source image, uv.y1 the bottom (see loadIconAtlas: y grows
    // downward while packing, matching screen-space y, since icons aren't flipped on load)
    m_iconVertData.push_back(HudVertex{pos, vec2(uv.x0, uv.y0), color});                         // top left
    m_iconVertData.push_back(HudVertex{vec2(pos.x + size.x, pos.y), vec2(uv.x1, uv.y0), color}); // top right
    m_iconVertData.push_back(HudVertex{pos + size, vec2(uv.x1, uv.y1), color});                  // bot right
    m_iconVertData.push_back(HudVertex{vec2(pos.x, pos.y + size.y), vec2(uv.x0, uv.y1), color}); // bot left

    m_iconIdxData.insert(m_iconIdxData.end(),
                         {
                             addedVertices,
                             addedVertices + 1,
                             addedVertices + 2,
                             addedVertices,
                             addedVertices + 2,
                             addedVertices + 3,
                         });
}

void HudRenderer::drawQuad(vec2 pos, vec2 size, vec4 color) { drawQuad_h(pos, size, getWhitePixelUV(), color); }

UVRect HudRenderer::getIconUV(const std::string &name) const { return m_iconUV.at(name); }

UVRect HudRenderer::getCharUV(char c) const { return m_charUV.at(c); }

UVRect HudRenderer::getWhitePixelUV() const { return m_iconUV.at("white_pixel"); }

int HudRenderer::textWidth(const std::string &text)
{
    int w = 0;
    for (char c : text)
    {
        w += m_charWidth.at(c) + 1;
    }
    return w;
}
