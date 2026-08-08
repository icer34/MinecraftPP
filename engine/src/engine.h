#pragma once

// single entry point for anything consuming the engine (a game, tests/, etc.) -- include
// this instead of individual engine headers. Keep this list in sync whenever a new public
// engine header is added.

// app
#include "app/application.h"
#include "app/application_interface.h"

// core
#include "core/block_registry.h"
#include "core/chunk.h"
#include "core/chunk_attrib_registry.h"
#include "core/default_terrain_generator.h"
#include "core/entity.h"
#include "core/settings_registry.h"
#include "core/terrain_generator.h"
#include "core/world.h"

// graphics
#include "graphics/block_texture_atlas.h"
#include "graphics/camera.h"
#include "graphics/cascaded_shadow_map.h"
#include "graphics/frame_buffer.h"
#include "graphics/renderer.h"
#include "graphics/shader.h"
#include "graphics/texture.h"

// graphics/hud
#include "graphics/hud/hud.h"
#include "graphics/hud/hud_renderer.h"
#include "graphics/hud/settings_menu.h"

// graphics/mesh
#include "graphics/mesh/block_outline.h"
#include "graphics/mesh/chunk_mesh.h"
#include "graphics/mesh/chunk_mesher.h"
#include "graphics/mesh/drawable.h"
#include "graphics/mesh/mesh.h"

// util
#include "util/aabb.h"
#include "util/directions.h"
#include "util/frustum.h"
#include "util/key_codes.h"
#include "util/perlin_noise.h"
#include "util/raycaster.h"
#include "util/spline.h"
#include "util/uv_rect.h"
#include "util/window.h"
