#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>

#include "core/settings_registry.h"
#include "hud_renderer.h"
#include "util/spline.h"

class SettingsMenu
{
public:
    SettingsMenu();

    // returns true if the user requested to close the menu (e.g. clicked "back to game")
    bool render(
        int screenW, int screenH, glm::vec2 cursorPos, bool justClicked, bool isHeld, float scroll);

    void resetNavigation()
    {
        m_activeCategory = std::nullopt;
        m_scrollOffset = 0.0f;
    }

private:
    HudRenderer &m_renderer;
    SettingsRegistry &m_settings;

    int m_screenW;
    int m_screenH;
    glm::vec2 m_cursorPos;
    bool m_justClicked;
    bool m_isHeld;

    float m_scrollOffset = 0.0f;
    float m_totalContentHeight = 0.0f;
    float m_maxScroll = 0.0f;
    bool m_scrollbarActive = false;
    float m_scrollSpeed = 20.0f;

    float *m_activeSliderFloat = nullptr;
    int *m_activeSliderInt = nullptr;

    std::optional<SettingCategory> m_activeCategory = std::nullopt;

    // index into the "settings_scale" cycle options ("1".."4") -- kept separate from the
    // actual scale factor below, since the enum widget needs an index (0-based, wraps via
    // modulo) while everything else needs the literal multiplier (1-based)
    int m_scaleIndex = 1;

    // derived from m_scaleIndex -- computed on every call instead of cached, so changing
    // the scale at runtime (via the settings_scale toggle) takes effect immediately
    int pixelScale() const { return m_scaleIndex + 1; }
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

    // bounds of the scrollable settings area -- derived from m_screenH/widgetSpacing() so both
    // getSettingsHeight() and render() always agree, instead of each recomputing their own copy
    float visibleTop() const { return 3 * widgetSpacing(); }
    float visibleBottom() const { return m_screenH - 6 * widgetSpacing(); }

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