#include "application.h"

Application::Application(IApplication &app, int width, int height, const std::string &title)
    : m_window(width, height, title, false),
      m_renderer(m_window, app.getWorld()),
      m_app(app)
{
    m_app.init(m_window, m_renderer);
}

void Application::run()
{
    float lastFrameTime = m_window.getTime();

    while (!m_window.shouldClose())
    {
        float currentTime = m_window.getTime();
        float dt = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        m_window.pollEvents();
        m_app.processInput(m_window);
        m_app.update(dt);
        m_app.render(dt, m_window);
        m_window.swapBuffers();
    }
}
