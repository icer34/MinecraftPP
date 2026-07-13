cmake_minimum_required(VERSION 3.20)
project(MinecraftPP VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# ─── stb (header-only) ───────────────────────────────────────────────────────
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4
)
FetchContent_MakeAvailable(stb)

# ─── Glad (généré pour ce projet, pas de repo à fetch → reste committé) ──────
add_library(glad STATIC
    libs/glad/src/glad.c
)
target_include_directories(glad PUBLIC libs/glad/include)

# ─── Exécutable principal ─────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/util/window.cpp
    src/game.cpp
    src/graphics/mesh.cpp
    src/graphics/shader.cpp
    src/game/blocks.cpp
    src/graphics/chunk_mesh.cpp
    src/graphics/camera.cpp
    src/graphics/chunk_mesher.cpp
    src/graphics/texture_atlas.cpp
    src/game/world.cpp
    src/graphics/renderer.cpp
    src/game/terrain_generator.cpp
    src/util/perlin_noise.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    libs/glad/include
    ${glfw_SOURCE_DIR}/include
    ${glm_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${stb_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    glfw
    glm::glm
    imgui
)

# S'assure qu'imgui est compilé avant MinecraftPP
add_dependencies(${PROJECT_NAME} imgui)

# Copie les shaders/assets dans le répertoire de build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets  $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)
