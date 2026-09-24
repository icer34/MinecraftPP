// texture unit of every sampler -- must match the TextureUnit enum in
// src/graphics/gl/texture_units.h. Use as: layout(binding = TEX_UNIT_BLOCK_ATLAS) uniform ...

#define TEX_UNIT_BLOCK_ATLAS 0
#define TEX_UNIT_BLOCK_COLORMAP 1
#define TEX_UNIT_SHADOW_MAP 2
#define TEX_UNIT_SOLID_COLOR 3
#define TEX_UNIT_SOLID_DEPTH 4
#define TEX_UNIT_HUD_ATLAS 5
