// single translation unit providing stb_image_write's implementation for engine_lib --
// linked into both MinecraftPP.exe and tests.exe, so any code in src/ can call
// stbi_write_png without each executable needing its own copy of the implementation.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
