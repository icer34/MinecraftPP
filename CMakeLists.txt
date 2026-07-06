cmake_minimum_required(VERSION 3.20)
project(MinecraftPP VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Glad (pas de CMakeLists → on l'intègre manuellement) ───────────────────
add_library(glad STATIC
    libs/glad/src/glad.c
)
target_include_directories(glad PUBLIC libs/glad/include)

# ─── GLFW ────────────────────────────────────────────────────────────────────
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/glfw)

# ─── GLM ─────────────────────────────────────────────────────────────────────
add_subdirectory(libs/glm)

# ─── ImGui (pas de CMakeLists → intégration manuelle) ────────────────────────
add_library(imgui STATIC
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    libs/imgui
    libs/imgui/backends
)
target_link_libraries(imgui PRIVATE glfw)

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
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    libs/glad/include
    libs/glfw/include
    libs/glm
    libs/imgui
    libs/imgui/backends
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