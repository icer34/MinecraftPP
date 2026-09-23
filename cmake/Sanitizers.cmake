# ─── AddressSanitizer ────────────────────────────────────────────────────────
# enable_sanitizers(<target>...)
#
# Instruments only the given targets (our own code). Third-party libraries
# (glfw, imgui, implot, glad) are left untouched: they don't need it, and it
# avoids recompiling them for nothing.

function(enable_sanitizers)
    if(MSVC)
        # ASan under MSVC is incompatible with /RTC (added by default in Debug).
        # CMAKE_CXX_FLAGS_DEBUG is a directory-level variable, so the change is
        # pushed to the caller's scope.
        string(REGEX REPLACE "/RTC[^ ]*" "" _debug_flags "${CMAKE_CXX_FLAGS_DEBUG}")
        set(CMAKE_CXX_FLAGS_DEBUG "${_debug_flags}" PARENT_SCOPE)
    endif()

    foreach(target IN LISTS ARGN)
        get_target_property(_type ${target} TYPE)

        if(MSVC)
            target_compile_options(${target} PRIVATE /fsanitize=address /Zi)
            # ...and with incremental linking (only meaningful for executables)
            if(_type STREQUAL "EXECUTABLE")
                target_link_options(${target} PRIVATE /INCREMENTAL:NO)
            endif()
        else()
            target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
            if(_type STREQUAL "EXECUTABLE")
                target_link_options(${target} PRIVATE -fsanitize=address)
            endif()
        endif()
    endforeach()

    message(STATUS "AddressSanitizer enabled for: ${ARGN}")
endfunction()