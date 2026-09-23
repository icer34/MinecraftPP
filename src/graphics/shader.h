/**
 * @file shader.h
 * @brief OpenGL shader program wrapper.
 */

#pragma once

#include <glm/glm.hpp>
#include <string>

/**
 * @brief Compiles, links and owns an OpenGL shader program.
 *
 * The uniform setters look the uniform up by name on every call and print an error if it
 * cannot be found. The GLSL compiler removes uniforms that do not affect the output, so a
 * declared but unused uniform also triggers that error.
 *
 * Must be created and destroyed while a GL context is current.
 */
class Shader
{
public:
    /**
     * @brief Loads, compiles and links a vertex + fragment shader program.
     *
     * @param vertPath path to the vertex shader source file
     * @param fragPath path to the fragment shader source file
     * @throws std::runtime_error if a file cannot be read, or if compilation or linking fails
     */
    Shader(const char *vertPath, const char *fragPath);

    /**
     * @brief Deletes the program and its shader objects.
     */
    ~Shader();

    /**
     * @brief Compiles a geometry shader and relinks the program with it.
     *
     * @param path path to the geometry shader source file
     * @throws std::runtime_error if the file cannot be read, or if compilation or linking fails
     */
    void addGeometryShader(const char *path);

    /**
     * @brief Makes this program the active one (`glUseProgram`).
     *
     * The setters below apply to the active program, so call this first.
     */
    void use();

    /**
     * @brief Uploads an array of matrices to the uniform array `name[]`.
     */
    void setMat4Array(const std::string &name, const std::vector<glm::mat4> &value);
    /** @brief Sets a `mat4` uniform. */
    void setMat4(const std::string &name, glm::mat4 value);
    /** @brief Sets a `vec3` uniform. */
    void setVec3(const std::string &name, glm::vec3 value);
    /** @brief Sets an `int` uniform (also used for sampler texture units). */
    void setInt(const std::string &name, int value);
    /** @brief Sets a `float` uniform. */
    void setFloat(const std::string &name, float value);
    /**
     * @brief Uploads an array of floats to the uniform array `name[]`.
     */
    void setFloatArray(const std::string &name, const std::vector<float> &value);

private:
    unsigned int _programID;
    unsigned int _vertID;
    unsigned int _fragID;
    unsigned int _geomID = 0;
};