#pragma once

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "graphics/shader.h"
#include "graphics/texture.h"
#include "util/uv_rect.h"

struct HudVertex
{
    glm::vec2 pos;
    glm::vec2 uv;
    glm::vec4 color;
};

class HudRenderer
{
public:
    static HudRenderer &instance();
    ~HudRenderer() = default;

    // allocates the buffers in the gpu and empties the data vectors
    void begin();
    // sets up the gl settings, fills the buffers with the data added to the vectors and sends the
    // draw call(s) to the gpu -- screenWidth/screenHeight are needed to build the ortho projection
    void end(int screenWidth,
             int screenHeight,
             std::optional<glm::vec4> scissorRect = std::nullopt);

    //* all the below 'draw' methods add vertices to draw to the vectors -> no draw calls per hud
    //* element, we send one big batch (per atlas) with the end() method
    /**
     * @param pos position of the top left corner
     * @param size width and height
     *
     * @brief SIZE MUST BE AN INTEGER MULTIPLE OF THE NATIVE ICON SIZE IN PIXELS
     */
    void drawIcon(const std::string &name,
                  glm::vec2 pos,
                  glm::vec2 size,
                  glm::vec4 color = glm::vec4{1.0f});
    /**
     * @brief used to draw icons that can make use of the 9-slice technique --> icon must be
     * separable in 9 parts with only the corners not stretched (usually icons with borders of fixed
     * size)
     */
    void drawIconSliced(const std::string &name,
                        glm::vec2 pos,
                        glm::vec2 size,
                        int borderPxNative,
                        float pixelScale,
                        glm::vec4 color = glm::vec4(1.0f));
    /**
     * @param pos top left of the first char in the text
     */
    void drawText(const std::string &text,
                  glm::vec2 pos,
                  float scale,
                  glm::vec4 color = glm::vec4{1.0f});
    /**
     * @param pos top left corner of the text box
     */
    void drawShadowedText(const std::string &text,
                          glm::vec2 pos,
                          float scale,
                          glm::vec4 color = glm::vec4{1.0f});
    /**
     * solid color quad
     */
    void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 color);

    /**
     * @returns the text width in pixels
     */
    int textWidth(const std::string &text);
    static constexpr int TEXT_HEIGHT = 8;

    // for debug/test dumping only -- see tests/test_main.cpp
    unsigned int getIconAtlasTextureID() const { return m_iconAtlasTexture.getID(); }
    unsigned int getFontTextureID() const { return m_fontTexture.getID(); }

private:
    HudRenderer();

    // drawQuad() helper method
    void drawQuad_h(glm::vec2 pos, glm::vec2 size, UVRect uv, glm::vec4 color = glm::vec4{0.3f});
    UVRect getCharUV(char c) const;
    UVRect getIconUV(const std::string &name) const;
    UVRect getWhitePixelUV() const;

    void loadFont();
    void loadIconAtlas();

    // identical setup for both batches (icon atlas / font atlas)
    void setupBuffers(unsigned int &vao, unsigned int &vbo, unsigned int &ebo);
    // uploads one batch's CPU-side data to its GPU buffers, binds its texture and draws it
    void flushBatch(unsigned int vao,
                    unsigned int vbo,
                    unsigned int ebo,
                    const std::vector<HudVertex> &vertData,
                    const std::vector<unsigned int> &idxData,
                    unsigned int textureID);

    static constexpr int FONT_CHAR_SIZE = 8;
    static constexpr int FONT_TEXTURE_SIZE = 128;
    Texture m_fontTexture;
    std::unordered_map<char, UVRect> m_charUV;
    std::unordered_map<char, int> m_charWidth;

    static constexpr int ATLAS_WIDTH = 512;
    int m_atlasHeight = 0;
    Texture m_iconAtlasTexture;
    std::unordered_map<std::string, UVRect> m_iconUV;

    static constexpr int MAX_QUADS = 1024;
    std::vector<HudVertex> m_iconVertData;
    std::vector<HudVertex> m_textVertData;
    std::vector<unsigned int> m_iconIdxData;
    std::vector<unsigned int> m_textIdxData;

    unsigned int m_iconVao, m_iconVbo, m_iconEbo;
    unsigned int m_textVao, m_textVbo, m_textEbo;

    Shader m_shader;
};
