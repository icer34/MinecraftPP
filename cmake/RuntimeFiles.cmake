# ─── Runtime files (shaders, assets) ─────────────────────────────────────────
# copy_runtime_files(<target>...)
#
# Copies shaders/ and assets/ next to each given executable. This is a separate
# target built with ALL, so it runs on every build and not only when an
# executable is relinked (otherwise editing a .glsl file alone would never be
# copied, since no .cpp changed).

function(copy_runtime_files)
    set(_commands "")

    foreach(target IN LISTS ARGN)
        list(APPEND _commands
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:${target}>/shaders
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/assets  $<TARGET_FILE_DIR:${target}>/assets
        )
    endforeach()

    add_custom_target(copy_runtime_files ALL
        ${_commands}
        COMMENT "Copying shaders and assets to build output directories"
    )

    foreach(target IN LISTS ARGN)
        add_dependencies(${target} copy_runtime_files)
    endforeach()
endfunction()