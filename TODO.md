**NEXT UP**
- architectural refactoring 
  - seperate engine / game --> better engine api
  - abstract shader code 
    - includes, quality of life features (cfr. acreola video)

**POST-SPLIT DECOUPLING** (discovered while separating engine/game, deferred to keep that split scoped)
- Chunk : remplacer temperature/humidity hardcodés par un systeme d'attributs generique
  (chunk.addAttrib(type = perCol/perBlock, dataSize, ...)) que le game peut etendre
- World : reste cote game pour l'instant (depend directement de TerrainGenerator) --
  IVoxelWorld couvre deja la lecture (getBlock/isBlockSolid/getChunks/getChunkMeshes) pour
  qu'Entity/RayCaster/Renderer restent engine ; a revoir seulement si World lui-meme doit
  devenir reutilisable un jour
- Renderer::renderDebug() : appelle TerrainGenerator::instance() en dur pour les lignes
  PV/Erosion/Continentalness -- a resoudre en meme temps que le DebugPanel registry
  generique deja note plus bas (struct Entry = {getValue() + label})

**ENGINE/GAME SHADERS + ASSETS SPLIT** (deplacement de shaders/ et assets/, avec override)
- deplacer shaders/ -> game/shaders/ et assets/ -> game/assets/ tel quel, mettre a jour
  copy_runtime_files dans CMakeLists.txt en consequence (etape purement mecanique, a faire
  en premier pour ne rien casser)
- auditer les shaders actuels : lesquels sont assez generiques pour devenir des defauts
  engine (block, depth/shadow, outline, hud de base) vs ceux qui restent MinecraftPP
  (water/sky ont un tuning artistique specifique -- bon candidats pour tester l'override
  plutot que rester purement engine)
- creer engine/shaders/ avec les copies genericisees identifiees ci-dessus
- concevoir un resolveur de chemin : nom logique ("block") -> cherche d'abord dans
  game/shaders/, sinon retombe sur engine/shaders/ -- meme convention de nommage des deux
  cotes pour que l'override fonctionne (cfr. resource packs Minecraft, meme principe)
- brancher Shader/Renderer sur ce resolveur au lieu des chemins en dur actuels
  ("shaders/water_vert.glsl")
- texture de fallback ("missing texture" damier magenta/noir) dans engine/assets/, a
  utiliser si un asset attendu par l'engine n'est pas fourni par le game

**GENERAL IDEAS**
- gameplay
  - player -> item system

- hud / settings (voir docs/hud_and_settings_architecture.md)
  - Hud : chat (historique des messages + timers d'affichage, purge des expires)
  - ItemRenderer : rendu live d'un item-bloc en 3D (hotbar, inventaire, item lache au sol)
  - Hud : hotbar (une fois ItemRenderer + inventaire dispo)
  - DebugPanel -> make a registry of stats: struct Entry = {getValue() + label}, update the values of the registry and keep a circular buffer

- shadows
  - blue noise + accumulation temporelle sur les ombres -- need TAA d'abord (voir post processing)

- post processing : (dificulty order)
  - gamma correction / tone mapping simple -- corrige l'image lineaire vers sRGB (+ tonemap basique)
  - vignette -- assombrit les coins de l'ecran selon la distance au centre
  - chromatic aberration -- decale les UV par canal (R/G/B) en s'eloignant du centre
  - FXAA -- anti-aliasing par detection de contraste sur l'image deja rendue
  - bloom -- extrait les pixels tres lumineux, les floute, et les recompose en additif
  - depth of field -- floute selon la distance au plan de nettete (lecture du depth buffer)
  - SSAO (screen space ambient occlusion) -- assombrit les pixels selon la geometrie environnante (depth + normales), en plus de l'AO par vertex deja en place
  - god rays / light shafts (screen-space) -- blur radial depuis la position ecran de la lumiere, occlus par le depth buffer
  - SSR (screen-space reflections) -- utile pour l'eau, ray-marching dans le depth buffer pour trouver ce qui se reflete a l'ecran
  - motion blur -- flou base sur un buffer de velocite (position projetee frame courante vs precedente)
  - TAA (temporal anti-aliasing) -- combine plusieurs frames avec jitter de projection + historique + buffer de velocite