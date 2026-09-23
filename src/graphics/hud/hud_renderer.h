/**
 * @file hud_renderer.h
 * @brief Batched 2D renderer for the HUD and menus (icons, text, colored quads).
 */

#pragma once

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "graphics/shader.h"
#include "graphics/texture.h"
#include "util/uv_rect.h"

/**
 * @brief Vertex of a HUD quad.
 */
struct HudVertex
{
    glm::vec2 pos;   ///< Screen position in pixels, from the top-left corner.
    glm::vec2 uv;    ///< Texture coordinates in the icon or font atlas.
    glm::vec4 color; ///< RGBA color multiplied with the texture.
};

/**
 * @brief Batched 2D renderer for screen-space UI: icons, bitmap text and solid quads.
 *
 * Usage per frame: begin(), any number of `draw...()` calls, then end(). The draw methods
 * only append vertices to CPU-side batches; end() uploads them and issues one draw call per
 * atlas. All positions and sizes are in screen pixels, with the origin at the top-left corner.
 *
 * There are two batches: icons (and solid quads) first, then text. Text is therefore always
 * drawn on top of icons, whatever the call order.
 *
 * Icons are loaded from every PNG of `assets/textures/gui` and packed into one atlas; they
 * are referred to by file name, without folder or extension.
 */
class HudRenderer
{
public:
    // must be constructed after and destroyed before the GL context (owned by Game, declared after
    // the Window) -- a static singleton would outlive glfwTerminate() and crash on GL deletes
    /**
     * @brief Loads the font and the icon atlas, and creates the GPU buffers.
     *
     * Must be constructed after the GL context and destroyed before it.
     */
    HudRenderer();
    ~HudRenderer();

    HudRenderer(const HudRenderer &) = delete;
    HudRenderer &operator=(const HudRenderer &) = delete;

    /**
     * @brief Starts a new batch, discarding everything queued since the last end().
     */
    void begin();

    /**
     * @brief Uploads the queued quads and draws them on top of the current frame.
     *
     * Disables depth testing and culling while drawing, then enables them again for the 3D
     * renderer.
     *
     * @param screenWidth screen width in pixels, used for the orthographic projection
     * @param screenHeight screen height in pixels, used for the orthographic projection
     * @param scissorRect if set, only draws inside this rectangle, given as
     * (x, y, width, height) in pixels from the top-left corner
     */
    void end(int screenWidth,
             int screenHeight,
             std::optional<glm::vec4> scissorRect = std::nullopt);

    //* all the below 'draw' methods add vertices to draw to the vectors -> no draw calls per hud
    //* element, we send one big batch (per atlas) with the end() method
    /**
     * @brief Queues an icon, stretched to `size`.
     *
     * @param name icon file name, without folder or extension
     * @param pos position of the top left corner
     * @param size width and height
     * @param color tint multiplied with the icon
     *
     * @warning `size` must be an integer multiple of the native icon size in pixels, otherwise
     * the pixel art is distorted.
     * @throws std::out_of_range if no icon has that name
     */
    void drawIcon(const std::string &name,
                  glm::vec2 pos,
                  glm::vec2 size,
                  glm::vec4 color = glm::vec4{1.0f});
    /**
     * @brief Queues an icon with 9-slice scaling: the corners keep their size, the edges
     * stretch along one axis and the center stretches along both.
     *
     * Used for icons with fixed-size borders, such as buttons, that must be drawn at any size.
     *
     * @param name icon file name, without folder or extension
     * @param pos position of the top left corner
     * @param size total width and height
     * @param borderPxNative border thickness in the source icon, in texels
     * @param pixelScale screen pixels per icon texel, applied to the borders
     * @param color tint multiplied with the icon
     * @throws std::out_of_range if no icon has that name
     */
    void drawIconSliced(const std::string &name,
                        glm::vec2 pos,
                        glm::vec2 size,
                        int borderPxNative,
                        float pixelScale,
                        glm::vec4 color = glm::vec4(1.0f));
    /**
     * @brief Queues a line of text with the bitmap font.
     *
     * @param text the text; every character must exist in the font
     * @param pos top left of the first char in the text
     * @param scale screen pixels per font pixel
     * @param color text color
     */
    void drawText(const std::string &text,
                  glm::vec2 pos,
                  float scale,
                  glm::vec4 color = glm::vec4{1.0f});
    /**
     * @brief Queues a line of text with a drop shadow (the same color at 25% brightness),
     * offset by one font pixel.
     *
     * @param text the text; every character must exist in the font
     * @param pos top left corner of the text box
     * @param scale screen pixels per font pixel
     * @param color text color
     */
    void drawShadowedText(const std::string &text,
                          glm::vec2 pos,
                          float scale,
                          glm::vec4 color = glm::vec4{1.0f});
    /**
     * @brief Queues a solid color rectangle.
     *
     * @param pos position of the top left corner
     * @param size width and height
     * @param color fill color (alpha is blended)
     */
    void drawQuad(glm::vec2 pos, glm::vec2 size, glm::vec4 color);

    /**
     * @brief Measures a line of text.
     *
     * @returns the text width in font pixels (multiply by the draw scale to get screen pixels)
     */
    int textWidth(const std::string &text);
    static constexpr int TEXT_HEIGHT = 8; ///< Height of a line of text, in font pixels.

    // for debug/test dumping only -- see tests/test_main.cpp
    /** @brief OpenGL name of the icon atlas texture. For debugging and tests only. */
    unsigned int getIconAtlasTextureID() const { return _iconAtlasTexture.getID(); }
    /** @brief OpenGL name of the font texture. For debugging and tests only. */
    unsigned int getFontTextureID() const { return _fontTexture.getID(); }

private:
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
    Texture _fontTexture;
    std::unordered_map<char, UVRect> _charUV;
    std::unordered_map<char, int> _charWidth;

    static constexpr int ATLAS_WIDTH = 512;
    int _atlasHeight = 0;
    Texture _iconAtlasTexture;
    std::unordered_map<std::string, UVRect> _iconUV;

    static constexpr int MAX_QUADS = 1024;
    std::vector<HudVertex> _iconVertData;
    std::vector<HudVertex> _textVertData;
    std::vector<unsigned int> _iconIdxData;
    std::vector<unsigned int> _textIdxData;

    unsigned int _iconVao, _iconVbo, _iconEbo;
    unsigned int _textVao, _textVbo, _textEbo;

    Shader _shader;
};
