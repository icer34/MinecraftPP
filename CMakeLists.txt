cmake_minimum_required(VERSION 3.20)
project(MinecraftPP VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# ─── Options ─────────────────────────────────────────────────────────────────
# Off by default (slower builds, incompatible with /RTC and incremental linking).
# Turn on with -DENABLE_ASAN=ON to hunt memory corruption / UB bugs.
option(ENABLE_ASAN "Build engine_lib, MinecraftPP and tests with AddressSanitizer" OFF)
option(BUILD_DOCS  "Generate the Doxygen documentation (target: docs)"             OFF)

# ─── Modules ─────────────────────────────────────────────────────────────────
include(Dependencies)   # third-party libraries (GLFW, GLM, ImGui, ImPlot, stb, glad...)
include(Sanitizers)     # enable_sanitizers()
include(RuntimeFiles)   # copy_runtime_files()
include(Documentation)  # docs target, only when BUILD_DOCS=ON

# ─── engine_lib ──────────────────────────────────────────────────────────────
# All the engine/game logic without the entry point, so that other executables
# (tests/) can link against it and call individual functions without main().
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM ENGINE_SOURCES "${CMAKE_SOURCE_DIR}/src/main.cpp")

add_library(engine_lib STATIC ${ENGINE_SOURCES})

target_include_directories(engine_lib PUBLIC
    src
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

# ─── Game executable ─────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE engine_lib)

# ─── Tests executable ────────────────────────────────────────────────────────
# Separate main(): include any header from src/ to test functions individually,
# without going through the game's main() or opening a window.
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")

add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE engine_lib)

# Project root exposed as a macro so tests can read/write files at a fixed place,
# whatever the current working directory is (VS Code, terminal, etc.).
target_compile_definitions(tests PRIVATE PROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

# ─── Target-level features ───────────────────────────────────────────────────
if(ENABLE_ASAN)
    enable_sanitizers(engine_lib ${PROJECT_NAME} tests)
endif()

copy_runtime_files(${PROJECT_NAME} tests)