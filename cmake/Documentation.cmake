# ─── Doxygen documentation ───────────────────────────────────────────────────
# Enabled with -DBUILD_DOCS=ON (option declared in the root CMakeLists.txt),
# then generated with: cmake --build build --target docs
# Output: docs/html/index.html

if(NOT BUILD_DOCS)
    return()
endif()

find_package(Doxygen REQUIRED OPTIONAL_COMPONENTS dot)

# Modern theme for the generated HTML (pure CSS, no build step)
FetchContent_Declare(
    doxygen_awesome
    GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
    GIT_TAG        v2.3.4
)
FetchContent_MakeAvailable(doxygen_awesome)

# ── General ──
set(DOXYGEN_PROJECT_NAME        "MinecraftPP")
set(DOXYGEN_PROJECT_BRIEF       "A voxel sandbox engine written from scratch in C++ and OpenGL")
set(DOXYGEN_OUTPUT_DIRECTORY    ${CMAKE_SOURCE_DIR}/docs)
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE ${CMAKE_SOURCE_DIR}/README.md)
set(DOXYGEN_IMAGE_PATH          ${CMAKE_SOURCE_DIR}/docs/images)
set(DOXYGEN_STRIP_FROM_PATH     ${CMAKE_SOURCE_DIR}/src)
set(DOXYGEN_STRIP_FROM_INC_PATH ${CMAKE_SOURCE_DIR}/src)

# ── What to extract ──
set(DOXYGEN_EXTRACT_ALL         NO)
set(DOXYGEN_EXTRACT_PRIVATE     NO)
set(DOXYGEN_EXTRACT_STATIC      YES)
set(DOXYGEN_JAVADOC_AUTOBRIEF   YES)  # first sentence of a /** */ block becomes the @brief
set(DOXYGEN_RECURSIVE           YES)
set(DOXYGEN_EXCLUDE_PATTERNS    "*/stb_image_write_impl.cpp")

# ── Warnings ──
set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
set(DOXYGEN_WARN_NO_PARAMDOC     YES)

# ── HTML output ──
set(DOXYGEN_GENERATE_HTML       YES)
set(DOXYGEN_GENERATE_LATEX      NO)
set(DOXYGEN_GENERATE_TREEVIEW   YES)
set(DOXYGEN_DISABLE_INDEX       NO)
set(DOXYGEN_FULL_SIDEBAR        NO)
set(DOXYGEN_HTML_COLORSTYLE     LIGHT) # required by doxygen-awesome
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${doxygen_awesome_SOURCE_DIR}/doxygen-awesome.css)

# ── Graphs (only if Graphviz is installed) ──
if(DOXYGEN_DOT_FOUND)
    set(DOXYGEN_HAVE_DOT          YES)
    set(DOXYGEN_CLASS_GRAPH       YES)
    set(DOXYGEN_COLLABORATION_GRAPH YES)
    set(DOXYGEN_INCLUDE_GRAPH     NO)
    set(DOXYGEN_INCLUDED_BY_GRAPH NO)
    set(DOXYGEN_DOT_IMAGE_FORMAT  svg)
    set(DOXYGEN_INTERACTIVE_SVG   YES)
endif()

doxygen_add_docs(docs
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/docs/pages
    COMMENT "Generating Doxygen documentation"
)