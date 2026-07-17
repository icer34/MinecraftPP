#version 330 core

// see ChunkMesher::mesh() in chunk_mesher.cpp to see the packing format in detail
layout (location = 0) in uvec2 packedData;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

// mirrors CUBE_FACE_CORNERS in chunk_mesher.cpp: for each direction, the position offset
// (0/1 per axis) of each corner (BOTTOM_LEFT, TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT), flattened
// to a 1D array (index = normalIdx * 4 + cornerIdx) since GLSL 330 doesn't support arrays
// of arrays. Keep this in sync if the winding ever changes CPU-side.
const vec3 FACE_CORNER_OFFSET[24] = vec3[24](
    // NORTH
    vec3(0, 0, 0), vec3(0, 1, 0), vec3(1, 1, 0), vec3(1, 0, 0),
    // SOUTH
    vec3(0, 0, 1), vec3(0, 1, 1), vec3(1, 1, 1), vec3(1, 0, 1),
    // EAST
    vec3(1, 0, 0), vec3(1, 1, 0), vec3(1, 1, 1), vec3(1, 0, 1),
    // WEST
    vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 1, 1), vec3(0, 0, 1),
    // TOP
    vec3(0, 1, 0), vec3(0, 1, 1), vec3(1, 1, 1), vec3(1, 1, 0),
    // BOTTOM
    vec3(0, 0, 0), vec3(0, 0, 1), vec3(1, 0, 1), vec3(1, 0, 0)
);

void main()
{
    uint data1 = packedData.x;
    uint data2 = packedData.y;

    uint chunkX = (data1 >> 28) & 0xFu;
    uint chunkY = (data1 >> 20) & 0xFFu;
    uint chunkZ = (data1 >> 16) & 0xFu;
    uint normalIdx = (data1 >> 13) & 0x7u; //* also encodes the direction, thus the face Idx.
    uint cornerIdx = (data2 >> 14) & 0x3u;

    vec3 chunkPos = vec3(float(chunkX), float(chunkY), float(chunkZ));
    vec3 facePos = chunkPos + FACE_CORNER_OFFSET[normalIdx * 4u + cornerIdx];

    gl_Position = lightSpaceMatrix * model * vec4(facePos, 1.0);
}
