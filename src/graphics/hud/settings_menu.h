/**
 * @file settings_menu.h
 * @brief In-game settings menu, drawn with the HudRenderer.
 */

#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>

#include "game/settings_registry.h"
#include "hud_renderer.h"
#include "util/spline.h"

/**
 * @brief Pause menu with one page per SettingCategory, built from the SettingsRegistry.
 *
 * Immediate-mode UI: everything is laid out, drawn and hit-tested inside render(), every
 * frame. Widgets edit the registered variables directly. The size of the whole menu follows
 * the "Hud Scale" setting.
 */
class SettingsMenu
{
public:
    /**
     * @brief Creates the menu and registers its own "Hud Scale" setting.
     *
     * @param renderer renderer used to draw the menu, must outlive this object
     */
    explicit SettingsMenu(HudRenderer &renderer);

    /**
     * @brief Handles input for the menu and draws it. Call every frame while the menu is open.
     *
     * Shows the main page (one button per category) or the page of the selected category.
     *
     * @param screenW screen width in pixels
     * @param screenH screen height in pixels
     * @param cursorPos cursor position in pixels, from the top-left corner
     * @param justClicked true if the left button was pressed this frame
     * @param isHeld true while the left button is held (for dragging sliders and points)
     * @param scroll scroll wheel offset since the last frame
     * @return true if the user asked to close the menu (e.g. clicked "Back To Game")
     */
    bool render(
        int screenW, int screenH, glm::vec2 cursorPos, bool justClicked, bool isHeld, float scroll);

    /**
     * @brief Goes back to the main page and resets the scrolling. Call when the menu is opened.
     */
    void resetNavigation()
    {
        _activeCategory = std::nullopt;
        _scrollOffset = 0.0f;
    }

private:
    HudRenderer &_renderer;
    SettingsRegistry &_settings;

    int _screenW;
    int _screenH;
    glm::vec2 _cursorPos;
    bool _justClicked;
    bool _isHeld;

    float _scrollOffset = 0.0f;
    float _totalContentHeight = 0.0f;
    float _maxScroll = 0.0f;
    bool _scrollbarActive = false;
    float _scrollSpeed = 20.0f;

    float *_activeSliderFloat = nullptr;
    int *_activeSliderInt = nullptr;

    std::optional<SettingCategory> _activeCategory = std::nullopt;

    // index into the "settings_scale" cycle options ("1".."4") -- kept separate from the
    // actual scale factor below, since the enum widget needs an index (0-based, wraps via
    // modulo) while everything else needs the literal multiplier (1-based)
    int _scaleIndex = 1;

    // derived from _scaleIndex -- computed on every call instead of cached, so changing
    // the scale at runtime (via the settings_scale toggle) takes effect immediately
    int pixelScale() const { return _scaleIndex + 1; }
    int textScale() const { return pixelScale(); }
    glm::vec2 widgetSize() const { return glm::vec2(200, 20) * (float)pixelScale(); }
    int widgetSpacing() const { return 8 * pixelScale(); }
    glm::vec2 checkboxSize() const { return glm::vec2(20, 20) * (float)pixelScale(); }
    glm::vec2 sliderHandleSize() const { return glm::vec2(8, 20) * (float)pixelScale(); }
    glm::vec2 graphSize() const { return glm::vec2(450, 300) * 0.75f; }
    glm::vec2 splineSize() const
    {
        return glm::vec2(graphSize().x, graphSize().y + widgetSize().y + widgetSpacing());
    }
    int scrollbarWidth() const { return 6 * pixelScale(); }

    // bounds of the scrollable settings area -- derived from _screenH/widgetSpacing() so both
    // getSettingsHeight() and render() always agree, instead of each recomputing their own copy
    float visibleTop() const { return 3 * widgetSpacing(); }
    float visibleBottom() const { return _screenH - 6 * widgetSpacing(); }

    /**
     * @returns true if this category needs scrolling
     */
    bool getSettingsHeight(SettingCategory category);
    void renderCategoryDetails(SettingCategory category);

    /**
     * returns true if the button has been clicked
     */
    bool drawButton(const std::string &label, bool enabled, glm::vec2 pos, glm::vec2 size);

    /**
     * returns true if the value has been changed
     */
    bool drawCheckBox(const std::string &label, bool &value, glm::vec2 pos, glm::vec2 size);

    /**
     * returns true if the value of the slider has been modified
     */
    bool drawSliderFloat(const std::string &label,
                         float &value,
                         float min,
                         float max,
                         glm::vec2 pos,
                         glm::vec2 size);

    /**
     * returns true if the value of the slider has been modified
     */
    bool drawSliderInt(
        const std::string &label, int &value, int min, int max, glm::vec2 pos, glm::vec2 size);

    /**
     * returns true if the value of the cycle is changed
     */
    bool drawToggleCycle(const std::string &label,
                         int &index,
                         const std::vector<std::string> &options,
                         glm::vec2 pos,
                         glm::vec2 size);
    /**
     * returns true if the spline has been modified
     */
    void drawSpline(const std::string &label,
                    Spline &value,
                    const std::string &description,
                    glm::vec2 pos,
                    glm::vec2 size);

    void drawScrollbar();
};