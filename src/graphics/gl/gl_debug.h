/**
 * @file gl_debug.h
 * @brief OpenGL debug output, object labels and debug groups.
 */

#pragma once

#include <glad/glad.h>

#include <string_view>

namespace gl
{
/**
 * @brief Prints the messages of the OpenGL debug output (errors, deprecated usage, performance
 * warnings) to stderr.
 *
 * Installed by Window in Debug builds, with synchronous output: the callback runs inside the
 * faulty GL call, so a breakpoint on the high severity branch gives the call stack of the
 * error.
 */
void APIENTRY onDebugMessage(GLenum source,
                             GLenum type,
                             GLuint id,
                             GLenum severity,
                             GLsizei length,
                             const GLchar *message,
                             const void *userParam);

/**
 * @brief Names an OpenGL object. The name shows up in the debug output and in RenderDoc.
 *
 * @param identifier kind of object (`GL_BUFFER`, `GL_TEXTURE`, `GL_PROGRAM`...)
 * @param id OpenGL name of the object
 * @param label name to give it
 */
void setLabel(GLenum identifier, GLuint id, std::string_view label);

/**
 * @brief Opens a named debug group for the lifetime of the object (RAII).
 *
 * Every GL command issued while the group is open is shown under that name in RenderDoc, as
 * a collapsible section. Almost free: groups stay enabled in every build.
 */
class DebugGroup
{
public:
    explicit DebugGroup(std::string_view name);
    ~DebugGroup();

    DebugGroup(const DebugGroup &) = delete;
    DebugGroup &operator=(const DebugGroup &) = delete;
};
} // namespace gl
