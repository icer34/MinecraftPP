/**
 * @file settings_registry.h
 * @brief Central registry of the user-tweakable settings shown in the settings menu.
 */

#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class Spline;

// the root grouping is a small, stable set -- worth an enum so a typo can't silently
// create a duplicate category. Sub-grouping below the root (e.g. "Water", "Shaders")
// stays a free-form string path (see subPath below), since that set grows/changes often.
/**
 * @brief Top-level category of a setting (one page of the settings menu).
 *
 * Finer grouping inside a category uses the free-form `subPath` of each setting.
 */
enum class SettingCategory
{
    Graphics,
    Gameplay,
    Controls,
    WorldGen,
};

/**
 * @brief Every category, in the order they are listed in the settings menu.
 */
constexpr std::array<SettingCategory, 4> ALL_SETTING_CATEGORIES = {SettingCategory::Graphics,
                                                                   SettingCategory::Controls,
                                                                   SettingCategory::Gameplay,
                                                                   SettingCategory::WorldGen};

// the only place that maps the enum to its display text
/**
 * @brief Returns the display name of a category.
 */
std::string categoryName(SettingCategory category);

/**
 * @brief A spline, edited in the menu as a graph with draggable points.
 */
struct SplineSetting
{
    SettingCategory category; ///< Page the setting appears on.
    std::string subPath;      ///< Sub-group inside the category, empty if none.
    std::string label;        ///< Unique display name, also used as the registry key.
    Spline *value;            ///< The edited spline, owned by the caller.
    std::string description;  ///< Help text shown next to the graph.
};

/**
 * @brief A choice between named options, edited as a button that cycles through them.
 */
struct EnumSetting
{
    SettingCategory category;         ///< Page the setting appears on.
    std::string subPath;              ///< Sub-group inside the category, empty if none.
    std::string label;                ///< Unique display name, also used as the registry key.
    int *value;                       ///< Index of the selected option, owned by the caller.
    std::vector<std::string> options; ///< Display names of the options.
};

/**
 * @brief A float value, edited as a slider.
 */
struct FloatSetting
{
    SettingCategory category; ///< Page the setting appears on.
    /// Sub-group inside the category, e.g. "Water" or "Shaders/Bloom"; empty if none.
    std::string subPath;
    std::string label; ///< Unique display name, also used as the registry key.
    float *value;      ///< The edited value, owned by the caller.
    float min;         ///< Slider minimum.
    float max;         ///< Slider maximum.
};

/**
 * @brief A boolean value, edited as a checkbox.
 */
struct BoolSetting
{
    SettingCategory category; ///< Page the setting appears on.
    std::string subPath;      ///< Sub-group inside the category, empty if none.
    std::string label;        ///< Unique display name, also used as the registry key.
    bool *value;              ///< The edited value, owned by the caller.
};

/**
 * @brief An integer value, edited as a slider.
 */
struct IntSetting
{
    SettingCategory category; ///< Page the setting appears on.
    std::string subPath;      ///< Sub-group inside the category, empty if none.
    std::string label;        ///< Unique display name, also used as the registry key.
    int *value;               ///< The edited value, owned by the caller.
    int min;                  ///< Slider minimum.
    int max;                  ///< Slider maximum.
};

/**
 * @brief Any kind of setting.
 */
using Setting = std::variant<FloatSetting, BoolSetting, IntSetting, EnumSetting, SplineSetting>;

/**
 * @brief Global registry of the settings shown in the settings menu (singleton).
 *
 * Systems register pointers to their own variables, usually in their constructor. The menu
 * then edits those variables directly, so changes apply immediately. Every registered
 * variable must outlive the registry's use of it.
 *
 * Settings are identified by their label, which must be unique: adding a setting with an
 * existing label is ignored (with an error message).
 */
class SettingsRegistry
{
public:
    /**
     * @brief Returns the unique registry instance.
     */
    static SettingsRegistry &instance();

    /**
     * @brief Returns the setting with the given label.
     *
     * @throws std::out_of_range if no setting has that label
     */
    Setting get(const std::string &label);

    /**
     * @brief Returns every setting of a category, in no particular order.
     */
    std::vector<Setting> getByCategory(SettingCategory category);

    /**
     * @brief Registers a float slider.
     *
     * @param category page the setting appears on
     * @param subPath sub-group inside the category, empty if none
     * @param label unique display name
     * @param value the variable to edit
     * @param min slider minimum
     * @param max slider maximum
     */
    void addFloat(SettingCategory category,
                  const std::string &subPath,
                  const std::string &label,
                  float *value,
                  float min,
                  float max);
    /**
     * @brief Registers an integer slider. Parameters are the same as addFloat().
     */
    void addInt(SettingCategory category,
                const std::string &subPath,
                const std::string &label,
                int *value,
                int min,
                int max);
    /**
     * @brief Registers a checkbox.
     *
     * @param category page the setting appears on
     * @param subPath sub-group inside the category, empty if none
     * @param label unique display name
     * @param value the variable to edit
     */
    void addBool(SettingCategory category,
                 const std::string &subPath,
                 const std::string &label,
                 bool *value);
    /**
     * @brief Registers a button that cycles through named options.
     *
     * @param category page the setting appears on
     * @param subPath sub-group inside the category, empty if none
     * @param label unique display name
     * @param value index of the selected option, the variable to edit
     * @param options display names of the options
     */
    void addEnum(SettingCategory category,
                 const std::string &subPath,
                 const std::string &label,
                 int *value,
                 std::vector<std::string> options);
    /**
     * @brief Registers a spline editor.
     *
     * @param category page the setting appears on
     * @param subPath sub-group inside the category, empty if none
     * @param label unique display name
     * @param value the spline to edit
     * @param description help text shown next to the graph
     */
    void addSpline(SettingCategory category,
                   const std::string &subPath,
                   const std::string &label,
                   Spline *value,
                   const std::string &description);

private:
    std::unordered_map<std::string, Setting> _settings;

    SettingsRegistry() = default;
    SettingsRegistry(SettingsRegistry &registry) = delete;
    SettingsRegistry &operator=(const SettingsRegistry &) = delete;
};
