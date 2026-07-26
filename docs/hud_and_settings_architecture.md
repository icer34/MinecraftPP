# Architecture HUD & Settings

Document de référence pour l'implémentation du HUD (crosshair, hotbar, chat) et
d'un système de settings custom (sans ImGui). Résume les décisions prises et le
rôle de chaque classe — pas une implémentation, un plan à suivre.

## Principe directeur

Séparer clairement quatre responsabilités qui n'ont pas à se connaître :

1. **Dessiner des pixels à l'écran** (`HudRenderer`) — ne sait rien du jeu.
2. **Dessiner un item** (`ItemRenderer`) — ne sait rien du HUD ni des menus.
3. **Composer des éléments de jeu à partir de primitives** (`Hud`, `SettingsMenu`) —
   ne sait rien du GPU.
4. **Déclarer quels settings existent** (`SettingsRegistry`) — ne sait rien de
   comment ils sont affichés.

Chaque couche ne dépend que de celle en dessous. Aucune classe "boîte à tout
faire" qui accumule des variables au fil du temps.

```
Game / autres systèmes (Renderer, TerrainGenerator, BlockTextureAtlas...)
        │  possèdent leurs propres variables, s'enregistrent dans...
        ▼
SettingsRegistry            (données pures : quels settings existent)
        │
        ▼
SettingsMenu   ┐            (composition : quoi dessiner, quelle logique)
Hud            ┤
ItemRenderer   ┘
        │  appellent des primitives génériques de...
        ▼
HudRenderer                 (bas niveau : quads/texte texturés, batching GPU)
```

---

## `HudRenderer`

**Rôle** : dessiner des quads 2D texturés en espace écran, et rien d'autre. Ne
connaît aucun concept de jeu (pas de crosshair, pas d'item, pas de bouton).

**Ce qu'il possède** : la projection orthographique, l'atlas UI (icônes + police
+ un pixel blanc de réserve), le batch de quads de la frame en cours, le
shader/pipeline 2D.

**Méthodes** :
- `begin()` — reset le batch, bind l'atlas UI, configure la projection ortho.
- `drawQuad(vec2 pos, vec2 size, vec2 uvMin, vec2 uvMax, vec4 color = white)` —
  la seule vraie primitive. Une icône = uv réelle + couleur blanche. Un quad de
  couleur unie (fond du chat, barres du crosshair) = uv du pixel blanc de
  réserve + couleur voulue.
- `drawText(const std::string &text, vec2 pos, float scale, vec4 color)` —
  boucle sur les caractères, appelle `drawQuad` par glyphe avec l'uv du glyphe
  dans l'atlas police. Pas un chemin de rendu séparé.
- `end()` — upload le batch accumulé, un seul (ou très peu de) draw call pour
  toute la frame de HUD.

**Ce qu'il ne fait pas** : dessiner un item 3D (voir `ItemRenderer`), savoir ce
qu'est un bouton ou un slider (voir `SettingsMenu`), stocker le moindre état de
jeu.

**Décision** : deux textures séparées (police + icônes), donc deux jeux de
vecteurs/buffers/draw calls (`m_iconVertData`/`m_iconIdxData` et
`m_textVertData`/`m_textIdxData`). Plus de code dupliqué que la fusion en un
seul atlas, mais plus simple à raisonner, et le coût réel (2 draw calls au
lieu d'1 pour tout le HUD par frame) est négligeable à cette échelle. À
revisiter (fusion en un seul atlas) seulement si ça devient un vrai problème,
pas préventivement.

**Décision** : pas de classe `UiAtlas` séparée — construction du buffer +
lookup nom→UV vivent dans `HudRenderer` lui-même. Contrairement à
`BlockTextureAtlas`, il n'y a ici aucune mipmap à générer (les éléments HUD
s'affichent toujours à l'échelle 1:1, jamais minifiés), donc la complexité qui
justifiait une classe séparée pour l'atlas de blocs ne s'applique pas — et
rien d'autre que `HudRenderer` n'a besoin d'utiliser cet atlas indépendamment.
Pour que ça reste lisible malgré tout dans une seule classe : garder le
chargement (`loadAtlas()`) et le lookup (`getIconUV()`) dans des méthodes
privées bien séparées, pas mélangés au constructeur ni dispersés entre les
appels de dessin — extraction facile plus tard si un besoin de réutilisation
apparaît.

### Lookup nom → UV (comment les appelants trouvent leurs coordonnées)

`drawQuad` prend des UV bruts, mais aucun appelant ne doit les calculer/hardcoder
lui-même — même principe que `BlockTextureAtlas::getIndex()`, transposé côté
HUD : un lookup construit une fois au chargement, interrogé par nom symbolique.

```cpp
struct UVRect { vec2 min, max; };

UVRect getIconUV(const std::string &name) const;   // rempli via un
                                                     // unordered_map<string, UVRect>
                                                     // au chargement de l'atlas UI
UVRect getGlyphUV(char c) const;                    // idem, indexé par caractère,
                                                     // + métriques (avance, taille)
UVRect getWhitePixelUV() const;                     // le pixel de réserve pour
                                                     // les quads de couleur unie
```

Méthodes de confort qui font lookup + draw en un appel, pour que `Hud` et
`SettingsMenu` ne manipulent jamais d'UV bruts :
- `drawIcon(const std::string &name, vec2 pos, vec2 size, vec4 color = white)`
- `drawColoredQuad(vec2 pos, vec2 size, vec4 color)` — wrapper fin autour de
  `getWhitePixelUV()` + `drawQuad`.

---

## `ItemRenderer`

**Rôle** : savoir dessiner *un item*, peu importe où il apparaît (hotbar,
inventaire, item lâché au sol, tooltip). Le seul endroit du code qui sait faire
la différence entre un item-sprite plat et un item-bloc (rendu 3D miniature).

**Méthodes** :
- `drawItem(ItemStack item, vec2 screenPos, float size)` — point d'entrée
  unique utilisé par `Hud`, un futur écran d'inventaire, etc.

**Décision d'architecture (voir discussion)** : rendu 3D live à chaque frame,
avec sa propre petite projection HUD-space — pas de cache/bake, correspond à
comment Minecraft rend les items-blocs. Nécessite un clear du depth buffer
entre la passe 3D du monde et cette passe (sinon les items du HUD s'enterrent
dans la géométrie).

**Alternatives envisagées et pourquoi pas (pour référence)** :
- *Icônes précalculées dans l'atlas* : rendu trivial (juste un quad de plus
  dans `HudRenderer`), mais aucune dynamique possible (glint, animation) sans
  un fallback live de toute façon.
- *Rendu vers texture avec cache* : flexible, mais ajoute une vraie gestion de
  cache/invalidation pour un gain qui ne sert que si des items changent
  d'apparence dynamiquement.

**Ce qu'il ne fait pas** : savoir où positionner les slots de la hotbar (c'est
`Hud` qui décide *où* appeler `drawItem`), gérer l'inventaire lui-même.

---

## `Hud`

**Rôle** : orchestrateur du HUD de jeu. Compose les primitives de `HudRenderer`
et `ItemRenderer` pour dessiner crosshair, hotbar, chat. Le seul endroit qui
connaît la mise en page (positions, tailles) de ces éléments.

**Possède** : un `HudRenderer`, un `ItemRenderer`, l'état propre au HUD qui ne
vit nulle part ailleurs (historique du chat + timers d'affichage, éventuel
slot de hotbar survolé).

**Méthodes** (une par élément, privées, appelées depuis `render()`) :
- `render(const Player &player)` — `begin()` / dessine tout / `end()`.
- `drawCrosshair()` — deux `drawColoredQuad` (barres) au centre écran.
- `drawHotbar(const Inventory &inv)` — fond de slots via `drawQuad`, puis
  `m_itemRenderer.drawItem(...)` par slot.
- `drawChat()` — fond semi-transparent + `drawText` par ligne, purge les
  messages expirés selon leurs timers.

**Ce qu'il ne fait pas** : du rendu bas niveau (délégué à `HudRenderer`), de la
logique de jeu (inventaire, dégâts — il *lit* l'état du joueur, ne le modifie
pas).

---

## `SettingsRegistry`

**Rôle** : registre générique des settings interactifs (pas des stats en
lecture seule — voir plus bas). Ne sait rien de l'affichage.

```cpp
struct FloatSetting { std::string label; float *value; float min, max; };
struct BoolSetting  { std::string label; bool *value; };
using Setting = std::variant<FloatSetting, BoolSetting /*, ... */>;
```

**Méthodes** :
- `addFloat(std::string label, float *value, float min, float max)`
- `addBool(std::string label, bool *value)`
- `getAll() const -> const std::vector<Setting>&`

**Principe clé** : le registre ne *possède* pas les valeurs, il référence par
pointeur des variables qui vivent dans leur système d'origine (l'anisotropie
reste un membre de `BlockTextureAtlas`, le FOV reste un membre de `Camera`).
Chaque système s'enregistre lui-même au démarrage :
```cpp
// dans le setup de BlockTextureAtlas
settings.addFloat("Anisotropy", &m_maxAnisotropy, 1.0f, 16.0f);
```
C'est ce qui évite la prolifération de variables dans une classe centrale —
la donnée reste où elle est logiquement utilisée, seule une référence est
partagée.

**Ce qu'il ne fait pas** : dessiner quoi que ce soit, savoir ce qu'est un
slider.

---

## `SettingsMenu`

**Rôle** : composer un menu de settings à partir d'un `SettingsRegistry`, en
utilisant les primitives de `HudRenderer`. Symétrique à `Hud` : une couche de
composition au-dessus du rendu bas niveau.

**Méthodes** :
- `render(const SettingsRegistry &settings)` — boucle sur `settings.getAll()`,
  `std::visit` pour dispatcher par type vers le bon widget.
- `drawWidgetFor(const FloatSetting &)` (privé) — slider : une barre de fond
  (`drawColoredQuad`) + une poignée + un label (`drawText`), lit/écrit
  directement `*setting.value`.
- `drawWidgetFor(const BoolSetting &)` (privé) — checkbox : un quad +
  éventuellement une coche + un label.

**Ce qu'il ne fait pas** : connaître individuellement "l'anisotropie" ou "le
FOV" — il ne voit que des `FloatSetting`/`BoolSetting` génériques.

---

## Stats vs settings — ne pas confondre

Ce qui est actuellement dans `Renderer::renderDebug()` (FPS, nombre de
chunks, position caméra, échantillons de bruit) est de la **télémétrie en
lecture seule**, pas des settings. Pas besoin de `SettingsRegistry` ni de
membres persistants pour ça : ce sont des variables **locales** dans la
fonction de rendu, calculées puis passées directement à `HudRenderer::drawText`
— rien à stocker entre deux frames. Le `SettingsRegistry` est réservé à ce que
l'utilisateur peut réellement modifier via un widget.

---

## Ordre d'implémentation suggéré

1. `HudRenderer` seul (quads + texte), testé avec un crosshair statique en dur.
2. `Hud` qui utilise `HudRenderer` pour crosshair + chat (pas d'item pour
   l'instant).
3. `ItemRenderer` (rendu live d'un item-bloc), branché dans `Hud::drawHotbar`.
4. `SettingsRegistry`, avec un ou deux settings existants migrés dedans
   (anisotropie, niveaux de mip) pour valider le pattern.
5. `SettingsMenu` au-dessus, avec les deux types de widgets (slider, checkbox).