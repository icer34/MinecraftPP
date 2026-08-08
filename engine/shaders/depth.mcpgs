#version 330 core

// no "invocations" qualifier -- that needs GLSL 400/GL_ARB_gpu_shader5, not available in
// plain #version 330 core. Instead, a single invocation loops over every cascade itself and
// emits a full triangle (3 vertices) per cascade, directed to the matching array layer.
layout(triangles) in;
layout(triangle_strip, max_vertices = 15) out; // 5 cascades * 3 vertices

uniform mat4 lightSpaceMatrices[5];

void main()
{
    for (int layer = 0; layer < 5; layer++)
    {
        for (int i = 0; i < 3; i++)
        {
            gl_Position = lightSpaceMatrices[layer] * gl_in[i].gl_Position;
            gl_Layer = layer;
            EmitVertex();
        }
        EndPrimitive();
    }
}
