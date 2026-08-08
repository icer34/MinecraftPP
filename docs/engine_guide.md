# Engine usage guide

Practical notice: how to use what exists today, not how it got built. For the design history
and rationale, see `shader_pipeline_architecture.md`.

For now this guide covers: shaders, uniforms, and chunk vertex data (how it works + how to add
an attribute).

---

## Shaders

### Roles, not file paths

`Renderer` (and any code loading a shader) never asks for a file by path, only by **role**:
`"block"`, `"water"`, `"depth"`, `"sky"`, ... The role is the stable contract between engine and
game.

```cpp
Shader shader;
shader.load("water"); // resolves the "water" role to an actual .mcpvs/.mcpfs file
```

`Shader::load(role)` resolves `role + ".mcpvs"` / `role + ".mcpfs"` / `role + ".mcpgs"` (if
present) by searching, in this order:

1. `game/shaders/`
2. `engine/shaders/`

The first one that exists wins. So a game can **override** the engine's default shader for a
given role just by dropping a file at `game/shaders/<role>.mcpvs` -- nothing to change in
`Renderer`.

### Extensions

`.mcpvs` / `.mcpfs` / `.mcpgs` (vertex/fragment/geometry MinecraftPP Shader). It's still GLSL,
with a custom preprocessor on top (see below) -- the custom extensions just distinguish "goes
through our preprocessor" from raw `.glsl`.

### `#include`

```glsl
#include "sky.mcpis"
```

Resolved recursively before compilation, with cycle detection. Two kinds of include are worth
knowing about:

- A regular file (resolved through the same `game/shaders/` / `engine/shaders/` paths as shaders
  themselves).
- `#include "chunk_vertex_format"` -- a **virtual include**: no file with that name exists on
  disk, it's intercepted by `resolveShaderSource` and replaced with GLSL generated on the fly
  (see the vertex data section below).

Watch out: declaring the same variable/uniform twice within a **single stage** (e.g. if two
included files both declare `uniform vec3 lightDir;`) is a compile error (`redefinition`). This
is never an issue between the vertex and fragment shader though -- those are two separately
compiled programs, redeclaring a uniform on both sides is normal and often required.

---

## Uniforms

### `UniformManager` (singleton)

`UniformManager::instance()` is a plain value store, independent of any particular shader:

```cpp
auto &uniforms = UniformManager::instance();
uniforms.setValue("time", m_window.getTime());
uniforms.setValue("view", cam.getViewMatrix());
```

To send the values to a shader:

```cpp
shader.bind();
uniforms.applyTo(shader);
```

`applyTo` walks that specific shader's **active** uniforms (cached by `Shader::load` via
`glGetActiveUniform`) and, for each one, looks up the value registered under the same name in
the manager. If a uniform is active in the shader but was never `setValue`'d, `applyTo` throws --
this surfaces forgotten uniforms immediately instead of silently leaving a shader
under-fed.

Supported types (see `UniformValue` in `shader.h`): `float`, `int`, `glm::vec2`, `glm::vec3`,
`glm::mat4`, `std::vector<float>`, `std::vector<glm::mat4>`.

### Why an "active" uniform can go missing

GLSL dead-code-eliminates any `uniform` that's declared but never actually read by the shader's
final computation -- it simply won't show up in `GL_ACTIVE_UNIFORMS`, even though it's right
there in the compiled source. Concretely: a declared-but-unused `uniform float time;` won't be
active, and `applyTo` won't even attempt to push it (no error, no effect). If a uniform needs to
be active, it needs to actually influence some output of the shader.

### The same value, pushed differently per shader

The "last value sent" for a given uniform is tracked **per `Shader` instance**
(`Shader::m_activeUniformValues`), not inside `UniformManager`. That means two shaders can both
have an active `projection` uniform with different values (2D orthographic for the HUD, 3D
perspective for world rendering) without stepping on each other -- just call
`setValue("projection", ...)` with the right value right before each shader's `applyTo`.
`applyTo` only issues an actual `glUniform*` call when the value differs from what **that
specific shader** last received (`Shader::setIfChanged`), so no redundant GL calls frame after
frame if nothing changed for it.

---

## Chunk vertex data

### The packed format

Each chunk vertex is a single GLSL vertex attribute, a `uvecN` (`N` = 2, 3, or 4 depending on
how many chunk attributes are enabled -- see below):

```glsl
layout (location = 0) in uvecN packedData;
```

`packedData[0]` ("data1") has a **fixed** format, always the same regardless of `N`:

| field        | bits    | width |
|--------------|---------|-------|
| `chunkX`     | 28-31   | 4     |
| `chunkY`     | 20-27   | 8     |
| `chunkZ`     | 16-19   | 4     |
| `normalIdx`  | 13-15   | 3     |
| `textureIdx` | 1-12    | 12    |
| `isTinted`   | 0       | 1     |

`packedData[1]` and beyond hold, in order, `cornerIdx` (2 bits), then `aoValue` (2 bits), then
one byte per **enabled** chunk attribute, in `ChunkAttribRegistry::getEnabledAttribs()` order,
spilling into the next word(s) once it no longer fits in 32 bits.

Packing (CPU side, `ChunkMesher::addSolidFace` in `chunk_mesher.cpp`) and unpacking (generated
GLSL, `generateVertexFormat()` in `shader.cpp`) both use the same bit-cursor algorithm
(`packBits`/`unpackBits`) in the same order -- that's what keeps them in sync. If one side of the
format ever changes, the other has to change to match.

### `#include "chunk_vertex_format"`

Instead of hand-writing the unpacking, any `.mcpvs` that needs chunk data just does:

```glsl
#version 330 core

#include "chunk_vertex_format"

void main()
{
    // chunkX, chunkY, chunkZ, normalIdx, textureIdx, isTinted, cornerIdx, aoValue,
    // plus one uint per enabled chunk attribute (e.g. temperature, humidity), are all here.
    vec3 chunkPos = vec3(float(chunkX), float(chunkY), float(chunkZ));
    ...
}
```

See `block.mcpvs`, `water.mcpvs`, `depth.mcpvs` for real examples.

### The cap

4 `uint32` per vertex maximum (`ChunkAttribRegistry::computeVertexWordCount()` computes it,
`enableAttrib` refuses to enable an attribute that would exceed this) -- GLSL has no type beyond
`uvec4`.

---

## Adding a chunk attribute

Example: adding a `lightLevel` attribute (uint8, per block).

**1. Register and enable it**, on the game side, before `Window`/`Renderer` exist (in `Game`'s
constructor, same as `temperature`/`humidity` in `game.cpp`):

```cpp
auto &reg = ChunkAttribRegistry::instance();
reg.registerAttrib<uint8_t>("lightLevel", AttribScope::PerBlock, /*default*/ 15);
reg.enableAttrib("lightLevel");
```

- `registerAttrib` alone makes the attribute usable from `Chunk` (CPU-side storage), but
  invisible to shaders.
- `enableAttrib` additionally makes it show up in the vertex packing -- i.e. in
  `chunk_vertex_format`. Only `uint8_t` attributes can be enabled (each takes exactly one packed
  byte).
- `AttribScope::PerColumn` (one value per column `(x,z)`, like `temperature`/`humidity`) vs.
  `AttribScope::PerBlock` (one value per block `(x,y,z)`) determines the position used to
  read/write the value.

**2. Read/write it from game code** (terrain generation, gameplay, ...):

```cpp
chunk.set<uint8_t>("lightLevel", glm::ivec3(x, y, z), 8);
uint8_t light = chunk.get<uint8_t>("lightLevel", glm::ivec3(x, y, z));
```

(`glm::ivec2` instead of `glm::ivec3` for a `PerColumn` attribute.)

**3. Use it in a shader** -- nothing else to do. Any `.mcpvs` that already does
`#include "chunk_vertex_format"` now has direct access to `uint lightLevel`:

```glsl
float brightness = float(lightLevel) / 15.0;
```

The order attributes are `enableAttrib`'d in determines their packing order -- no manual
bookkeeping needed to stay in sync, since `chunk_vertex_format` and the mesher both read
`getEnabledAttribs()` in the same order.
