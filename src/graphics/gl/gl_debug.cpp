#include "gl_debug.h"

#include <iostream>

namespace
{
const char *sourceName(GLenum source)
{
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "window system";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "shader compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "third party";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "application";
    default:
        return "other";
    }
}

const char *typeName(GLenum type)
{
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        return "error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "deprecated";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "undefined behavior";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "portability";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "performance";
    case GL_DEBUG_TYPE_MARKER:
        return "marker";
    default:
        return "other";
    }
}

const char *severityName(GLenum severity)
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        return "HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "MEDIUM";
    case GL_DEBUG_SEVERITY_LOW:
        return "LOW";
    default:
        return "NOTIFICATION";
    }
}
} // namespace

void APIENTRY gl::onDebugMessage(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei /*length*/,
                                 const GLchar *message,
                                 const void * /*userParam*/)
{
    // our own debug groups also emit push/pop messages -- they are only meant for RenderDoc
    if (type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP)
        return;

    std::cerr << "[GL " << severityName(severity) << "] " << typeName(type) << " ("
              << sourceName(source) << ", id " << id << "): " << message << std::endl;

    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        // put a breakpoint here: with GL_DEBUG_OUTPUT_SYNCHRONOUS, the call stack leads to
        // the faulty GL call
    }
}

void gl::setLabel(GLenum identifier, GLuint id, std::string_view label)
{
    glObjectLabel(identifier, id, static_cast<GLsizei>(label.size()), label.data());
}

gl::DebugGroup::DebugGroup(std::string_view name)
{
    glPushDebugGroup(
        GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<GLsizei>(name.size()), name.data());
}

gl::DebugGroup::~DebugGroup() { glPopDebugGroup(); }
