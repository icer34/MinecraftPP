#version 330 core

// see ChunkMesher::mesh() in chunk_mesher.cpp to see the packing format in detail
layout (location = 0) in uvec2 packedData;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform sampler2D colormap;

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vTint;
out float vAO;

// AO occlusion count (0..3) -> brightness factor. index 0 = no occluding neighbors (full light),
// index 3 = corner fully enclosed (darkest). see ChunkMesher::cornerAO in chunk_mesher.cpp.
const float AO_LEVELS[4] = float[4](1.0, 0.75, 0.5, 0.25);

// mirrors the Direction enum order in directions.h (NORTH, SOUTH, EAST, WEST, TOP, BOTTOM)
const vec3 NORMALS[6] = vec3[6](
    vec3(0.0,  0.0, -1.0), // NORTH
    vec3(0.0,  0.0,  1.0), // SOUTH
    vec3(1.0,  0.0,  0.0), // EAST
    vec3(-1.0, 0.0,  0.0), // WEST
    vec3(0.0,  1.0,  0.0), // TOP
    vec3(0.0, -1.0,  0.0)  // BOTTOM
);

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

// mirrors the Corner enum in chunk_mesher.cpp (BOTTOM_LEFT, TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT)
const vec2 CORNER_UV[4] = vec2[4](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

// must match BlockTextureAtlas::TEXTURE_SIZE / ATLAS_SIZE in block_texture_atlas.h
const uint ATLAS_COLUMNS = 64u;
const float CELL_UV_SIZE = 16.0 / 1024.0;

vec2 uvFromTextureIndex(uint textureIdx, uint cornerIdx)
{
    uint cellCol = textureIdx % ATLAS_COLUMNS;
    uint cellRow = textureIdx / ATLAS_COLUMNS;
    vec2 cellOrigin = vec2(float(cellCol), float(cellRow)) * CELL_UV_SIZE;
    return cellOrigin + CORNER_UV[cornerIdx] * CELL_UV_SIZE;
}

vec3 sampleColorMap(uint humidity, uint temperature)
{
    float ftemp = float(temperature) / 255.0;
    float fhum = float(humidity) / 255.0;

    float adjustedHum = fhum * ftemp;
    vec2 uv = vec2(ftemp, 1.0 - adjustedHum);
    return texture(colormap, uv).rgb;
}

void main()
{
    uint data1 = packedData.x;
    uint data2 = packedData.y;

    uint chunkX = (data1 >> 28) & 0xFu;
    uint chunkY = (data1 >> 20) & 0xFFu;
    uint chunkZ = (data1 >> 16) & 0xFu;
    uint normalIdx = (data1 >> 13) & 0x7u; //* also encodes the direction, thus the face Idx.
    uint textureIdx = (data1 >> 1) & 0xFFFu;
    bool isTinted = bool(data1 & 0x1u);

    uint temperature = (data2 >> 24) & 0xFFu;
    uint humidity = (data2 >> 16) & 0xFFu;
    uint cornerIdx = (data2 >> 14) & 0x3u;
    uint aoValue = (data2 >> 12) & 0x3u;

    vec3 chunkPos = vec3(float(chunkX), float(chunkY), float(chunkZ));
    vec3 facePos = chunkPos + FACE_CORNER_OFFSET[normalIdx * 4u + cornerIdx];

    vNormal = NORMALS[normalIdx];
    vTexCoord = uvFromTextureIndex(textureIdx, cornerIdx);
    vTint = isTinted ? sampleColorMap(humidity, temperature) : vec3(1.0);
    vAO = AO_LEVELS[aoValue];

    gl_Position = projection * view * model * vec4(facePos, 1.0);
}
