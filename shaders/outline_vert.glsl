#version 460 core

#include "common/frame_data.glsl"

// minimum corner of the outlined block, in world space
layout (location = 0) uniform vec3 blockPos;

// width of the outline, in pixels
const float WIDTH_PX = 3.0;
// pulls the outline toward the camera by this fraction of its distance. A point moved along
// its own view ray keeps its screen position, so this is invisible, but the outline then wins
// the depth test against the block's faces, which the edge quads half overlap.
const float DEPTH_PULL = 0.995;

// the 8 corners of a unit cube: index = x + 2y + 4z
const vec3 CORNERS[8] = vec3[8](
    vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0),
    vec3(0, 0, 1), vec3(1, 0, 1), vec3(0, 1, 1), vec3(1, 1, 1)
);

// the 12 edges of the cube, as pairs of corner indices
const ivec2 EDGES[12] = ivec2[12](
    // along X
    ivec2(0, 1), ivec2(2, 3), ivec2(4, 5), ivec2(6, 7),
    // along Y
    ivec2(0, 2), ivec2(1, 3), ivec2(4, 6), ivec2(5, 7),
    // along Z
    ivec2(0, 4), ivec2(1, 5), ivec2(2, 6), ivec2(3, 7)
);

// every edge is drawn as a quad of 2 triangles. For each of its 6 vertices: x = which end of
// the edge (0 = start, 1 = end), y = which side of the edge (-1 or 1)
const vec2 QUAD[6] = vec2[6](
    vec2(0, -1), vec2(1, -1), vec2(1, 1),
    vec2(0, -1), vec2(1, 1), vec2(0, 1)
);

vec4 toClip(vec3 worldPos)
{
    vec4 viewPos = view * vec4(worldPos, 1.0);
    viewPos.xyz *= DEPTH_PULL;
    return projection * viewPos;
}

void main()
{
    ivec2 edge = EDGES[gl_VertexID / 6];
    vec2 quadVertex = QUAD[gl_VertexID % 6];

    vec4 a = toClip(blockPos + CORNERS[edge.x]);
    vec4 b = toClip(blockPos + CORNERS[edge.y]);

    // clip the edge against the near plane (visible points have z <= w with reverse-Z): an
    // end behind the camera would break the perspective division below
    float distA = a.w - a.z;
    float distB = b.w - b.z;
    if (distA < 0.0 && distB < 0.0)
    {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // outside the clip volume: nothing is drawn
        return;
    }
    if (distA < 0.0)
        a = mix(a, b, distA / (distA - distB));
    else if (distB < 0.0)
        b = mix(b, a, distB / (distB - distA));

    // direction of the edge on screen, in pixels
    vec2 screenA = a.xy / a.w * 0.5 * screenSize;
    vec2 screenB = b.xy / b.w * 0.5 * screenSize;
    vec2 dir = screenB - screenA;
    dir = length(dir) > 1e-4 ? normalize(dir) : vec2(1.0, 0.0);
    vec2 normal = vec2(-dir.y, dir.x);

    // widen the edge into a quad, and extend it by half a width at both ends so that the
    // edges join at the corners
    vec4 pos = quadVertex.x == 0.0 ? a : b;
    float along = quadVertex.x == 0.0 ? -1.0 : 1.0;
    vec2 offsetPx = (normal * quadVertex.y + dir * along) * WIDTH_PX * 0.5;

    // pixels -> NDC (2 units over screenSize pixels) -> clip space
    pos.xy += offsetPx * 2.0 / screenSize * pos.w;
    gl_Position = pos;
}
