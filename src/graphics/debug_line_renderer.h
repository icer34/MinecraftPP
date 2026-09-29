#pragma once

#include <vector>

#include "graphics/gl/gl_objects.h"
#include "graphics/shader.h"

class DebugDraw;

class DebugLineRenderer
{
public:
    DebugLineRenderer();

    /// Draws the lines of `shapes` into the bound framebuffer. FrameData must be uploaded.
    void draw(const DebugDraw &shapes);

private:
    // (re)creates the buffer if it holds fewer than `vertices` vertices
    void reserve(size_t vertices);

    GLVertexArray _vao;
    GLBuffer _vbo;
    size_t _capacity = 0; // in vertices
    Shader _shader;
};
