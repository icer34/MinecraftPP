#include "frame_data.h"

#include "graphics/gl/gl_debug.h"

FrameDataBuffer::FrameDataBuffer()
    : _buffer(gl::createBuffer())
{
    glNamedBufferStorage(_buffer.id(), sizeof(FrameData), nullptr, GL_DYNAMIC_STORAGE_BIT);
    gl::setLabel(GL_BUFFER, _buffer.id(), "Frame data");
}

void FrameDataBuffer::upload(const FrameData &data)
{
    glNamedBufferSubData(_buffer.id(), 0, sizeof(FrameData), &data);
    // rebound every frame (it's cheap) rather than once at creation, so that nothing else
    // binding a buffer to the same point can silently break every shader
    glBindBufferBase(GL_UNIFORM_BUFFER, FRAME_DATA_BINDING, _buffer.id());
}
