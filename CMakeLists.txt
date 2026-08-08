cmake_minimum_required(VERSION 3.20)
project(MinecraftPP VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# off by default (slower builds, and not compatible with /RTC or incremental linking) --
# turn on with -DENABLE_ASAN=ON to hunt memory corruption/UB bugs
option(ENABLE_ASAN "Build engine_lib/game_lib/MinecraftPP/tests with AddressSanitizer" OFF)

include(FetchContent)

# ─── GLFW ────────────────────────────────────────────────────────────────────
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        b00e6a8a88ad1b60c0a045e696301deb92c9a13e
)
FetchContent_MakeAvailable(glfw)

# ─── GLM ─────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        6f14f4792a0cde5d0cf2c910506724d61cb95834
)
FetchContent_MakeAvailable(glm)

# ─── ImGui (pas de CMakeLists → intégration manuelle) ────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        4b80d409e721b096b2b26e5edbf575178bdb48f0
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PRIVATE glfw)

# ─── ImPlot (pas de CMakeLists → intégration manuelle, comme imgui) ──────────
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG        master # v0.16 is incompatible with our pinned ImGui (1.92.8, post-1.90 API changes)
)
FetchContent_MakeAvailable(implot)

add_library(implot STATIC
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(implot PUBLIC
    ${implot_SOURCE_DIR}
)
target_link_libraries(implot PRIVATE imgui)

# ─── stb (header-only) ───────────────────────────────────────────────────────
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4
)
FetchContent_MakeAvailable(stb)

# ─── BS::thread_pool (header-only) ───────────────────────────────────────────
FetchContent_Declare(
    bs_thread_pool
    GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG        v5.0.0 # verifie que ce tag existe encore -- ajuste si FetchContent echoue
)
FetchContent_MakeAvailable(bs_thread_pool)

# ─── Glad (généré pour ce projet, pas de repo à fetch → reste committé) ──────
add_library(glad STATIC
    engine/libs/glad/src/glad.c
)
target_include_directories(glad PUBLIC engine/libs/glad/include)

# ─── engine_lib (voxel engine générique -- aucun code spécifique à MinecraftPP) ─
# séparée du jeu pour pouvoir être réutilisée par un autre jeu, et pour que
# tests/ puisse linker dessus seule (comme un deuxième "jeu" minimal)
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "engine/src/*.cpp")

add_library(engine_lib STATIC
    ${ENGINE_SOURCES}
)

target_include_directories(engine_lib PUBLIC
    engine/src
    engine/libs/glad/include
    ${glfw_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${bs_thread_pool_SOURCE_DIR}/include
)

target_link_libraries(engine_lib PUBLIC
    glad
    glfw
    glm::glm
    imgui
    implot
)

# S'assure qu'imgui/implot sont compilés avant engine_lib
add_dependencies(engine_lib imgui implot)

# ─── game_lib (logique spécifique à MinecraftPP : blocs, terrain, joueur) ────
# séparée de l'exécutable principal pour la même raison qu'engine_lib/tests
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "game/src/*.cpp")
list(REMOVE_ITEM GAME_SOURCES "${CMAKE_SOURCE_DIR}/game/src/main.cpp")

add_library(game_lib STATIC
    ${GAME_SOURCES}
)

target_include_directories(game_lib PUBLIC
    game/src
)

target_link_libraries(game_lib PUBLIC
    engine_lib
)

# ─── Exécutable principal ─────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    game/src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    game_lib
)

# ─── Exécutable de tests ──────────────────────────────────────────────────────
# linke uniquement engine_lib (pas game_lib) -- sert aussi de vérification que
# l'engine reste utilisable seul, sans rien de spécifique à MinecraftPP
file(GLOB_RECURSE MINECRAFTPP_TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")

add_executable(tests
    ${MINECRAFTPP_TEST_SOURCES}
)

target_link_libraries(tests PRIVATE
    engine_lib
)

# ─── AddressSanitizer (voir option ENABLE_ASAN plus haut) ────────────────────
# applique uniquement à notre code (pas aux libs fetch -- glfw/imgui/implot/glad n'ont
# pas besoin d'être instrumentées, et ça éviterait de tout recompiler pour rien)
if(ENABLE_ASAN)
    # MSVC est vrai pour cl.exe ET pour clang-cl (qui simule son frontend) -- il faut donc
    # exclure explicitement Clang ici pour ne prendre la branche /fsanitize (syntaxe cl.exe)
    # que pour le vrai cl.exe ; clang-cl passe par la branche -fsanitize (syntaxe Clang) comme
    # n'importe quel Clang, plus fiable que de compter sur sa compat avec la syntaxe à slash.
    if(MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        foreach(target engine_lib game_lib ${PROJECT_NAME} tests)
            target_compile_options(${target} PRIVATE /fsanitize=address /Zi)
        endforeach()
        # ASan sous MSVC est incompatible avec /RTC (ajouté automatiquement en Debug)
        # et avec le linking incrémental
        string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL:NO)
        target_link_options(tests PRIVATE /INCREMENTAL:NO)
    else()
        foreach(target engine_lib game_lib ${PROJECT_NAME} tests)
            target_compile_options(${target} PRIVATE -fsanitize=address -g)
            target_link_options(${target} PRIVATE -fsanitize=address)
        endforeach()
    endif()
endif()

# expose la racine du projet en macro pour que les tests puissent écrire/lire
# des fichiers à un endroit fixe, peu importe le working directory courant
# (celui-ci dépend de comment l'exe est lancé -- VS Code, terminal, etc.)
target_compile_definitions(tests PRIVATE
    PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}"
)

# Copie les shaders/assets dans le répertoire de build -- cible séparée avec ALL pour
# qu'elle tourne à chaque build, pas seulement quand MinecraftPP doit être relinké
# (sinon modifier un .glsl seul ne recopie jamais rien, vu qu'aucun .cpp n'a changé)
add_custom_target(copy_runtime_files ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/engine/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/engine/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/game/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/game/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/shaders $<TARGET_FILE_DIR:tests>/engine/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/assets $<TARGET_FILE_DIR:tests>/engine/assets
    COMMENT "Copying shaders and assets to build output directories"
)
add_dependencies(${PROJECT_NAME} copy_runtime_files)
add_dependencies(tests copy_runtime_files)
