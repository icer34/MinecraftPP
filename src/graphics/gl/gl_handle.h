#pragma once

#include <glad/glad.h>
#include <utility>

/**
 * @brief Owns an OpenGL object name and deletes it on destruction.
 * Move-Only: the OpenGL object has exactly one owner
 */
template <void (*DeleteFn)(GLuint)> class GLHandle
{
public:
    GLHandle() = default;
    explicit GLHandle(GLuint id)
        : _id(id) {};
    ~GLHandle() { reset(); }

    GlHandle(const GlHandle &) = delete;
    GlHandle &operator=(const GlHandle &) = delete;

    GlHandle(GlHandle &&other) noexcept
        : _id(std::exchange(other._id, 0))
    {
    }
    GlHandle &operator=(GlHandle &&other) noexcept
    {
        if (this != &other)
        {
            reset();
            _id = std::exchange(other._id, 0);
        }
        return *this;
    }

    GLuint id() const { return _id; }
    explicit operator bool() const { return _id != 0; }

    void reset()
    {
        if (_id != 0)
            DeleteFn(_id);
        _id = 0;
    }

private:
    GLuint _id = 0;
};