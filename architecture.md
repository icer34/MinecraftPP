# Revue de l'architecture des fichiers

État des lieux de l'organisation de `src/`, avec les problèmes relevés et la réorganisation qui en découle. Les points 1, 2, 3 et 5 sont appliqués ; le point 4 reste à faire (voir [Suivi](#suivi)).

## Ce qui fonctionne bien

- **Le découpage principal est clair** : `game/` (simulation), `graphics/` (rendu), `debug/`, `util/`, avec `game.h/.cpp` comme point d'assemblage à la racine, à côté de `main.cpp`.
- **`graphics/gl/` isole la couche OpenGL bas niveau** (handles RAII, debug output, `GpuTimer`) des renderers qui l'utilisent.
- **Le nommage est cohérent** : fichiers en snake_case, une paire `.h/.cpp` par classe, un en-tête Doxygen `@file` dans chaque fichier.

## Problèmes relevés

### 1. `graphics/hud/` mélange deux niveaux

Le dossier contient :

- **`HudRenderer`**, qui sait *comment* dessiner des quads 2D à l'écran (buffers, shaders, atlas) : c'est du rendu ;
- **`Hud` et `SettingsMenu`**, qui décident *quoi* afficher et gèrent la navigation dans les menus : c'est de la logique d'interface.

Le nom `hud` est par ailleurs trop étroit : le menu des paramètres n'est pas un HUD. `2d` ne conviendrait pas mieux : ce nom décrit la technique plutôt que le rôle, contrairement aux autres dossiers, et un nom qui commence par un chiffre ne peut pas servir de namespace C++.

**Proposition** : suivre la même séparation qu'en 3D, où `Renderer` sait dessiner et `World` sait quoi dessiner.

- `graphics/ui_renderer.h/.cpp` : l'actuel `HudRenderer`, directement à la racine de `graphics/`, à côté de `renderer` et `debug_line_renderer`. Un dossier pour un seul fichier n'apporterait rien ;
- `ui/` au premier niveau : `hud`, `settings_menu`.

**Fait.** La classe `HudRenderer` est devenue `UiRenderer`. Les shaders `hud_*.glsl` et l'unité de texture `HUD_ATLAS` gardent leur nom.

### 2. `util/` est devenu un fourre-tout

`util/`, avec son sous-dossier `math/`, devrait être la couche la plus basse, sans dépendance vers le reste du projet. Aujourd'hui :

| Fichier | Problème | Destination proposée |
|---|---|---|
| `frustum.h` | inclut `graphics/camera.h` et utilise `Chunk::SIZE` ; c'est du culling | `graphics/` |
| `raycaster.h` | inclut `game/world.h` ; c'est un raycast contre le monde | `game/world/` |
| `window`, `input`, `key_codes` | couche plateforme : fenêtre, contexte GL, clavier et souris | `platform/` |
| `aabb`, `directions`, `uv_rect` | maths et géométrie | `util/math/` |
| `spline`, `perlin_noise` | outils génériques, utilisés par la génération de terrain | `util/math/` |
| `ring_buffer.h`, `stb_image_write_impl.cpp` | vraiment génériques | restent dans `util/` |

**Fait.** Au passage, `raycaster.h` utilise maintenant `std::abs` : son `abs()` non qualifié ne prenait la version `float` que selon l'ordre des `#include`, et le tri des includes l'avait fait basculer sur la version entière.

### 3. `game/` va devoir se découper

Il mélange déjà :

- les données du monde : `chunk`, `world` ;
- la génération : `terrain_generator`, `biome_registry` ;
- le contenu : `blocks`, `block_registry` ;
- les entités : `entity`, `player`.

`settings_registry` n'est pas de la logique de jeu : c'est le système de paramètres du moteur, utilisé par les menus. Il peut aller dans un dossier `settings/`.

**Fait** : `game/world/`, `game/worldgen/`, `game/blocks/`, `game/entity/` et `settings/`.

### 4. Dépendance circulaire entre `game` et `graphics` (le point le plus important)

- `game/world.h` inclut `graphics/mesh/chunk_mesh.h` et `chunk_mesher.h`, parce que `World` possède les meshes des chunks et lance le meshing.
- `Blocks::registerAll()` reçoit le `BlockTextureAtlas`, qui fait partie de `graphics/`.
- Dans l'autre sens, `Renderer` dépend de `World`.

Le jeu dépend donc du rendu, et le rendu du jeu. C'est pour ça qu'il est difficile de dire à quelle couche appartient chaque fichier.

**Correction habituelle** : un `ChunkMeshManager` côté `graphics/` possède les meshes, reçoit les modifications de chunks et lance le meshing. `World` ne connaît que les blocs. C'est un vrai refactor, à faire après la réorganisation des dossiers.

**Reste à faire.**

### 5. La gestion d'ImGui est éclatée

- `Window` crée les contextes ImGui et ImPlot ;
- `Renderer` démarre et termine chaque frame ImGui (`beginUI` et `endUI`) ;
- `DebugUI` et `SettingsMenu` dessinent.

Une petite classe `ImGuiLayer`, dans `platform/` ou `debug/`, qui s'occupe de l'initialisation, du début de frame et du rendu retirerait l'UI de `Renderer`. Son `@file` le disait lui-même : « 3D world renderer and ImGui debug overlay ».

**Fait** : `platform/imgui_layer`, déclaré dans `Game` juste après la `Window`. `Window` expose son `GLFWwindow` via `getHandle()`, et `Renderer` ne dessine plus que le monde 3D.

## Arborescence

```
src/
  main.cpp, game.h/.cpp      point d'assemblage
  platform/                  window, input, key_codes, imgui_layer
  util/                      ring_buffer, stb_image_write_impl
    math/                    aabb, directions, uv_rect, spline, perlin_noise
  game/
    world/                   chunk, world, raycaster
    worldgen/                terrain_generator, biome_registry
    blocks/                  blocks, block_registry
    entity/                  entity, player
  graphics/
    gl/                      handles, objets, debug, gpu_timer, texture_units
    mesh/                    mesh, chunk_mesh, chunk_mesher, block_outline
    renderer, debug_line_renderer, ui_renderer (ex-HudRenderer), camera, frustum,
    cascaded_shadow_map, shader, texture, frame_buffer, frame_data, block_texture_atlas
  ui/                        hud, settings_menu
  settings/                  settings_registry
  debug/                     debug_settings, debug_ui, debug_draw, debug_shapes
```

## Suivi

La réorganisation a été faite sur la branche `architecture_refactor` :

1. **Aucun changement CMake n'a été nécessaire.** `CMakeLists.txt` récupère les sources avec `GLOB_RECURSE` et `CONFIGURE_DEPENDS` : seuls les `#include` ont changé.
2. **Les fichiers ont été déplacés avec `git mv`**, dans des commits qui ne contiennent que les déplacements et les `#include` qu'ils cassent : `git log --follow` retrouve l'historique de chaque fichier.
3. **Les changements de code sont dans des commits séparés** : renommage de `HudRenderer`, `ImGuiLayer`, correctif de `raycaster.h`, puis un commit de `clang-format` qui trie les `#include` modifiés.

**Reste à faire** : casser la dépendance `game` ↔ `graphics` (point 4), un refactor à part entière.
