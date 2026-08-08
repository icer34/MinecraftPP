#pragma once

class Window;
class Renderer;
class World;

// lifecycle a game implements to run on top of the engine -- the engine owns the window
// and the main loop (see Application), the game only reacts to these calls.
class IApplication
{
public:
    virtual ~IApplication() = default;

    // called before the window/renderer exist, so Application can construct the
    // renderer against this world -- must be safe to call before init()
    virtual World &getWorld() = 0;

    // called once, right after the engine has created the window and renderer
    virtual void init(Window &window, Renderer &renderer) = 0;

    virtual void processInput(Window &window) = 0;
    virtual void update(float dt) = 0;
    virtual void render(float dt, Window &window) = 0;
};
