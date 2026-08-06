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
enum class SettingCategory
{
    Graphics,
    Gameplay,
    Controls,
    WorldGen,
};

constexpr std::array<SettingCategory, 4> ALL_SETTING_CATEGORIES = {SettingCategory::Graphics,
                                                                   SettingCategory::Controls,
                                                                   SettingCategory::Gameplay,
                                                                   SettingCategory::WorldGen};

// the only place that maps the enum to its display text
std::string categoryName(SettingCategory category);

struct SplineSetting
{
    SettingCategory category;
    std::string subPath;
    std::string label;
    Spline *value;
    std::string description;
};

struct EnumSetting
{
    SettingCategory category;
    std::string subPath;
    std::string label;
    int *value; // index in options
    std::vector<std::string> options;
};

struct FloatSetting
{
    SettingCategory category;
    std::string subPath; // e.g. "Water", "Shaders/Bloom" -- empty if no sub-grouping needed
    std::string label;
    float *value;
    float min, max;
};

struct BoolSetting
{
    SettingCategory category;
    std::string subPath;
    std::string label;
    bool *value;
};

struct IntSetting
{
    SettingCategory category;
    std::string subPath;
    std::string label;
    int *value;
    int min, max;
};

using Setting = std::variant<FloatSetting, BoolSetting, IntSetting, EnumSetting, SplineSetting>;

class SettingsRegistry
{
public:
    static SettingsRegistry &instance();

    Setting get(const std::string &label);

    std::vector<Setting> getByCategory(SettingCategory category);

    void addFloat(SettingCategory category,
                  const std::string &subPath,
                  const std::string &label,
                  float *value,
                  float min,
                  float max);
    void addInt(SettingCategory category,
                const std::string &subPath,
                const std::string &label,
                int *value,
                int min,
                int max);
    void addBool(SettingCategory category,
                 const std::string &subPath,
                 const std::string &label,
                 bool *value);
    void addEnum(SettingCategory category,
                 const std::string &subPath,
                 const std::string &label,
                 int *value,
                 std::vector<std::string> options);
    void addSpline(SettingCategory category,
                   const std::string &subPath,
                   const std::string &label,
                   Spline *value,
                   const std::string &description);

private:
    std::unordered_map<std::string, Setting> m_settings;

    SettingsRegistry() = default;
    SettingsRegistry(SettingsRegistry &registry) = delete;
    SettingsRegistry &operator=(const SettingsRegistry &) = delete;
};
