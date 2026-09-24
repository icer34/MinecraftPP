#include "gl_objects.h"

#include <cassert>
#include <thread>

namespace
{
std::thread::id contextThread;

void assertContextThread()
{
    assert(std::this_thread::get_id() == contextThread
           && "OpenGL objects must be created and destroyed on the context thread");
}
} // namespace

void gl::setContextThread() { contextThread = std::this_thread::get_id(); }

void gl::deleteBuffer(GLuint id)
{
    assertContextThread();
    glDeleteBuffers(1, &id);
}
void gl::deleteVertexArray(GLuint id)
{
    assertContextThread();
    glDeleteVertexArrays(1, &id);
}
void gl::deleteTexture(GLuint id)
{
    assertContextThread();
    glDeleteTextures(1, &id);
}
void gl::deleteFramebuffer(GLuint id)
{
    assertContextThread();
    glDeleteFramebuffers(1, &id);
}
void gl::deleteQuery(GLuint id)
{
    assertContextThread();
    glDeleteQueries(1, &id);
}

GLBuffer gl::createBuffer()
{
    assertContextThread();
    GLuint id = 0;
    glCreateBuffers(1, &id);
    return GLBuffer(id);
}

GLTexture gl::createTexture(GLenum target)
{
    assertContextThread();
    GLuint id = 0;
    glCreateTextures(target, 1, &id);
    return GLTexture(id);
}

GLVertexArray gl::createVertexArray()
{
    assertContextThread();
    GLuint id = 0;
    glCreateVertexArrays(1, &id);
    return GLVertexArray(id);
}

GLFramebuffer gl::createFramebuffer()
{
    assertContextThread();
    GLuint id = 0;
    glCreateFramebuffers(1, &id);
    return GLFramebuffer(id);
}

GLQuery gl::createQuery(GLenum target)
{
    assertContextThread();
    GLuint id = 0;
    glCreateQueries(target, 1, &id);
    return GLQuery(id);
}