# Plan : outils de debug au runtime

Ce document détaille la mise en place des outils de debug de MinecraftPP : visualisations
(wireframe, bordures de chunks, AABB), DebugDraw, profiler CPU/GPU et graphiques de
performance dans le panneau F3. Il est pensé pour être suivi dans l'ordre, chaque étape
laissant le projet dans un état fonctionnel.

Objectif de version : **v0.2.0**.

---

## Sommaire

- [Principes](#principes)
- [Organisation du travail](#organisation-du-travail)
- [Phase 0 : fondations](#phase-0--fondations)
- [Phase 1 : le module de debug](#phase-1--le-module-de-debug)
- [Phase 2 : DebugDraw](#phase-2--debugdraw)
- [Phase 3 : les visualisations](#phase-3--les-visualisations)
- [Phase 4 : le profiler](#phase-4--le-profiler)
- [Phase 5 : les extras](#phase-5--les-extras)
- [Phase 6 : finalisation](#phase-6--finalisation)
- [Checklist récapitulative](#checklist-récapitulative)

---

## Principes

### Les outils font partie du jeu

Les outils de debug sont disponibles dans la version joueur, comme le menu F3 de Minecraft.
Il n'y a donc **pas d'option CMake** pour les retirer : tout est compilé dans tous les builds,
et activé ou désactivé au runtime.

La conséquence directe : **un outil désactivé ne doit rien coûter**. Concrètement :

- Chaque fonction de DebugDraw et du profiler commence par vérifier si l'outil est actif,
  et retourne immédiatement sinon.
- Aucune allocation mémoire par frame : les tableaux sont réservés une fois, puis vidés
  avec `clear()` (qui garde la capacité) à chaque frame.
- Les requêtes GPU (timer queries) ne sont émises que lorsque les graphiques sont affichés.
- Aucun appel OpenGL tant que rien n'est à dessiner.

Seule exception : la sortie de debug d'OpenGL (voir [0.2](#02-la-sortie-de-debug-dopengl)),
qui a un coût réel et n'est activée que dans les builds Debug, via la macro standard `NDEBUG`.

### Une seule responsabilité par brique

| Brique | Rôle | Connaît ImGui ? |
|---|---|---|
| `DebugSettings` | Les flags d'activation des outils | Non |
| `DebugDraw` | Dessiner des lignes, boîtes, frustums dans le monde 3D | Non |
| `Profiler` | Mesurer les temps CPU et GPU, garder l'historique | Non |
| `DebugUI` | Afficher le panneau F3 et modifier les flags | **Oui, et c'est le seul** |

Le reste du moteur se contente de **lire** les flags et d'**appeler** DebugDraw ou le
profiler. Il ne dépend jamais de `DebugUI`.

### Séparé des réglages du joueur

`DebugSettings` est volontairement séparé du `SettingsRegistry` :

- les flags de debug s'adressent au développeur, pas au joueur, et vivent dans le panneau
  F3 (ImGui), pas dans le menu des paramètres ;
- ils sont temporaires : on les active pour enquêter, puis on les désactive. Ils ne seront
  jamais sauvegardés avec les réglages du joueur.

Les réglages de génération (catégorie `WorldGen`) restent dans le registre pour l'instant.
Ils migreront plus tard vers un fichier de configuration par monde et le launcher.

---

## Organisation du travail

### Branche et commits

Tout se fait sur une branche dédiée :

```bash
git checkout -b feature/debug-tools
```

Un commit par étape (au minimum), avec des messages clairs, par exemple :

```
refactor(gl): add RAII wrappers for OpenGL objects
refactor(graphics): migrate Mesh to GL wrappers and DSA
refactor(graphics): per-frame uniform buffer and layout bindings in shaders
refactor(shadows): instanced geometry shader for cascades
feat(graphics): render the scene into an HDR framebuffer
feat(graphics): reverse-Z depth buffer
feat(debug): add DebugSettings and DebugUI, move debug panel out of Renderer
feat(debug): add DebugDraw immediate-mode line renderer
feat(debug): add wireframe toggle and chunk borders
feat(debug): add CPU profiler with frame time graphs
feat(debug): add GPU timer queries per render pass
```

Les étapes de pur refactor (0.1, 0.3, 1.2) ne doivent **rien changer** au comportement visible :
c'est ce qui les rend faciles à vérifier.

### Arborescence cible

```
src/
├── debug/
│   ├── debug_settings.h        Flags des outils
│   ├── debug_draw.h/.cpp       Rendu immédiat de lignes
│   ├── profiler.h/.cpp         Timers CPU/GPU et historique
│   ├── ring_buffer.h           Buffer circulaire générique
│   └── debug_ui.h/.cpp         Panneau F3 (ImGui + ImPlot)
├── graphics/
│   ├── gl/
│   │   ├── gl_handle.h         Template RAII
│   │   ├── gl_objects.h/.cpp   Types GlBuffer, GlTexture... et fonctions de création
│   │   └── texture_units.h     Numéros d'unités de texture partagés
│   ├── frame_data.h            Structure C++ du uniform buffer par frame
│   └── ...
shaders/
├── common/
│   └── frame_data.glsl         Bloc FrameData inclus par les shaders
├── debug_line_vert.glsl
└── debug_line_frag.glsl
```

---

## Phase 0 : fondations

### 0.1 Les wrappers RAII pour les objets OpenGL

#### Le problème actuel

Sept fichiers appellent directement `glGen*` et `glDelete*` :

- `src/graphics/mesh/mesh.cpp`
- `src/graphics/frame_buffer.cpp`
- `src/graphics/texture.h`
- `src/graphics/hud/hud_renderer.cpp`
- `src/graphics/mesh/block_outline.cpp`
- `src/graphics/cascaded_shadow_map.cpp`
- `src/graphics/block_texture_atlas.cpp`

Chaque classe gère ses ressources à sa manière : `Texture` est déplaçable, `Mesh` ne l'est
pas, `FrameBuffer` libère dans son destructeur. C'est du code dupliqué et une source de
fuites ou de doubles libérations.

#### La solution

Un template unique qui possède un identifiant OpenGL, le libère à sa destruction, et ne peut
qu'être déplacé :

```cpp
// src/graphics/gl/gl_handle.h
#pragma once

#include <glad/glad.h>
#include <utility>

/**
 * Owns an OpenGL object name and deletes it on destruction.
 * Move-only: an OpenGL object has exactly one owner.
 */
template <void (*DeleteFn)(GLuint)>
class GlHandle
{
public:
    GlHandle() = default;
    explicit GlHandle(GLuint id) : _id(id) {}
    ~GlHandle() { reset(); }

    GlHandle(const GlHandle &) = delete;
    GlHandle &operator=(const GlHandle &) = delete;

    GlHandle(GlHandle &&other) noexcept : _id(std::exchange(other._id, 0)) {}
    GlHandle &operator=(GlHandle &&other) noexcept
    {
        if (this != &other)
        {
            reset();
            _id = std::exchange(other._id, 0);
        }
        return *this;
    }

    GLuint id() const { return _id; }
    explicit operator bool() const { return _id != 0; }

    void reset()
    {
        if (_id != 0)
            DeleteFn(_id);
        _id = 0;
    }

private:
    GLuint _id = 0;
};
```

Les fonctions `glDelete*` prennent un tableau, alors que le template attend une fonction
qui prend un seul identifiant. Il faut donc de petites fonctions d'adaptation.

Tout ce qui concerne les types d'objets OpenGL vit dans `gl_objects.h/.cpp` : les fonctions
de suppression, les alias de types et les fonctions de création. Le header ne contient que
des déclarations, et le `.cpp` les implémentations, ce qui évite d'inclure les détails
d'implémentation partout.

```cpp
// src/graphics/gl/gl_objects.h
#pragma once

#include "gl_handle.h"

namespace gl
{
    // Suppression : utilisées comme paramètre du template, définies dans le .cpp
    void deleteBuffer(GLuint id);
    void deleteVertexArray(GLuint id);
    void deleteTexture(GLuint id);
    void deleteFramebuffer(GLuint id);
    void deleteQuery(GLuint id);
}

using GlBuffer      = GlHandle<gl::deleteBuffer>;
using GlVertexArray = GlHandle<gl::deleteVertexArray>;
using GlTexture     = GlHandle<gl::deleteTexture>;
using GlFramebuffer = GlHandle<gl::deleteFramebuffer>;
using GlQuery       = GlHandle<gl::deleteQuery>;

namespace gl
{
    /// Records the calling thread as the one owning the OpenGL context. Called once by
    /// Window, right after the context is created.
    void setContextThread();

    // Création, avec le DSA d'OpenGL 4.5+ : l'objet existe immédiatement, sans bind
    GlBuffer      createBuffer();
    GlVertexArray createVertexArray();
    GlTexture     createTexture(GLenum target); // GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY...
    GlFramebuffer createFramebuffer();
    GlQuery       createQuery(GLenum target);   // GL_TIMESTAMP pour le profiler GPU
}
```

```cpp
// src/graphics/gl/gl_objects.cpp
#include "gl_objects.h"

#include <cassert>
#include <thread>

namespace
{
    std::thread::id contextThread;

    void assertContextThread()
    {
        assert(std::this_thread::get_id() == contextThread
               && "OpenGL objects must be created and destroyed on the context thread");
    }
}

void gl::setContextThread() { contextThread = std::this_thread::get_id(); }

void gl::deleteBuffer(GLuint id)      { assertContextThread(); glDeleteBuffers(1, &id); }
void gl::deleteVertexArray(GLuint id) { assertContextThread(); glDeleteVertexArrays(1, &id); }
void gl::deleteTexture(GLuint id)     { assertContextThread(); glDeleteTextures(1, &id); }
void gl::deleteFramebuffer(GLuint id) { assertContextThread(); glDeleteFramebuffers(1, &id); }
void gl::deleteQuery(GLuint id)       { assertContextThread(); glDeleteQueries(1, &id); }

GlBuffer gl::createBuffer()
{
    assertContextThread();
    GLuint id = 0;
    glCreateBuffers(1, &id);
    return GlBuffer(id);
}

GlTexture gl::createTexture(GLenum target)
{
    assertContextThread();
    GLuint id = 0;
    glCreateTextures(target, 1, &id);
    return GlTexture(id);
}

// createVertexArray, createFramebuffer et createQuery sur le même modèle
```

Le thread du contexte est stocké dans un namespace anonyme : il est invisible en dehors de
ce fichier, comme un membre privé. Et les assertions disparaissent automatiquement en
Release.

#### La migration

Migrer une classe à la fois, en vérifiant à chaque fois que le rendu est identique :

1. **`Mesh`** : remplacer `_vao`, `_vbo`, `_ebo` par un `GlVertexArray` et deux `GlBuffer`.
   Supprimer le destructeur et les `= delete` : la classe suit maintenant la *rule of zero*
   et devient déplaçable automatiquement.
2. **`FrameBuffer`** : un `GlFramebuffer` et deux `GlTexture`.
3. **`Texture`** : un `GlTexture`, et supprimer le move constructor écrit à la main.
4. **`CascadedShadowMap`**, **`BlockTextureAtlas`**, **`HudRenderer`**, **`BlockOutline`** :
   même principe.

Critère de fin : `grep -rn "glGen\|glDelete" src` ne renvoie plus rien en dehors de
`src/graphics/gl/`.

Chaque classe migrée est aussi l'occasion de la passer aux fonctionnalités de 4.6
(DSA, stockage immuable...) : voir [0.3](#03-la-modernisation-vers-opengl-46).

#### Deux règles à respecter

**Le thread principal uniquement.** Les objets OpenGL ne peuvent être créés et détruits que
sur le thread qui possède le contexte. Aujourd'hui, les threads du thread pool ne produisent
que des données CPU, et `World::update()` envoie les meshes au GPU sur le thread principal :
c'est correct. Il faut juste que ça reste vrai. Si un jour un objet contenant un wrapper
doit être détruit depuis un worker (par exemple via un `shared_ptr` dont la dernière
référence disparaît dans un job), il faudra une file de suppressions différées, vidée au
début de chaque frame sur le thread principal.

Les assertions de `gl_objects.cpp` détectent le problème immédiatement en Debug, à
condition que `Window` appelle `gl::setContextThread()` juste après avoir créé le contexte.

**Détruire avant le contexte.** Tous les wrappers doivent être détruits avant la fenêtre.
Dans `Game`, les membres sont détruits dans l'ordre inverse de leur déclaration, et
`_window` est déclaré en premier : l'ordre actuel est correct. Il faudra le garder ainsi
en ajoutant de nouveaux membres.

### 0.2 La sortie de debug d'OpenGL

Avec OpenGL 4.6, le driver peut appeler une fonction dès qu'une erreur, un usage déprécié ou
un problème de performance survient. C'est bien plus pratique que `glGetError`.

Cet outil a un coût réel, donc il n'est activé que dans les builds Debug :

```cpp
// Avant la création de la fenêtre, dans Window
#ifndef NDEBUG
glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

// Après le chargement de glad
#ifndef NDEBUG
glEnable(GL_DEBUG_OUTPUT);
glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // le callback est appelé dans l'appel fautif
glDebugMessageCallback(onGlDebugMessage, nullptr);
// ignorer les notifications, trop bavardes
glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
                      0, nullptr, GL_FALSE);
#endif
```

Grâce au mode synchrone, un point d'arrêt dans `onGlDebugMessage` pour les messages de
sévérité `GL_DEBUG_SEVERITY_HIGH` donne directement la pile d'appels de l'appel fautif.

Au passage : mettre à jour la documentation de `Window` dans `window.h`, qui mentionne
encore un contexte OpenGL 3.3.

### 0.3 La modernisation vers OpenGL 4.6

Le contexte et les shaders sont déjà en 4.6, et le filtrage anisotrope (devenu standard en
4.6) est déjà utilisé dans `BlockTextureAtlas`. Mais le code C++ est encore écrit comme en
3.3 : bind puis modification, `glGetUniformLocation` à chaque appel, textures mutables,
`glActiveTexture` + `glBindTexture` + `setInt` pour chaque sampler. Cette section liste tout
ce qui peut être simplifié, avec les endroits concernés dans le code.

**Conseil d'organisation** : fais cette modernisation classe par classe, **en même temps**
que la migration vers les wrappers RAII (0.1). Quand tu reprends `Mesh` pour y mettre des
`GlBuffer`, passe-la aussi au DSA dans le même commit : tu ne touches chaque fichier qu'une
fois.

#### 0.3.1 Vue d'ensemble

| Aujourd'hui (style 3.3) | En 4.6 | Version | Gain |
|---|---|---|---|
| `glGen*` + `glBind*` avant chaque modification | `glCreate*` + fonctions `glNamed*` / `glTexture*` (DSA) | 4.5 | Plus d'état global caché, moins de bugs de binding |
| `glBufferData` (buffer redimensionnable) | `glNamedBufferStorage` (buffer immuable) | 4.4 / 4.5 | Le driver connaît l'usage exact, erreurs détectées tôt |
| `glVertexAttribPointer` après `glBindBuffer` | Format de sommet séparé du buffer (`glVertexArrayAttribFormat`) | 4.3 / 4.5 | Un seul VAO par format, partagé par tous les meshes |
| `glTexImage2D` (texture mutable) | `glTextureStorage2D` + `glTextureSubImage2D` | 4.2 / 4.5 | Texture complète et valide dès sa création |
| `glActiveTexture` + `glBindTexture` | `glBindTextureUnit(unit, tex)` | 4.5 | Une ligne au lieu de deux |
| `setInt("shadowMap", 2)` pour chaque sampler | `layout(binding = 2) uniform sampler2DArray shadowMap;` | 4.2 | Plus aucun appel C++ pour les samplers |
| `glGetUniformLocation` à chaque `set*()` | `layout(location = N) uniform ...` ou un cache | 4.3 | Plus de recherche par nom dans la boucle de rendu |
| Mêmes uniforms envoyés à chaque shader | Un uniform buffer par frame avec `layout(binding = 0)` | 3.1 / 4.2 | Envoyés une seule fois par frame, partagés |
| `glBindFramebuffer` + `glBlitFramebuffer` | `glBlitNamedFramebuffer` | 4.5 | Pas de bind/rebind autour du blit |
| `glDrawBuffer` / `glReadBuffer` sur le FBO lié | `glNamedFramebufferDrawBuffer` / `ReadBuffer` | 4.5 | Idem |
| Geometry shader qui boucle sur les 5 cascades | Geometry shader instancié (`invocations = 5`) | 4.0 | Plus simple et exécuté en parallèle |
| Boucle `glGetError()` | Sortie de debug (0.2), labels et groupes | 4.3 | Messages clairs, visibles dans RenderDoc |

#### 0.3.2 Les buffers : DSA et stockage immuable

Exemple avec `Mesh::upload` :

```cpp
// Avant
glBindVertexArray(_vao);
glBindBuffer(GL_ARRAY_BUFFER, _vbo);
glBufferData(GL_ARRAY_BUFFER, _nVert * sizeof(GLuint), data.vertices.data(), GL_STATIC_DRAW);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...);
glVertexAttribIPointer(0, 2, GL_UNSIGNED_INT, 0, 0);
glEnableVertexAttribArray(0);

// Après
_vbo = gl::createBuffer();
glNamedBufferStorage(_vbo.id(), vertexBytes, data.vertices.data(), 0);
_ebo = gl::createBuffer();
glNamedBufferStorage(_ebo.id(), indexBytes, data.indices.data(), 0);
```

Le dernier paramètre de `glNamedBufferStorage` indique ce qu'on compte faire du buffer
ensuite. `0` signifie "jamais modifié" : c'est le cas des meshes de chunks, qui sont
reconstruits entièrement quand un bloc change. Pour les buffers modifiés à chaque frame
(HUD, DebugDraw), utiliser `GL_DYNAMIC_STORAGE_BIT` puis `glNamedBufferSubData`.

Un buffer immuable ne peut pas changer de taille : quand un chunk est remeshé, on crée un
nouveau buffer et l'ancien est libéré automatiquement par son wrapper. C'est exactement ce
que fait déjà ton code aujourd'hui, en plus explicite.

Fichiers concernés : `mesh.cpp`, `hud_renderer.cpp`, `block_outline.cpp`.

#### 0.3.3 Les VAO : un format de sommet, pas un VAO par mesh

En 3.3, un VAO mélange deux choses : le **format** des sommets (quels attributs, quel type)
et les **buffers** d'où ils viennent. En 4.3+, ces deux notions sont séparées : le format
est décrit une fois, et on change seulement le buffer attaché.

```cpp
// Une seule fois, pour tous les meshes de chunks
GlVertexArray chunkVao = gl::createVertexArray();
glEnableVertexArrayAttrib(chunkVao.id(), 0);
glVertexArrayAttribIFormat(chunkVao.id(), 0, 2, GL_UNSIGNED_INT, 0); // uvec2 packedData
glVertexArrayAttribBinding(chunkVao.id(), 0, 0);                     // attribut 0 -> binding 0

// Pour dessiner un chunk
glVertexArrayVertexBuffer(chunkVao.id(), 0, mesh.vbo(), 0, sizeof(GLuint) * 2);
glVertexArrayElementBuffer(chunkVao.id(), mesh.ebo());
glDrawElements(GL_TRIANGLES, mesh.indexCount(), GL_UNSIGNED_INT, nullptr);
```

Chaque `Mesh` n'a alors plus de VAO, seulement ses deux buffers. Le VAO appartient au
renderer, un par format de sommet : chunks, HUD, lignes de debug, outline.

C'est aussi la première marche vers le rendu GPU-driven : le jour où tous les chunks seront
dans un seul gros buffer, ce VAO unique n'aura même plus besoin de changer de buffer.

Fichiers concernés : `mesh.h/.cpp`, `renderer.cpp`, `hud_renderer.cpp`, `block_outline.cpp`.

#### 0.3.4 Les textures : stockage immuable et unités de texture

**Création.** `glTexImage2D` crée une texture dont chaque niveau de mipmap peut avoir une
taille ou un format différent, ce qui la rend potentiellement incomplète (et donc noire
à l'écran, sans erreur). `glTextureStorage2D` alloue d'un coup tous les niveaux, avec un
format fixe :

```cpp
// BlockTextureAtlas : tous les niveaux de mipmap alloués en une fois
_atlas = gl::createTexture(GL_TEXTURE_2D);
glTextureStorage2D(_atlas.id(), MIPMAP_LEVELS, GL_RGBA8, width, height);
for (int level = 0; level < MIPMAP_LEVELS; level++)
    glTextureSubImage2D(_atlas.id(), level, 0, 0, w >> level, h >> level,
                        GL_RGBA, GL_UNSIGNED_BYTE, mipData[level].data());

glTextureParameteri(_atlas.id(), GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
glTextureParameteri(_atlas.id(), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTextureParameterf(_atlas.id(), GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
```

`GL_TEXTURE_MAX_LEVEL` devient inutile : le nombre de niveaux est donné à la création. Tes
mipmaps générés à la main restent compatibles, seule la façon de les envoyer change.

Pour la shadow map : `glTextureStorage3D(id, 1, GL_DEPTH_COMPONENT32F, size, size, 5)`.

**Utilisation.** Déclarer l'unité directement dans le shader :

```glsl
// block_frag.glsl
layout(binding = 0) uniform sampler2D atlas;
layout(binding = 1) uniform sampler2D colormap;
layout(binding = 2) uniform sampler2DArray shadowMap;

// water_frag.glsl
layout(binding = 3) uniform sampler2D solidColor;
layout(binding = 4) uniform sampler2D solidDepth;
```

Et côté C++, une seule ligne par texture, sans `setInt` :

```cpp
// Avant
glActiveTexture(GL_TEXTURE3);
glBindTexture(GL_TEXTURE_2D, _frameBuffer->getColorTextureID());
_waterShader->setInt("solidColor", 3);

// Après
glBindTextureUnit(3, _frameBuffer->colorTexture());
```

Conseil : regrouper les numéros d'unités dans un seul header partagé (par exemple un
`enum TextureUnit { Atlas = 0, Colormap = 1, ShadowMap = 2, ... }`) pour éviter que deux
shaders utilisent la même unité pour deux textures différentes par erreur.

Fichiers concernés : `texture.h`, `block_texture_atlas.cpp`, `cascaded_shadow_map.cpp`,
`frame_buffer.cpp`, `renderer.cpp`, `hud_renderer.cpp`, et les shaders.

#### 0.3.5 Les uniforms : un buffer par frame et des locations fixes

Aujourd'hui, chaque `setMat4`, `setFloat`, `setVec3` appelle `glGetUniformLocation`, qui
recherche le nom dans le programme. Dans la boucle des chunks, `setMat4("model", ...)` est
appelé pour **chaque chunk rendu**, donc des centaines de recherches par nom à chaque frame.

**Les données communes dans un uniform buffer.** Plusieurs shaders reçoivent les mêmes
valeurs : `view`, `projection`, `lightDir`, `camPos`, `time`, `zNear`, `zFar`, et
`lightSpaceMatrices` (envoyé à la fois à `block_frag` et `depth_geom`). Regroupe-les dans
un seul bloc, déclaré dans un fichier commun :

```glsl
layout(std140, binding = 0) uniform FrameData
{
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrices[5];
    vec4 cutoffDist[2];  // 5 valeurs utiles, voir l'encadré std140 ci-dessous
    vec4 lightDir;       // xyz utilisé
    vec4 camPos;         // xyz utilisé
    float time;
    float zNear;
    float zFar;
};
```

Côté C++, une structure identique, un seul buffer créé au démarrage avec
`GL_DYNAMIC_STORAGE_BIT`, rempli **une fois par frame** avec `glNamedBufferSubData`, et
attaché une fois pour toutes avec `glBindBufferBase(GL_UNIFORM_BUFFER, 0, buffer)`. Tous les
shaders qui déclarent `FrameData` y ont accès, sans aucun appel supplémentaire.

> **Attention à l'alignement std140.** C'est le piège classique des uniform buffers :
> un `vec3` occupe la place d'un `vec4` (16 octets), et **chaque élément d'un tableau de
> `float` occupe aussi 16 octets**. Ton `float cutoffDist[5]` ferait donc 80 octets côté
> GPU, alors que le tableau C++ équivalent en fait 20 : les valeurs seraient décalées.
> Solutions : utiliser des `vec4` (comme ci-dessus, avec `cutoffDist[i / 4][i % 4]` dans le
> shader), et côté C++ utiliser `glm::vec4` et `alignas(16)`. Ajoute un
> `static_assert(sizeof(FrameData) == ...)` pour détecter tout décalage à la compilation.

Comme GLSL n'a pas de `#include`, le bloc doit être recopié dans chaque shader, ou inséré
automatiquement par ta classe `Shader` au chargement (une simple recherche d'une ligne
`#include "frame_data.glsl"` remplacée par le contenu du fichier). C'est justement dans ton
TODO ("shader includes").

**La position du chunk à la place de la matrice `model`.** La matrice `model` d'un chunk
n'est qu'une translation. Un uniform `vec3 chunkOffset` suffit, avec une location fixe :

```glsl
// block_vert.glsl
layout(location = 0) uniform vec3 chunkOffset;
```

```cpp
// Dans la boucle des chunks : aucune recherche par nom
glProgramUniform3f(_blockShader->id(), 0, offset.x, offset.y, offset.z);
```

`glProgramUniform*` (4.1) modifie l'uniform d'un programme sans qu'il soit actif, ce qui
évite aussi un `use()` dans certains cas.

**Pour les uniforms restants**, deux options : leur donner une `layout(location = N)`
comme ci-dessus, ou garder l'interface par nom mais ajouter un cache dans `Shader`
(une `std::unordered_map<std::string, GLint>` remplie au premier appel). Le cache est plus
simple à mettre en place et suffisant pour les uniforms envoyés une fois par frame.

Fichiers concernés : `shader.h/.cpp`, `renderer.cpp`, `cascaded_shadow_map.cpp`, tous les
shaders.

#### 0.3.6 Les framebuffers

Remplacer les appels qui dépendent du framebuffer lié par leurs équivalents DSA :

| Avant | Après |
|---|---|
| `glGenFramebuffers` + `glBindFramebuffer` | `glCreateFramebuffers` (via le wrapper) |
| `glFramebufferTexture` | `glNamedFramebufferTexture` |
| `glDrawBuffer(GL_NONE)` / `glReadBuffer(GL_NONE)` | `glNamedFramebufferDrawBuffer` / `glNamedFramebufferReadBuffer` |
| `glCheckFramebufferStatus` | `glCheckNamedFramebufferStatus` |
| `glBindFramebuffer` ×2 + `glBlitFramebuffer` + rebind | `glBlitNamedFramebuffer(src, dst, ...)` |
| `glBindFramebuffer` + `glClear` | `glClearNamedFramebufferfv` (sans bind) |

Le blit de `renderWorld()` (copie de la scène opaque avant l'eau) devient une seule ligne,
avec `0` pour désigner le framebuffer par défaut :

```cpp
glBlitNamedFramebuffer(0, _frameBuffer->id(),
                       0, 0, w, h, 0, 0, w, h,
                       GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
```

**Recommandé : rendre la scène dans ton propre framebuffer.** Aujourd'hui, le monde est
dessiné directement dans le framebuffer par défaut, multisamplé par GLFW (`GLFW_SAMPLES 4`).
Tu n'as donc aucun contrôle sur son format : profondeur 24 bits entiers, couleur 8 bits.
Rendre la scène dans un FBO à toi (couleur `GL_RGBA16F`, profondeur `GL_DEPTH_COMPONENT32F`,
multisamplé avec `glTextureStorage2DMultisample`), puis le copier à l'écran à la fin, te
donne :

- la possibilité d'utiliser le reverse-Z (voir 0.3.8), qui exige une profondeur flottante ;
- une base prête pour le post-processing de ton TODO (tone mapping, bloom, FXAA...), qui a
  besoin de lire l'image de la scène dans une texture, en HDR ;
- le choix de l'antialiasing, que tu pourras un jour remplacer par du TAA.

Tu retires alors `GLFW_SAMPLES` : le framebuffer par défaut n'a plus besoin d'être
multisamplé.

Fichiers concernés : `frame_buffer.cpp`, `cascaded_shadow_map.cpp`, `renderer.cpp`,
`window.cpp`.

#### 0.3.7 Les ombres : le geometry shader instancié

Le commentaire en tête de `depth_geom.glsl` l'explique : le qualificateur `invocations`
n'était pas disponible en 3.3, donc une seule invocation boucle sur les 5 cascades. En 4.0+,
le GPU peut lancer une invocation par cascade, en parallèle :

```glsl
#version 460 core

layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;

// lightSpaceMatrices vient maintenant du bloc FrameData (0.3.5)

void main()
{
    for (int i = 0; i < 3; i++)
    {
        gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}
```

Le shader est plus court, `max_vertices` passe de 15 à 3, et les drivers exécutent
généralement mieux cette version.

Optimisation possible ensuite : ne pas émettre le triangle dans une cascade dont il est
entièrement hors du frustum de lumière (un test sur les trois sommets après transformation).

#### 0.3.8 Le reverse-Z (après 0.3.6)

Avec une profondeur classique, la précision est concentrée près de la caméra, et les
surfaces lointaines se mélangent (z-fighting). Le reverse-Z inverse la plage de profondeur
(1 près de la caméra, 0 au loin) et, combiné à une profondeur flottante, répartit la
précision presque uniformément. Pour un monde voxel avec une grande distance de vue, c'est
un gain net.

Mise en place :

1. Profondeur en `GL_DEPTH_COMPONENT32F` dans ton FBO de scène (d'où la dépendance à 0.3.6).
2. `glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)` au démarrage (4.5) : la profondeur en NDC
   va de 0 à 1 au lieu de -1 à 1, ce qui évite de perdre la moitié de la précision.
3. Une matrice de projection inversée (glm n'en fournit pas directement : il faut la
   construire à la main, ou inverser la plage avec une matrice de correction).
4. Effacer la profondeur à `0.0` au lieu de `1.0`, et `glDepthFunc(GL_GREATER)` au lieu de
   `GL_LESS`.

**Attention** : tout code qui interprète la profondeur doit être adapté. Chez toi, c'est le
cas de `water_frag.glsl`, qui utilise `zNear` et `zFar` pour linéariser `solidDepth`
(réfraction et SSR), et de la reconstruction de position du SSR. Les shadow maps peuvent
rester en profondeur classique.

C'est l'étape la plus délicate de cette section : fais-la à part, dans son propre commit,
et vérifie soigneusement l'eau et les ombres.

#### 0.3.9 L'épaisseur des lignes

`BlockOutline` appelle `glLineWidth(3.0f)`. Or `Window` crée un contexte avec
`GLFW_OPENGL_FORWARD_COMPAT`, et dans un contexte *forward-compatible*, une épaisseur
supérieure à 1 **génère une erreur** `GL_INVALID_VALUE` : l'outline est probablement dessinée
avec une épaisseur de 1 sans que tu le saches. La sortie de debug (0.2) le signalera dès son
activation.

Ce hint n'est nécessaire que sur macOS, qui ne supporte de toute façon pas OpenGL 4.6 :
tu peux le retirer. Les lignes épaisses fonctionneront alors sur la plupart des drivers,
mais elles restent officiellement dépréciées. La solution durable est de dessiner l'outline
avec des quads fins (six faces légèrement plus grandes que le bloc, ou des quads orientés
vers la caméra le long de chaque arête).

#### 0.3.10 Les labels et groupes de debug

En complément de la sortie de debug (0.2), OpenGL 4.3 permet de nommer les objets et de
regrouper les commandes :

```cpp
glObjectLabel(GL_BUFFER, buffer.id(), -1, "Chunk (3, -2) vertices");
glObjectLabel(GL_TEXTURE, atlas.id(), -1, "Block atlas");

glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Shadow pass");
// ... rendu des cascades
glPopDebugGroup();
```

Les noms apparaissent dans les messages de la sortie de debug, et surtout dans **RenderDoc**
ou Nsight, où chaque passe devient une section repliable portant ce nom, au lieu d'une
longue liste anonyme de draw calls. Les groupes coûtent quasiment rien : ils peuvent rester
actifs dans tous les builds.

Astuce : le `GPU_PROFILE_SCOPE` de la phase 4 peut ouvrir et fermer un groupe de debug en
même temps que ses timer queries. Une seule macro par passe pour les deux outils.

#### 0.3.11 Ce qu'on garde pour plus tard

Ces fonctionnalités de 4.3 à 4.6 sont très utiles, mais demandent de repenser une partie du
moteur : elles ne font pas partie de ce refactor.

| Fonctionnalité | Version | Utilisation prévue |
|---|---|---|
| Timer queries (`glQueryCounter`) | 3.3 / 4.5 (DSA) | Profiler GPU, phase 4 de ce plan |
| Vues de texture (`glTextureView`) | 4.3 | Visualiseur des cascades, phase 5 |
| Compute shaders | 4.3 | Post-processing (bloom, SSAO, TAA), culling GPU |
| Shader storage buffers (SSBO) | 4.3 | Données des chunks lues par les shaders |
| Multi-draw indirect | 4.3 | Tous les chunks en un seul draw call |
| `gl_DrawID`, `gl_BaseInstance` | 4.6 | Retrouver les données du chunk dans le shader |
| Nombre de draws lu depuis un buffer | 4.6 | Culling sur GPU qui remplit lui-même la liste des draws |
| Buffers persistants (`GL_MAP_PERSISTENT_BIT`) | 4.4 | Upload des chunks et des lignes de debug sans copie |
| `glCopyImageSubData` | 4.3 | Copier des textures sans passer par un framebuffer |
| Shaders SPIR-V | 4.6 | Préparation d'un éventuel passage à Vulkan |

Les étapes 0.3.2 (buffers immuables) et 0.3.3 (un VAO partagé) préparent directement le
multi-draw indirect : quand tu t'y attaqueras, la moitié du travail sera faite.

#### 0.3.12 Bonus : des simplifications possibles sans rapport avec 4.x

Deux améliorations qui existaient déjà en 3.3, mais qui simplifieraient ton code :

- **L'atlas en texture array** (`GL_TEXTURE_2D_ARRAY`, une couche par texture de bloc) :
  chaque couche a ses propres mipmaps, sans débordement entre textures voisines. Tu peux
  alors générer les mipmaps avec `glGenerateTextureMipmap` et supprimer ta génération
  manuelle. C'est aussi un prérequis au greedy meshing, où les faces fusionnées doivent
  répéter leur texture.
- **La comparaison de profondeur matérielle pour les ombres** : avec
  `GL_TEXTURE_COMPARE_MODE` sur la shadow map et un `sampler2DArrayShadow` dans le shader,
  le GPU fait la comparaison et un filtrage bilinéaire gratuit à chaque échantillon. Ton
  PCF actuel devient plus doux pour le même nombre d'échantillons.

### 0.4 Les entrées clavier

L'enum `Key` de `src/util/key_codes.h` ne contient que `W`, `A`, `S`, `D`, `Space`,
`LShift`, `LCtrl`, `Esc` et `F3`. Il faut y ajouter les touches utilisées par les
raccourcis de debug (au minimum `G`, `B`, `H`, `L`, `P`, `F`), avec leur correspondance
GLFW dans `Window`.

Pour les combinaisons façon Minecraft (`F3 + G`), la logique est :

- si F3 est **maintenu** et qu'une autre touche est **pressée**, c'est un raccourci ;
- F3 seul, **relâché** sans qu'aucune combinaison n'ait été utilisée, ouvre ou ferme le
  panneau.

Ça demande de distinguer le relâchement de F3, et de retenir si une combinaison a été
utilisée pendant qu'il était maintenu. Toute cette logique doit vivre à un seul endroit,
par exemple une fonction `DebugUI::handleInput(Window &)`, et non dans `Game::processInput()`.

Raccourcis proposés :

| Raccourci | Action |
|---|---|
| `F3` | Afficher / masquer le panneau de debug |
| `F3 + G` | Bordures de chunks |
| `F3 + B` | AABB des entités |
| `F3 + L` | Wireframe |
| `F3 + P` | Graphiques de performance |
| `F3 + F` | Geler le culling |
| `F3 + H` | Aide : liste des raccourcis dans le panneau |

---

## Phase 1 : le module de debug

### 1.1 DebugSettings

Une structure simple, sans logique :

```cpp
// src/debug/debug_settings.h
#pragma once

/**
 * Runtime flags of the debug tools. Separate from the player settings: these are
 * never saved, and are only shown in the F3 panel.
 */
struct DebugSettings
{
    bool showPanel = false;       ///< F3 panel visible.
    bool showHelp = false;        ///< Shortcut list in the panel.

    // Visualisations
    bool wireframe = false;
    bool showChunkBorders = false;
    bool showEntityAABBs = false;
    bool showRaycast = false;
    bool freezeCulling = false;

    // Performance
    bool showPerformanceGraphs = false;
    bool gpuTimings = false;      ///< Emit GPU timer queries (only useful when graphs are shown).
};

/// Global access: debug tools must be reachable from anywhere in the engine.
DebugSettings &debugSettings();
```

Un accès global est acceptable ici : comme DebugDraw, ces flags doivent pouvoir être
consultés depuis n'importe quel système, et ne portent aucun état de jeu. La fonction
`debugSettings()` renvoie une instance statique définie dans un `.cpp`.

Le booléen `_showDebug` de `Game` disparaît au profit de `debugSettings().showPanel`.

### 1.2 DebugUI : sortir le panneau du Renderer

Actuellement, `Renderer::renderDebug()` affiche le FPS, la position de la caméra, le nombre
de chunks chargés et rendus, et les valeurs des noises du générateur. Le Renderer dépend
donc du générateur de terrain uniquement pour le debug.

Cette étape est **un refactor pur** : le panneau doit afficher exactement la même chose
qu'avant.

`DebugUI` ne doit pas aller chercher ses données partout dans le moteur. On lui passe une
structure qui regroupe ce qu'il affiche, remplie par `Game` à chaque frame :

```cpp
// src/debug/debug_ui.h
struct DebugFrameInfo
{
    glm::vec3 cameraPos;
    int loadedChunks;
    int renderedChunks;
    float pvNoise;
    float erosionNoise;
    float continentalnessNoise;
    // complété au fil des phases (bloc visé, compteurs...)
};

class DebugUI
{
public:
    void handleInput(Window &window);      // raccourcis F3 (voir 0.4)
    void render(const DebugFrameInfo &info);
};
```

Étapes :

1. Créer `DebugUI` et y copier le contenu de `Renderer::renderDebug()`.
2. Ajouter à `Renderer` et `World` des accesseurs pour les valeurs nécessaires (chunks rendus,
   par exemple), s'ils n'existent pas déjà.
3. Remplir `DebugFrameInfo` dans `Game::render()` et appeler `DebugUI::render()` à la place de
   `_renderer.renderDebug()`, entre `beginUI()` et `endUI()`.
4. Supprimer `renderDebug()`, `_fps`, `_msPerFrame` et les champs de debug de `Renderer`.
   Le calcul du FPS peut rester provisoirement dans `DebugUI` ; il passera dans le profiler
   à la phase 4.

### 1.3 L'organisation du panneau

Pour que le panneau reste lisible quand les outils s'accumulent, découpe-le en sections
repliables avec `ImGui::CollapsingHeader` :

- **Général** : FPS, temps de frame, position, direction, chunk courant ;
- **Monde** : chunks chargés et rendus, jobs en attente, valeurs des noises ;
- **Visualisations** : les cases à cocher de `DebugSettings` ;
- **Performance** : graphiques et compteurs (phase 4) ;
- **Rendu** : textures internes et vues de debug (phase 5).

Les cases à cocher et les raccourcis modifient les mêmes flags, ils sont donc toujours
synchronisés.

---

## Phase 2 : DebugDraw

### 2.1 L'API

Un renderer de lignes en mode immédiat, appelable depuis n'importe où, pendant toute la frame :

```cpp
// src/debug/debug_draw.h
#pragma once

#include <glm/glm.hpp>

class AABB;

namespace DebugDraw
{
    enum class Mode
    {
        DepthTested, ///< Hidden behind the terrain.
        Overlay,     ///< Always visible, drawn on top of everything.
    };

    void init();     ///< Creates the GPU resources. Called once, after the GL context exists.
    void shutdown(); ///< Releases them, before the context is destroyed.

    void beginFrame(); ///< Clears the previous frame's lines. Start of the frame.
    void endFrame(const glm::mat4 &viewProj); ///< Uploads and draws everything. In the render pass.

    void line(glm::vec3 a, glm::vec3 b, glm::vec4 color, Mode mode = Mode::DepthTested);
    void box(glm::vec3 min, glm::vec3 max, glm::vec4 color, Mode mode = Mode::DepthTested);
    void aabb(const AABB &box, glm::vec3 pos, glm::vec4 color, Mode mode = Mode::DepthTested);
    void frustum(const glm::mat4 &viewProj, glm::vec4 color, Mode mode = Mode::DepthTested);
    void cross(glm::vec3 center, float size, glm::vec4 color, Mode mode = Mode::DepthTested);
    void gridXZ(glm::vec3 origin, float size, int divisions, glm::vec4 color,
                Mode mode = Mode::DepthTested);
}
```

### 2.2 Le cycle d'une frame

La différence avec le `HudRenderer` : les appels ne se font pas seulement pendant le rendu,
mais aussi pendant la mise à jour (la physique du joueur, le raycaster...). `beginFrame()` et
`endFrame()` délimitent donc la **frame**, pas une zone de dessin.

```
Game::run()
 ├── DebugDraw::beginFrame()        vide les tableaux (clear, sans libérer la mémoire)
 ├── processInput()
 ├── update()                       appels à line(), aabb()... ajout dans les tableaux CPU
 └── render()
      ├── renderWorld()             ombres, terrain, eau, SSR
      ├── renderBlockOutline()
      ├── DebugDraw::endFrame()     upload + 2 draw calls (depth tested, puis overlay)
      ├── hud.render()
      └── UI (ImGui)
```

Placer `endFrame()` après le monde et l'eau, avant le HUD : le depth buffer du monde est
encore disponible, donc les lignes en mode `DepthTested` sont correctement masquées par le
terrain.

### 2.3 Le format des sommets

16 octets par sommet, avec la couleur compressée en un entier :

```cpp
struct DebugVertex
{
    glm::vec3 pos;  // 12 octets
    uint32_t color; // RGBA8, 4 octets
};
```

Côté VAO, l'attribut couleur est déclaré avec `glVertexArrayAttribFormat(vao, 1, 4,
GL_UNSIGNED_BYTE, GL_TRUE, offset)` : OpenGL convertit automatiquement les quatre octets en
`vec4` entre 0 et 1.

### 2.4 Le stockage

- Deux `std::vector<DebugVertex>` (un par mode), réservés à l'initialisation (par exemple
  65 536 sommets chacun) et vidés avec `clear()` à chaque frame : aucune allocation en
  régime normal.
- Un seul `GlBuffer` côté GPU. À chaque `endFrame()`, si la taille nécessaire dépasse la
  capacité, on réalloue avec une capacité doublée ; sinon on fait un `glNamedBufferSubData`.
- Les deux listes sont copiées l'une après l'autre dans le même buffer, et dessinées avec
  deux `glDrawArrays(GL_LINES, offset, count)`.

Optimisation possible plus tard : un buffer persistant (`glNamedBufferStorage` avec
`GL_MAP_PERSISTENT_BIT`) découpé en trois zones utilisées à tour de rôle, pour écrire
directement dedans sans copie. Inutile tant que le nombre de lignes reste modeste.

### 2.5 Le rendu

Un shader minimal :

```glsl
// shaders/debug_line_vert.glsl
#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 viewProj;
out vec4 vColor;
void main()
{
    vColor = aColor;
    gl_Position = viewProj * vec4(aPos, 1.0);
}
```

```glsl
// shaders/debug_line_frag.glsl
#version 460 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; }
```

Et dans `endFrame()` :

1. Si les deux listes sont vides, retourner immédiatement (aucun appel OpenGL).
2. Upload dans le buffer.
3. Liste `DepthTested` : test de profondeur activé, écriture de profondeur **désactivée**
   (`glDepthMask(GL_FALSE)`) pour que les lignes ne se masquent pas entre elles.
4. Liste `Overlay` : test de profondeur désactivé.
5. Restaurer l'état (`glDepthMask(GL_TRUE)`, `glEnable(GL_DEPTH_TEST)`).

Note sur l'épaisseur : `glLineWidth` au-delà de 1 n'est pas garanti en core profile. Des
lignes d'un pixel suffisent largement pour le debug. Si un jour tu veux des lignes épaisses,
il faudra générer des quads orientés vers la caméra dans le vertex shader.

### 2.6 Les fonctions d'aide

- **`box()` / `aabb()`** : les 8 coins, puis les 12 arêtes. Ton `AABB` expose déjà
  `getMin(pos)` et `getMax(pos)`.
- **`frustum()`** : les 8 coins du cube NDC (de -1 à 1 sur x et y, de -1 à 1 sur z en
  OpenGL), transformés par `inverse(viewProj)` puis divisés par `w`, et reliés par 12 arêtes.
  Il faut l'inverse de la matrice *view-projection* de la caméra à visualiser.
- **`cross()`** : trois segments centrés sur un point, un par axe.
- **`gridXZ()`** : des lignes parallèles sur un plan horizontal.

### 2.7 Les garde-fous

- **Retour immédiat** si `debugSettings().showPanel` est faux et qu'aucune visualisation
  n'est active. Ou plus simplement : chaque appelant vérifie son propre flag avant d'appeler
  DebugDraw (voir phase 3).
- **Thread principal uniquement** : une assertion au début de chaque fonction d'ajout.
  Si un jour tu as besoin de visualiser quelque chose pendant la génération des chunks,
  il faudra un mutex ou des tableaux par thread.
- **Limite de sommets** : au-delà d'un plafond (par exemple 1 million), ignorer les ajouts
  et afficher un avertissement dans F3, pour éviter qu'un bug dans une boucle ne fasse
  exploser la mémoire.

---

## Phase 3 : les visualisations

Principe commun : **chaque système décide lui-même de dessiner**, en vérifiant son flag.

```cpp
if (debugSettings().showEntityAABBs)
    DebugDraw::aabb(_aabb, _pos, colors::Yellow);
```

### 3.1 Le wireframe

Dans `Renderer::renderWorld()`, autour des passes du terrain et de l'eau seulement :

```cpp
const bool wireframe = debugSettings().wireframe;
if (wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

// passe du terrain
// passe de l'eau

if (wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
```

Points d'attention :

- **Pas pendant la passe d'ombres** : les shadow maps doivent rester pleines, sinon les
  ombres deviennent incohérentes.
- **Remettre `GL_FILL` avant tout le reste** : HUD, ImGui, blits de framebuffer.
- Le SSR de l'eau s'appuie sur une copie de la scène : en wireframe, ses reflets seront
  étranges. C'est normal, pas un bug.
- Option : désactiver temporairement le face culling pour voir aussi les faces arrière.

Amélioration pour plus tard : un mode "wireframe par-dessus" (le rendu normal plus les arêtes
des triangles en surimpression), avec des coordonnées barycentriques calculées dans le shader.
On voit alors la géométrie sans perdre les textures.

### 3.2 Les bordures de chunks

Avec `Chunk::SIZE = 16` et `Chunk::HEIGHT = 256` :

1. Calculer les coordonnées du chunk du joueur :
   `cx = floor(pos.x / Chunk::SIZE)`, `cz = floor(pos.z / Chunk::SIZE)`.
2. **Chunks voisins** : pour chaque chunk dans un rayon de 2 autour du joueur, une ligne
   verticale de `y = 0` à `y = Chunk::HEIGHT` à chacun des 4 coins, en rouge, en mode
   `Overlay` pour les voir à travers le terrain.
3. **Chunk courant** : sur ses 4 faces verticales, une grille plus fine en jaune, avec
   une ligne verticale tous les 2 blocs, et une ligne horizontale tous les 2 blocs sur une
   plage verticale limitée autour du joueur (par exemple 32 blocs au-dessus et en dessous),
   pour ne pas générer des milliers de lignes inutiles.
4. **Tranches de 16 blocs** : des lignes horizontales plus visibles (en bleu) à chaque
   multiple de 16 en hauteur. Elles seront utiles le jour où tu découperas les chunks en
   sections de 16³.

Le calcul se fait dans une fonction dédiée, appelée depuis `Game::update()` ou
`World::update()` si `showChunkBorders` est actif.

### 3.3 Les AABB des entités

Dans `Entity` (ou `Player`), après la résolution des collisions : dessiner l'AABB à la
position finale. Pour le debug de la physique, tu peux aussi dessiner en orange l'AABB à la
position *avant* résolution, et les blocs testés pour la collision en gris : c'est très
parlant quand une collision se comporte mal.

### 3.4 Le raycast

Avec `showRaycast` : le rayon de l'œil jusqu'au point d'impact (ou jusqu'à la portée
maximale en l'absence d'impact), un `cross()` au point d'impact, et la normale de la face
touchée sous forme d'un court segment.

L'outline du bloc visé (`BlockOutline`) reste tel quel : c'est un élément de gameplay, pas
un outil de debug.

---

## Phase 4 : le profiler

### 4.1 Le buffer circulaire

Un conteneur générique de taille fixe, qui écrase les valeurs les plus anciennes :

```cpp
// src/debug/ring_buffer.h
template <typename T, size_t N>
class RingBuffer
{
public:
    void push(T value)
    {
        _data[_head] = value;
        _head = (_head + 1) % N;
        _size = std::min(_size + 1, N);
    }

    size_t size() const { return _size; }
    size_t offset() const { return _size < N ? 0 : _head; } // index de la valeur la plus ancienne
    const T *data() const { return _data.data(); }

private:
    std::array<T, N> _data{};
    size_t _head = 0;
    size_t _size = 0;
};
```

ImPlot sait afficher directement un buffer circulaire grâce au paramètre `offset` de
`ImPlot::PlotLine`, sans aucune copie.

Taille conseillée : 300 valeurs, soit environ 5 secondes à 60 FPS.

### 4.2 Les timers CPU

```cpp
// src/debug/profiler.h
namespace Profiler
{
    void beginFrame();
    void endFrame(); // pousse les totaux de la frame dans les historiques

    void addSample(const char *name, float ms);
}

class ScopedTimer
{
public:
    explicit ScopedTimer(const char *name)
        : _name(name), _start(std::chrono::steady_clock::now()) {}

    ~ScopedTimer()
    {
        const auto end = std::chrono::steady_clock::now();
        Profiler::addSample(_name, std::chrono::duration<float, std::milli>(end - _start).count());
    }

private:
    const char *_name;
    std::chrono::steady_clock::time_point _start;
};

#define PROFILE_CONCAT_(a, b) a##b
#define PROFILE_CONCAT(a, b) PROFILE_CONCAT_(a, b)
#define PROFILE_SCOPE(name) ScopedTimer PROFILE_CONCAT(_profileTimer, __LINE__)(name)
```

Détails de conception :

- **Les noms sont des littéraux** (`const char *`) : on peut les utiliser comme clés sans
  copier de chaîne. Une `std::unordered_map<const char *, Entry>` fonctionne tant que le même
  littéral est utilisé au même endroit, ce qui est le cas avec la macro.
- **Accumulation par frame** : une même section peut être mesurée plusieurs fois dans une
  frame (par exemple l'upload de plusieurs chunks). `addSample()` additionne dans un total
  de frame, et `endFrame()` pousse ce total dans l'historique puis le remet à zéro.
- **Coût quand c'est désactivé** : `ScopedTimer` lit l'horloge deux fois, ce qui coûte
  quelques dizaines de nanosecondes. C'est négligeable pour une dizaine de sections par frame.
  Évite simplement de mettre un `PROFILE_SCOPE` dans une boucle très serrée (par bloc ou
  par sommet).
- **Thread principal uniquement** pour commencer. Les temps des jobs du thread pool
  (génération, meshing) peuvent être mesurés dans le job, puis transmis avec le résultat
  et ajoutés au profiler sur le thread principal lors de la récupération.

Sections à mesurer au départ :

| Section | Emplacement |
|---|---|
| `Frame` | Toute la boucle dans `Game::run()` |
| `Input` | `Game::processInput()` |
| `Player` | `Player::update()` |
| `World::update` | `World::update()` |
| `Chunk upload` | Envoi des meshes au GPU dans `World::update()` |
| `Render` | `Game::render()` |
| `Chunk generation` | Dans le job, remonté avec le résultat |
| `Chunk meshing` | Dans le job, remonté avec le résultat |

Le calcul du FPS, provisoirement dans `DebugUI`, passe ici : le FPS affiché est la moyenne
de la section `Frame` sur les N dernières frames, ce qui donne une valeur stable et lisible.

### 4.3 Les timers GPU

Les appels OpenGL sont asynchrones : mesurer côté CPU autour d'un draw call ne mesure que
le temps d'envoi de la commande, pas le temps que le GPU passe à l'exécuter. Il faut des
*timer queries*.

Principe :

- Deux `GlQuery` de type `GL_TIMESTAMP` par section (début et fin), créées avec
  `glCreateQueries(GL_TIMESTAMP, ...)`.
- `glQueryCounter(query, GL_TIMESTAMP)` au début et à la fin de la section.
- Le résultat, en nanosecondes, se lit avec `glGetQueryObjectui64v(query, GL_QUERY_RESULT, &ns)`.

**Le piège** : lire le résultat dans la même frame force le CPU à attendre que le GPU ait
fini, ce qui détruit les performances qu'on essaie de mesurer. La solution est d'avoir
**trois jeux de queries** utilisés à tour de rôle, et de lire à la frame N les résultats de
la frame N-2 :

```
Frame N     : émet les queries du jeu (N % 3), lit celles du jeu ((N + 1) % 3)
```

Avant de lire, vérifier la disponibilité avec `GL_QUERY_RESULT_AVAILABLE`. Si le résultat
n'est pas encore prêt (GPU très en retard), on saute simplement cette mesure.

Interface proposée :

```cpp
class GpuTimer
{
public:
    void begin(const char *section);
    void end(const char *section);
    void endFrame(); // récupère les résultats d'il y a deux frames et les donne au Profiler
};

#define GPU_PROFILE_SCOPE(timer, name) /* RAII équivalent à ScopedTimer */
```

Sections GPU :

| Section | Contenu |
|---|---|
| `GPU Shadows` | Rendu des cascades d'ombres |
| `GPU Terrain` | Passe principale du terrain |
| `GPU Water` | Eau, réfraction et SSR |
| `GPU Debug` | `DebugDraw::endFrame()` |
| `GPU HUD` | HUD et menu |
| `GPU ImGui` | Rendu d'ImGui |

Les queries ne sont émises que si `debugSettings().gpuTimings` est actif. Quand on active
l'option, il faut ignorer les deux premières frames, dont les résultats n'existent pas encore.

### 4.4 Les compteurs

En plus des temps, des valeurs par frame ou globales :

- **Draw calls et sommets** : un compteur incrémenté dans `Renderer` à chaque draw call
  (le plus simple est une petite fonction `drawIndexed(mesh)` qui dessine et compte), remis
  à zéro à chaque frame.
- **Chunks** : chargés, rendus (après frustum culling), en attente de génération, en attente
  de meshing.
- **Thread pool** : jobs en file et en cours, avec `get_tasks_queued()` et
  `get_tasks_running()` de BS::thread_pool.
- **Mémoire CPU des chunks** : nombre de chunks chargés × taille d'un chunk. Ce chiffre sera
  un bon indicateur le jour où tu compresseras les chunks avec une palette.

### 4.5 Les graphiques

Dans la section **Performance** du panneau, avec ImPlot :

**Temps de frame** : une courbe de la section `Frame`, avec deux lignes de référence
horizontales à 16,6 ms (60 FPS) et 33,3 ms (30 FPS), tracées avec `ImPlot::PlotInfLines`
et le flag `ImPlotInfLinesFlags_Horizontal`. Fixe l'axe Y (par exemple de 0 à 50 ms) pour
que le graphique ne change pas d'échelle en permanence, ce qui le rendrait illisible.

**Répartition CPU** : une courbe par section (`Player`, `World::update`, `Chunk upload`,
`Render`), sur le même graphique, pour voir laquelle provoque un pic.

**Répartition GPU** : pareil avec les sections GPU, quand `gpuTimings` est actif.

**Texte récapitulatif** au-dessus des graphiques : FPS moyen, temps de frame moyen, minimum
et maximum sur la fenêtre de l'historique. Le maximum est souvent plus révélateur que la
moyenne : ce sont les pics qui provoquent les saccades.

Tous les graphiques utilisent le paramètre `offset` du buffer circulaire, sans copie.

---

## Phase 5 : les extras

À faire selon les besoins, dans l'ordre qui te semble utile.

### 5.1 Geler le culling

Avec `freezeCulling`, le frustum utilisé pour le culling n'est plus mis à jour : on garde
celui du moment de l'activation, alors que la caméra continue de bouger. En s'éloignant, on
voit exactement quels chunks sont coupés. Dessine le frustum gelé avec `DebugDraw::frustum()`.

Il suffit de stocker une copie du `Frustum` et de la matrice view-projection au moment de
l'activation, et d'utiliser cette copie dans le culling tant que le flag est actif.

### 5.2 Visualiser les cascades d'ombres

Un uniform `debugCascades` dans le shader du terrain : quand il est actif, la couleur finale
est teintée selon l'indice de la cascade utilisée pour le fragment (rouge, vert, bleu,
jaune). Indispensable pour régler les distances de coupure des cascades.

### 5.3 Les vues de debug

Un uniform `debugView` (un entier) dans les shaders du terrain et de l'eau, exposé sous forme
de liste déroulante dans F3 :

| Valeur | Affichage |
|---|---|
| 0 | Rendu normal |
| 1 | Ambient occlusion seule |
| 2 | Normales |
| 3 | Facteur d'ombre seul |
| 4 | Albedo sans éclairage |
| 5 | Coordonnées UV |

Chaque vue n'est qu'un `if` en fin de fragment shader, qui remplace la couleur finale.

### 5.4 Le visualiseur de textures

Afficher les textures internes directement dans ImGui avec
`ImGui::Image((ImTextureID)(intptr_t)textureId, size)` :

- chaque cascade de la shadow map (c'est un `GL_TEXTURE_2D_ARRAY` : il faudra une vue par
  couche, avec `glTextureView`, ou un petit shader qui copie une couche dans une texture 2D) ;
- le buffer de profondeur ;
- la copie de la scène utilisée pour la réfraction et le SSR.

Les textures de profondeur s'affichent en niveaux de gris très sombres par défaut : il faudra
linéariser la profondeur pour obtenir une image lisible.

### 5.5 Les infos du bloc visé

Dans la section **Général**, à partir du résultat du raycaster : type de bloc, position,
coordonnées du chunk et position locale dans le chunk, face visée. Plus tard, le niveau de
lumière du bloc quand l'éclairage par blocs existera.

### 5.6 Le mode noclip

Un flag `noclip` qui désactive la gravité et les collisions du joueur, avec une vitesse de
déplacement réglable. Indispensable pour explorer rapidement le terrain et, plus tard,
les grottes.

---

## Phase 6 : finalisation

1. Vérifier que tous les outils désactivés ne coûtent rien : comparer le temps de frame
   avec tous les outils éteints avant et après la branche.
2. Vérifier le build Release : aucun avertissement, et la sortie de debug d'OpenGL bien
   désactivée.
3. Mettre à jour la documentation Doxygen des nouvelles classes, et ajouter une page
   `docs/pages/debug_tools.md` qui liste les outils et les raccourcis.
4. Mettre à jour le README : section Features (outils de debug) et tableau des contrôles
   (raccourcis F3).
5. Merger `feature/debug-tools` dans `main`.
6. Taguer et publier la release :

```bash
git tag -a v0.2.0 -m "v0.2.0: runtime debug tools"
git push origin v0.2.0
```

---

## Checklist récapitulative

### Phase 0 : fondations
- [ ] Template `GlHandle` et types `GlBuffer`, `GlVertexArray`, `GlTexture`, `GlFramebuffer`, `GlQuery`
- [ ] Fonctions de création en DSA
- [ ] `gl::setContextThread()` appelé par `Window`, assertions de thread
- [ ] Migration de `Mesh`
- [ ] Migration de `FrameBuffer`
- [ ] Migration de `Texture`
- [ ] Migration de `CascadedShadowMap`, `BlockTextureAtlas`, `HudRenderer`, `BlockOutline`
- [ ] Plus aucun `glGen*` / `glDelete*` hors de `src/graphics/gl/`
- [ ] Sortie de debug OpenGL en build Debug
- [ ] Documentation de `Window` mise à jour (OpenGL 4.6)
- [ ] Buffers en `glNamedBufferStorage` (meshes, HUD, outline)
- [ ] Un VAO par format de sommet, partagé (`glVertexArrayAttribFormat`)
- [ ] Textures en `glTextureStorage*` + `glTextureSubImage*`
- [ ] `glBindTextureUnit` et `layout(binding = N)` pour tous les samplers
- [ ] Header `texture_units.h`
- [ ] Uniform buffer `FrameData` (alignement std140 vérifié par `static_assert`)
- [ ] Mécanisme d'include dans la classe `Shader`
- [ ] `chunkOffset` à location fixe à la place de la matrice `model`
- [ ] Cache ou locations fixes pour les uniforms restants
- [ ] Framebuffers en DSA, `glBlitNamedFramebuffer`
- [ ] Scène rendue dans un FBO HDR multisamplé, `GLFW_SAMPLES` retiré
- [ ] Geometry shader des ombres instancié (`invocations = 5`)
- [ ] Reverse-Z (`glClipControl`, profondeur 32F, shaders de l'eau adaptés)
- [ ] `GLFW_OPENGL_FORWARD_COMPAT` retiré, épaisseur de l'outline corrigée
- [ ] Labels d'objets et groupes de debug par passe
- [ ] Nouvelles touches dans l'enum `Key`
- [ ] Gestion des combinaisons `F3 + touche`

### Phase 1 : module de debug
- [ ] `DebugSettings` et `debugSettings()`
- [ ] `DebugUI` avec `DebugFrameInfo`
- [ ] Panneau déplacé hors de `Renderer` (comportement identique)
- [ ] `_showDebug` de `Game` remplacé
- [ ] Panneau découpé en sections repliables

### Phase 2 : DebugDraw
- [ ] Shaders `debug_line`
- [ ] `beginFrame()` / `endFrame()` et les deux modes
- [ ] `line()`, `box()`, `aabb()`, `frustum()`, `cross()`, `gridXZ()`
- [ ] Garde-fous : thread principal, plafond de sommets, retour immédiat si vide

### Phase 3 : visualisations
- [ ] Wireframe (hors passe d'ombres)
- [ ] Bordures de chunks
- [ ] AABB des entités
- [ ] Visualisation du raycast

### Phase 4 : profiler
- [ ] `RingBuffer`
- [ ] `ScopedTimer` et `PROFILE_SCOPE`
- [ ] Sections CPU instrumentées
- [ ] Temps des jobs remontés au thread principal
- [ ] FPS calculé par le profiler
- [ ] `GpuTimer` avec trois jeux de queries
- [ ] Sections GPU instrumentées
- [ ] Compteurs (draw calls, chunks, thread pool, mémoire des chunks)
- [ ] Graphiques ImPlot (temps de frame, CPU, GPU)

### Phase 5 : extras
- [ ] Geler le culling
- [ ] Visualisation des cascades
- [ ] Vues de debug (`debugView`)
- [ ] Visualiseur de textures
- [ ] Infos du bloc visé
- [ ] Mode noclip

### Phase 6 : finalisation
- [ ] Vérification du coût des outils désactivés
- [ ] Documentation et page `debug_tools.md`
- [ ] README mis à jour
- [ ] Merge dans `main`
- [ ] Release `v0.2.0`