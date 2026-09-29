#include "gpu_timer.h"

GpuTimer::GpuTimer()
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        _startQueries[i] = gl::createQuery(GL_TIMESTAMP);
        _endQueries[i] = gl::createQuery(GL_TIMESTAMP);
    }
}

void GpuTimer::begin()
{
    if (_pending[_current])
    {
        // the end query completes last: once it is available, both results are
        GLint available = 0;
        glGetQueryObjectiv(_endQueries[_current].id(), GL_QUERY_RESULT_AVAILABLE, &available);

        // not ready even after FRAMES_IN_FLIGHT frames (very GPU bound): the measure is
        // dropped rather than waited for, and _lastTime keeps the previous one
        if (available)
        {
            GLuint64 start = 0, end = 0;
            glGetQueryObjectui64v(_startQueries[_current].id(), GL_QUERY_RESULT, &start);
            glGetQueryObjectui64v(_endQueries[_current].id(), GL_QUERY_RESULT, &end);
            _lastTime = static_cast<float>(end - start) * 1e-9f; // nanoseconds to seconds
        }
        _pending[_current] = false;
    }

    glQueryCounter(_startQueries[_current].id(), GL_TIMESTAMP);
}

void GpuTimer::end()
{
    glQueryCounter(_endQueries[_current].id(), GL_TIMESTAMP);
    _pending[_current] = true;
    _current = (_current + 1) % FRAMES_IN_FLIGHT;
}
