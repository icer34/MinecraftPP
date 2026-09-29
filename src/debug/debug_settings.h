#pragma once

#include "util/input.h"

/**
 * @brief Runtime flags of the debug tools, not in the SettingsMenu (debug tools only available
 * through kayboard shortcuts)
 */
struct DebugSettings
{
    bool showPanel = false; ///< F3 panel
    bool showHelp = false;  ///< Shortcut binds list
    bool showPerfGraphs = true;

    bool wireframe = false;
    bool showChunkBorders = false;
    bool showEntityHitBoxes = false;
    bool freezeCulling = false;

    //... might grow as more debug tools may be needed
};

struct DebugToggle
{
    Key key;
    bool DebugSettings::*flag;
    const char *description;
};
/// @brief keybinds of the various debug tools
// TODO:will be in a separate config file
constexpr DebugToggle TOGGLES[] = {
    {Key::G, &DebugSettings::showChunkBorders, "chunk borders"},
    {Key::B, &DebugSettings::showEntityHitBoxes, "show hitboxes"},
    {Key::L, &DebugSettings::wireframe, "wireframe mode"},
    {Key::C, &DebugSettings::freezeCulling, "freeze culling"},
    {Key::H, &DebugSettings::showHelp, "help"},
};

/// Global access fromn everywhere in the game
inline DebugSettings &debugSettings()
{
    static DebugSettings instance;
    return instance;
}