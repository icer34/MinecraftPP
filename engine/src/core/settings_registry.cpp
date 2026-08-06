#include "settings_registry.h"

#include <iostream>

std::string categoryName(SettingCategory category)
{
    switch (category)
    {
    case SettingCategory::Graphics:
        return "Graphics";
    case SettingCategory::Gameplay:
        return "Gameplay";
    case SettingCategory::Controls:
        return "Controls";
    case SettingCategory::WorldGen:
        return "World";
    }

    return "Unknown";
}

SettingsRegistry &SettingsRegistry::instance()
{
    static SettingsRegistry registry;
    return registry;
}

void SettingsRegistry::addFloat(SettingCategory category,
                                const std::string &subPath,
                                const std::string &label,
                                float *value,
                                float min,
                                float max)
{
    if (m_settings.find(label) != m_settings.end())
    {
        std::cout << "Cannot add an existing setting: [" << label << "]" << std::endl;
        return;
    }

    m_settings[label] = FloatSetting{category, subPath, label, value, min, max};
}

void SettingsRegistry::addInt(SettingCategory category,
                              const std::string &subPath,
                              const std::string &label,
                              int *value,
                              int min,
                              int max)
{
    if (m_settings.find(label) != m_settings.end())
    {
        std::cout << "Cannot add an existing setting: [" << label << "]" << std::endl;
        return;
    }

    m_settings[label] = IntSetting{category, subPath, label, value, min, max};
}

void SettingsRegistry::addBool(SettingCategory category,
                               const std::string &subPath,
                               const std::string &label,
                               bool *value)
{
    if (m_settings.find(label) != m_settings.end())
    {
        std::cout << "Cannot add an existing setting: [" << label << "]" << std::endl;
        return;
    }

    m_settings[label] = BoolSetting{category, subPath, label, value};
}

void SettingsRegistry::addEnum(SettingCategory category,
                               const std::string &subPath,
                               const std::string &label,
                               int *value,
                               std::vector<std::string> options)
{
    if (m_settings.find(label) != m_settings.end())
    {
        std::cout << "Cannot add an existing setting: [" << label << "]" << std::endl;
        return;
    }

    m_settings[label] = EnumSetting{category, subPath, label, value, options};
}

void SettingsRegistry::addSpline(SettingCategory category,
                                 const std::string &subPath,
                                 const std::string &label,
                                 Spline *value,
                                 const std::string &description)
{
    if (m_settings.find(label) != m_settings.end())
    {
        std::cout << "Cannot add an existing setting: [" << label << "]" << std::endl;
        return;
    }

    m_settings[label] = SplineSetting{category, subPath, label, value, description};
}

Setting SettingsRegistry::get(const std::string &label) { return m_settings.at(label); }

std::vector<Setting> SettingsRegistry::getByCategory(SettingCategory category)
{
    std::vector<Setting> result;
    for (auto &[label, setting] : m_settings)
    {
        SettingCategory cat = std::visit([](auto &s) { return s.category; }, setting);
        if (cat == category)
            result.push_back(setting);
    }
    return result;
}
