// per-frame uniforms shared by every shader, uploaded once per frame by the Renderer.
// must match struct FrameData in src/graphics/frame_data.h (std140 layout, checked there by
// static_asserts): change both together.

const int CASCADE_COUNT = 5; // CascadedShadowMap::CASCADE_COUNT

layout(std140, binding = 0) uniform FrameData
{
    mat4 view;
    mat4 projection;
    mat4 invView;
    mat4 invProjection;
    mat4 lightSpaceMatrices[CASCADE_COUNT];
    vec4 cutoffDistPacked[2]; // 4 cascades per vec4: a float[] would take 16 bytes per element
    vec3 lightDir;            // direction the sunlight travels in
    float time;               // seconds since startup
    vec3 camPos;
    float zNear;
    vec2 screenSize; // in pixels
    float zFar;
};

// view-space far distance of the cascade i
float cutoffDist(int i)
{
    return cutoffDistPacked[i / 4][i % 4];
}
