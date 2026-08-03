#include "settings_menu.h"

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <variant>

#include <imgui.h>
#include <implot.h>

using glm::vec2;
using glm::vec4;

SettingsMenu::SettingsMenu()
    : m_renderer(HudRenderer::instance()),
      m_settings(SettingsRegistry::instance())
{
    m_settings.addEnum(
        SettingCategory::Graphics, "", "Hud Scale", &m_scaleIndex, {"1", "2", "3", "4"});
    m_settings.addFloat(
        SettingCategory::Gameplay, "", "Scroll Speed", &m_scrollSpeed, 10.0f, 40.0f);
}

bool SettingsMenu::render(
    int screenW, int screenH, glm::vec2 cursorPos, bool justClicked, bool isHeld, float scroll)
{
    m_screenW = screenW;
    m_screenH = screenH;
    m_cursorPos = cursorPos;
    m_justClicked = justClicked;
    m_isHeld = isHeld;
    m_scrollOffset -= scroll * m_scrollSpeed;
    bool closeRequested = false;
    vec2 screenCenter{m_screenW / 2.0f, m_screenH / 2.0f};

    if (!m_activeCategory.has_value())
    {
        m_renderer.begin();

        float centerHeightGap = 1.5f * widgetSpacing() + widgetSize().y;
        vec2 backToGamePos = vec2(screenCenter.x - widgetSize().x / 2.0f,
                                  screenCenter.y - centerHeightGap - widgetSize().y);
        vec2 saveAndQuitPos
            = vec2(screenCenter.x - widgetSize().x / 2.0f, screenCenter.y + centerHeightGap);

        // 6 buttons in the settings menu : 1 per Settings categery + BACK_TO_GAME + SAVE_AND_QUIT
        if (drawButton("Back To Game", true, backToGamePos, widgetSize()))
        {
            closeRequested = true;
        }

        if (drawButton("Save And Quit", true, saveAndQuitPos, widgetSize()))
        {
            // TODO
        }

        // clang-format off
        std::array<vec2, 4> buttonsPos = {{
            {screenCenter.x - widgetSize().x / 2.0f, screenCenter.y - 0.5f * widgetSpacing() - widgetSize().y},
            {screenCenter.x + widgetSpacing(),  screenCenter.y - 0.5f * widgetSpacing() - widgetSize().y},
            {screenCenter.x - widgetSize().x / 2.0f, screenCenter.y + 0.5f * widgetSpacing()},
            {screenCenter.x + widgetSpacing(), screenCenter.y + 0.5f * widgetSpacing()}
        }};
        // clang-format on
        for (int i = 0; i < 4; i++)
        {
            auto category = ALL_SETTING_CATEGORIES[i];
            std::string label = categoryName(category);
            vec2 buttonSize = vec2(widgetSize().x / 2.0f - widgetSpacing(), widgetSize().y);

            if (drawButton(label, true, buttonsPos[i], buttonSize))
                m_activeCategory = category;
        }

        m_renderer.end(m_screenW, m_screenH);
    }
    else
    {
        //* first pass: scrollable list (scissored if needed)
        bool needsScroll = getSettingsHeight(m_activeCategory.value());

        m_renderer.begin();
        renderCategoryDetails(m_activeCategory.value());
        m_renderer.end(
            m_screenW,
            m_screenH,
            m_maxScroll > 0.0f
                ? std::optional(
                      vec4(0.0f, visibleTop(), (float)m_screenW, visibleBottom() - visibleTop()))
                : std::nullopt);

        //* second pass: Done button + scrollbar -> never scissored
        m_renderer.begin();

        if (needsScroll)
        {
            drawScrollbar();
        }

        if (drawButton("Done",
                       true,
                       vec2(screenCenter.x - widgetSize().x / 2.0f,
                            screenH - widgetSpacing() - widgetSize().y),
                       widgetSize()))
            m_activeCategory = std::nullopt;
        m_renderer.end(m_screenW, m_screenH);
    }

    return closeRequested;
}

bool SettingsMenu::getSettingsHeight(SettingCategory category)
{
    float totalHeight = 0.0f;
    std::vector<Setting> settings = m_settings.getByCategory(category);

    // 2 settings (spline or not) per row -- rowHeight tracks whichever type is currently seen,
    // and the row's height is committed every 2 settings, matching renderCategoryDetails
    float rowHeight = widgetSize().y;
    for (int i = 0; i < settings.size(); i++)
    {
        auto setting = settings.at(i);
        std::visit(
            [&](auto &s)
            {
                using T = std::decay_t<decltype(s)>;
                if constexpr (std::is_same_v<T, SplineSetting>)
                    rowHeight = splineSize().y;
            },
            setting);

        if (i % 2 == 1)
        {
            totalHeight += rowHeight + widgetSpacing();
            rowHeight = widgetSize().y;
        }
    }
    // an odd number of settings leaves one trailing row that never hit the i % 2 == 1 branch
    if (settings.size() % 2 == 1)
        totalHeight += rowHeight + widgetSpacing();

    m_totalContentHeight = totalHeight;

    float visibleHeight = visibleBottom() - visibleTop();
    m_maxScroll = std::max(0.0f, m_totalContentHeight - visibleHeight);
    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, m_maxScroll);

    return m_maxScroll > 0.0f;
}

void SettingsMenu::renderCategoryDetails(SettingCategory category)
{
    vec2 screenCenter{m_screenW / 2.0f, m_screenH / 2.0f};
    std::vector<Setting> settings = m_settings.getByCategory(category);
    float xPos[2]
        = {screenCenter.x - widgetSize().x - widgetSpacing(), screenCenter.x + widgetSpacing()};
    vec2 pos = vec2(xPos[0], 3 * widgetSpacing() - m_scrollOffset);

    for (int i = 0; i < settings.size(); i++)
    {
        auto setting = settings.at(i);
        float rowHeight = widgetSize().y;

        std::visit(
            [&](auto &s)
            {
                using T = std::decay_t<decltype(s)>;

                if constexpr (std::is_same_v<T, FloatSetting>)
                {
                    drawSliderFloat(s.label, *s.value, s.min, s.max, pos, widgetSize());
                }
                else if constexpr (std::is_same_v<T, IntSetting>)
                {
                    drawSliderInt(s.label, *s.value, s.min, s.max, pos, widgetSize());
                }
                else if constexpr (std::is_same_v<T, BoolSetting>)
                {
                    drawCheckBox(s.label, *s.value, pos, widgetSize());
                }
                else if constexpr (std::is_same_v<T, EnumSetting>)
                {
                    drawToggleCycle(s.label, *s.value, s.options, pos, widgetSize());
                }
                else if constexpr (std::is_same_v<T, SplineSetting>)
                {
                    drawSpline(s.label, *s.value, s.description, pos, splineSize());
                    rowHeight = splineSize().y;
                }
            },
            setting);

        pos.x = xPos[(i + 1) % 2];
        pos.y += (i % 2) * (rowHeight + widgetSpacing());
    }
}

bool SettingsMenu::drawButton(const std::string &label, bool enabled, glm::vec2 pos, glm::vec2 size)
{
    std::string buttonWidget = "button";

    bool insideX = m_cursorPos.x >= pos.x && m_cursorPos.x < pos.x + size.x;
    bool insideY = m_cursorPos.y >= pos.y && m_cursorPos.y < pos.y + size.y;
    bool isHovered = insideX && insideY;

    if (!enabled)
        buttonWidget = "button_disabled";
    else if (isHovered)
        buttonWidget = "button_highlighted";

    m_renderer.drawIconSliced(buttonWidget, pos, size, 2, pixelScale());

    vec2 buttonCenter = vec2(pos.x + size.x / 2, pos.y + size.y / 2);
    float labelWidth = m_renderer.textWidth(label);
    vec2 textSize = vec2(labelWidth * textScale(), HudRenderer::TEXT_HEIGHT * textScale());
    vec2 textPos = vec2(buttonCenter.x - textSize.x / 2, buttonCenter.y - textSize.y / 2);
    m_renderer.drawShadowedText(label, textPos, textScale());

    return m_justClicked && isHovered;
}

bool SettingsMenu::drawCheckBox(const std::string &label,
                                bool &value,
                                glm::vec2 pos,
                                glm::vec2 size)
{
    std::string texture = "checkbox";

    bool insideX = m_cursorPos.x >= pos.x && m_cursorPos.x < pos.x + size.x;
    bool insideY = m_cursorPos.y >= pos.y && m_cursorPos.y < pos.y + size.y;
    bool isHovered = insideX && insideY;

    if (isHovered && value)
        texture = "checkbox_selected_highlighted";
    else if (value)
        texture = "checkbox_selected";
    else if (isHovered)
        texture = "checkbox_highlighted";

    vec2 textAreaSize = vec2(widgetSize().x - checkboxSize().x, widgetSize().y);

    m_renderer.drawIcon(texture, pos + vec2(textAreaSize.x, 0.0f), checkboxSize());

    float textWidth = m_renderer.textWidth(label);
    vec2 textSize = vec2(textWidth * textScale(), HudRenderer::TEXT_HEIGHT * textScale());
    vec2 center = vec2(pos.x + size.x / 2, pos.y + size.y / 2);
    vec2 textPos = vec2(center.x - textSize.x / 2, center.y - textSize.y / 2);
    m_renderer.drawShadowedText(label, textPos, textScale());

    if (isHovered && m_justClicked)
        value = !value;

    return m_justClicked && isHovered;
}

bool SettingsMenu::drawSliderFloat(
    const std::string &label, float &value, float min, float max, glm::vec2 pos, glm::vec2 size)
{
    bool insideX = m_cursorPos.x >= pos.x && m_cursorPos.x < pos.x + size.x;
    bool insideY = m_cursorPos.y >= pos.y && m_cursorPos.y < pos.y + size.y;
    bool isHovered = insideX && insideY;
    bool isActive = m_activeSliderFloat == &value;

    // value updating logic
    if (m_justClicked && isHovered)
        m_activeSliderFloat = &value;

    if (!m_isHeld)
        m_activeSliderFloat = nullptr;

    bool changed = false;
    if (isActive)
    {
        float t = std::clamp((m_cursorPos.x - pos.x) / size.x, 0.0f, 1.0f);
        float newVal = min + t * (max - min);
        if (newVal != value)
        {
            value = newVal;
            changed = true;
        }
    }

    // rendering
    std::string baseTexture = "slider";
    std::string handleTexture = "slider_handle";
    if (isHovered || isActive)
        baseTexture = "slider_highlighted";

    if (m_isHeld && isActive)
        handleTexture = "slider_handle_highlighted";

    m_renderer.drawIcon(baseTexture, pos, size);

    float t = (value - min) / (max - min);
    vec2 handlePos = vec2((pos.x + t * size.x) - (sliderHandleSize().x / 2), pos.y);
    m_renderer.drawIcon(handleTexture, handlePos, sliderHandleSize());

    std::string displayLabel = std::format("{}: {:.2f}", label, value);
    float textWidth = m_renderer.textWidth(displayLabel);
    vec2 textSize = vec2(textWidth * textScale(), HudRenderer::TEXT_HEIGHT * textScale());
    vec2 center = vec2(pos.x + size.x / 2, pos.y + size.y / 2);
    vec2 textPos = vec2(center.x - textSize.x / 2, center.y - textSize.y / 2);
    m_renderer.drawShadowedText(displayLabel, textPos, textScale());

    return changed;
}

bool SettingsMenu::drawSliderInt(
    const std::string &label, int &value, int min, int max, glm::vec2 pos, glm::vec2 size)
{
    bool insideX = m_cursorPos.x >= pos.x && m_cursorPos.x < pos.x + size.x;
    bool insideY = m_cursorPos.y >= pos.y && m_cursorPos.y < pos.y + size.y;
    bool isHovered = insideX && insideY;
    bool isActive = m_activeSliderInt == &value;

    // value updating logic
    if (m_justClicked && isHovered)
        m_activeSliderInt = &value;

    if (!m_isHeld)
        m_activeSliderInt = nullptr;

    bool changed = false;
    if (isActive)
    {
        float t = std::clamp((m_cursorPos.x - pos.x) / size.x, 0.0f, 1.0f);
        int newVal = min + static_cast<int>(std::round(t * (max - min)));
        if (newVal != value)
        {
            value = newVal;
            changed = true;
        }
    }

    // rendering
    std::string baseTexture = "slider";
    std::string handleTexture = "slider_handle";
    if (isHovered || isActive)
        baseTexture = "slider_highlighted";

    if (m_isHeld && isActive)
        handleTexture = "slider_handle_highlighted";

    m_renderer.drawIcon(baseTexture, pos, size);

    float t = float(value - min) / float(max - min);
    vec2 handlePos = vec2((pos.x + t * size.x) - (sliderHandleSize().x / 2), pos.y);
    m_renderer.drawIcon(handleTexture, handlePos, sliderHandleSize());

    std::string displayLabel = std::format("{}: {}", label, value);
    float textWidth = m_renderer.textWidth(displayLabel);
    vec2 textSize = vec2(textWidth * textScale(), HudRenderer::TEXT_HEIGHT * textScale());
    vec2 center = vec2(pos.x + size.x / 2, pos.y + size.y / 2);
    vec2 textPos = vec2(center.x - textSize.x / 2, center.y - textSize.y / 2);
    m_renderer.drawShadowedText(displayLabel, textPos, textScale());

    return changed;
}

bool SettingsMenu::drawToggleCycle(const std::string &label,
                                   int &index,
                                   const std::vector<std::string> &options,
                                   glm::vec2 pos,
                                   glm::vec2 size)
{
    std::string displayLabel = std::format("{}: {}", label, options[index]);
    bool clicked = drawButton(displayLabel, true, pos, size);

    if (clicked)
        index = (index + 1) % (int)options.size();

    return clicked;
}

void SettingsMenu::drawSpline(const std::string &label,
                              Spline &spline,
                              const std::string &description,
                              glm::vec2 pos,
                              glm::vec2 size)
{
    // entierement hors zone visible -> pas la peine d'ouvrir la fenetre du tout
    if (pos.y + size.y < visibleTop() || pos.y > visibleBottom())
        return;

    vec2 xBounds = spline.getXBounds();
    vec2 yBounds = spline.getYBounds();

    std::string childLabel = std::string(label) + "##spline";

    ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin(childLabel.c_str(),
                 NULL,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
                     | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

    // clip reel : intersecte la fenetre avec la zone visible du panneau
    float clipMinY = std::max(pos.y, visibleTop());
    float clipMaxY = std::min(pos.y + size.y, visibleBottom());
    ImGui::PushClipRect(ImVec2(0.0f, clipMinY), ImVec2((float)m_screenW, clipMaxY), true);

    //? ImGui::Text("%s", label); --> custom text ?
    if (!description.empty())
    {
        // TODO: info widget for description
    }

    if (ImPlot::BeginPlot("##plot",
                          ImVec2(graphSize().x, graphSize().y),
                          ImPlotFlags_NoLegend | ImPlotFlags_NoMenus))
    {
        ImPlot::SetupAxis(ImAxis_X1, "x", ImPlotAxisFlags_NoLabel);
        ImPlot::SetupAxis(ImAxis_Y1, "y ", ImPlotAxisFlags_NoLabel);
        ImPlot::SetupAxesLimits(
            xBounds.x * 1.1, xBounds.y * 1.1, yBounds.x, yBounds.y * 1.1, ImPlotCond_Always);

        const auto &xs = spline.getXValues();
        const auto &ys = spline.getYValues();

        ImPlot::PlotLine("curve", xs.data(), ys.data(), static_cast<int>(xs.size()));

        int draggedIndex = -1;
        float draggedX = 0.0f, draggedY = 0.0f;
        int toRemove = -1;

        for (size_t i = 0; i < xs.size(); i++)
        {
            double x = xs[i];
            double y = ys[i];

            if (ImPlot::DragPoint(static_cast<int>(i), &x, &y, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)))
            {
                draggedIndex = static_cast<int>(i);
                draggedX = static_cast<float>(x);
                draggedY = static_cast<float>(y);
            }
        }

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)
            && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImVec2 mousePixel = ImGui::GetMousePos();
            float bestDistSq = FLT_MAX;
            int bestIndex = -1;

            for (size_t i = 0; i < xs.size(); i++)
            {
                ImVec2 pointPixel = ImPlot::PlotToPixels(ImPlotPoint(xs[i], ys[i]));
                float dx = pointPixel.x - mousePixel.x;
                float dy = pointPixel.y - mousePixel.y;
                float distSq = dx * dx + dy * dy;

                if (distSq < bestDistSq)
                {
                    bestDistSq = distSq;
                    bestIndex = static_cast<int>(i);
                }
            }

            constexpr float clickRadiusPx = 10.0f;
            if (bestIndex >= 0 && bestDistSq < clickRadiusPx * clickRadiusPx)
                toRemove = bestIndex;
        }

        // apply after the loop: setPoint() may re-sort the spline's internal vectors,
        // which would invalidate xs/ys (still referenced above) if done mid-loop
        if (draggedIndex >= 0)
            spline.setPoint(static_cast<size_t>(draggedIndex), draggedX, draggedY);

        if (toRemove >= 0)
            spline.removePoint(static_cast<size_t>(toRemove));

        ImPlot::EndPlot();
    }

    vec2 addBtnPos = vec2(pos.x, pos.y + graphSize().y);
    if (drawButton("Add", true, addBtnPos, vec2(splineSize().x, widgetSize().y)))
    {
        const auto &xs = spline.getXValues();
        float bestGap = 0.0f, bestX = xBounds.x, bestY = (yBounds.x + yBounds.y) / 2.0f;
        for (size_t i = 0; i + 1 < xs.size(); i++)
        {
            float gap = xs[i + 1] - xs[i];
            if (gap > bestGap)
            {
                bestGap = gap;
                bestX = (xs[i] + xs[i + 1]) / 2.0f;
                bestY = spline.get(bestX);
            }
        }
        spline.addPoint(bestX, bestY);
    }

    ImGui::PopClipRect();

    ImGui::End();
    ImGui::PopStyleVar();
}

void SettingsMenu::drawScrollbar()
{
    if (m_maxScroll <= 0.0f)
    {
        m_scrollbarActive = false;
        return;
    }

    float visibleHeight = visibleBottom() - visibleTop();
    float trackX = m_screenW - widgetSpacing() - scrollbarWidth();
    vec2 trackPos = vec2(trackX, visibleTop());
    vec2 trackSize = vec2(scrollbarWidth(), visibleHeight);

    float handleRatio = visibleHeight / m_totalContentHeight;
    float handleHeight = std::max(20.0f, trackSize.y * handleRatio);
    float scrollRatio = m_scrollOffset / m_maxScroll;
    vec2 handlePos = vec2(trackX, visibleTop() + scrollRatio * (trackSize.y - handleHeight));
    vec2 handleSize = vec2(scrollbarWidth(), handleHeight);

    bool insideX = m_cursorPos.x >= handlePos.x && m_cursorPos.x < handlePos.x + handleSize.x;
    bool insideY = m_cursorPos.y >= handlePos.y && m_cursorPos.y < handlePos.y + handleSize.y;

    if (m_justClicked && insideX && insideY)
        m_scrollbarActive = true;

    if (!m_isHeld)
        m_scrollbarActive = false;

    if (m_scrollbarActive)
    {
        // position of the cursor in the range [0..1]
        float t = std::clamp((m_cursorPos.y - visibleTop() - handleSize.y / 2.0f)
                                 / (trackSize.y - handleSize.y),
                             0.0f,
                             1.0f);
        m_scrollOffset = t * m_maxScroll;
    }

    m_renderer.drawIconSliced("scroller_background", trackPos, trackSize, 1, pixelScale());
    m_renderer.drawIconSliced("scroller", handlePos, handleSize, 1, pixelScale());
}
