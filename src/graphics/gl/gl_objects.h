#pragma once

#include "gl_handle.h"

namespace gl
{
inline void deleteBuffer(GLuint id);
inline void deleteVertexArray(GLuint id);
inline void deleteTexture(GLuint id);
inline void deleteFramebuffer(GLuint id);
inline void deleteQuery(GLuint id);
} // namespace gl

using GLBuffer = GLHandle<gl::deleteBuffer>;
using GLVertexArray = GLHandle<gl::deleteVertexArray>;
using GLTexture = GLHandle<gl::deleteTexture>;
using GLFramebuffer = GLHandle<gl::deleteFramebuffer>;
using GLQuery = GLHandle<gl::deleteQuery>;

namespace gl
{
/// Records the calling thread as the main thread owning the openGL context. Must be called once at
/// the context creation
void setContextThread();

// creation funcitons using openGL 4.5+ DSA
GLBuffer createBuffer();
GLVertexArray createVertexArray();
GLTexture createTexture(GLenum target); // GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, ...
GLFramebuffer createFramebuffer();
GLQuery createQuery(GLenum target); // GL_TIMESTAMP, ...

} // namespace gl
