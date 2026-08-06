#pragma once

#include <string>

#include "application_interface.h"
#include "graphics/renderer.h"
#include "util/window.h"

// owns the window, the renderer and the main loop -- drives an IApplication instead of
// letting the game manage any of that itself, so any game built on this engine gets the
// same loop/window/renderer setup for free.
class Application
{
public:
    Application(IApplication &app, int width, int height, const std::string &title);

    void run();

private:
    Window m_window;
    Renderer m_renderer;
    IApplication &m_app;
};
