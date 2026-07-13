#pragma once

#include <memory>

struct World;
struct Shader;
struct Camera;

class Renderer
{
  public:
    Renderer();
    ~Renderer();
    void renderWorld(const World &world, const Camera &cam);

  private:
    std::unique_ptr<Shader> m_shader;
};