/**
 * @file shader.h
 * @brief OpenGL shader program wrapper.
 */

#pragma once

#include <glm/glm.hpp>

#include <string>
#include <string_view>
#include <unordered_map>

#include "graphics/gl/gl_objects.h"

/**
 * @brief Compiles, links and owns an OpenGL shader program.
 *
 * Shader sources can include other files with `#include "path"`, relative to the including
 * file (e.g. `#include "common/frame_data.glsl"`). Each file is inserted at most once, and
 * `#line` directives keep the line numbers of compiler errors meaningful: the error message
 * lists which source string number is which file.
 *
 * Uniforms are set with `glProgramUniform*`, so the program does not need to be active. The
 * name-based setters cache each location on first use, and print an error once if the uniform
 * does not exist. The GLSL compiler removes uniforms that do not affect the output, so a
 * declared but unused uniform also triggers that error. Uniforms set in a hot loop should use
 * a fixed `layout(location = N)` and the location-based setters instead.
 *
 * Move-only. Must be created and destroyed while a GL context is current.
 */
class Shader
{
public:
    /**
     * @brief Loads, compiles and links a vertex + fragment (+ optional geometry) program.
     *
     * @param vertPath path to the vertex shader source file
     * @param fragPath path to the fragment shader source file
     * @param geomPath path to the geometry shader source file, or nullptr for none
     * @throws std::runtime_error if a file cannot be read, or if compilation or linking fails
     */
    Shader(const char *vertPath, const char *fragPath, const char *geomPath = nullptr);

    /**
     * @brief Makes this program the active one (`glUseProgram`), for the next draw calls.
     */
    void use() const;

    /** @brief OpenGL name of the program. */
    GLuint id() const { return _program.id(); }

    /** @brief Sets a `mat4` uniform. */
    void setMat4(std::string_view name, const glm::mat4 &value);
    /** @brief Sets a `vec3` uniform. */
    void setVec3(std::string_view name, glm::vec3 value);
    /** @brief Sets an `int` uniform. */
    void setInt(std::string_view name, int value);
    /** @brief Sets a `float` uniform. */
    void setFloat(std::string_view name, float value);

    /** @brief Sets the `vec3` uniform declared with `layout(location = location)`. */
    void setVec3(GLint location, glm::vec3 value);

private:
    GLint uniformLocation(std::string_view name);

    GLProgram _program;

    // transparent hash: lookups with a string_view (or a literal) don't build a std::string
    struct StringHash
    {
        using is_transparent = void;
        size_t operator()(std::string_view s) const { return std::hash<std::string_view>{}(s); }
    };
    std::unordered_map<std::string, GLint, StringHash, std::equal_to<>> _uniformLocations;
};
