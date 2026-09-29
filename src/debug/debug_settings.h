#pragma once

/**
 * @brief Runtime flags of the debug tools, not in the SettingsMenu (debug tools only available
 * through kayboard shortcuts)
 */
struct DebugSettings
{
    bool showPanel = false; ///< F3 panel
    bool showHelp = false;  ///< Shortcut binds list in the panel

    bool wireframe = false;
    bool showChunkBorders = false;
    bool showEntityHitBoxes = false;
    bool showRayCast = false;
    bool freezeCuling = false;

    bool showPerfGraphs = false;
    bool gpuTimings = false;

    //... might grow as more debug tools may be needed
};

/// Global access fromn everywhere in the game
inline DebugSettings &debugSettings()
{
    static DebugSettings instance;
    return instance;
}