# Utils.cmake — shared CMake helpers for cpp-lkm and the top-level project.

# cpp_lkm_assert_nonempty(<context> <name> <str>)
# Fails configuration if str is empty (after evaluation). Use for positional parameters
# and other single values.
#
# Note: do not name the third formal parameter `value`; CMake 4 may treat it specially and
# it will not receive the caller's argument reliably when nested inside other functions.
function(cpp_lkm_assert_nonempty context name str)
    if("${str}" STREQUAL "")
        message(
            FATAL_ERROR
                "${context}: ${name} is required (non-empty)."
        )
    endif()
endfunction()

# cpp_lkm_assert_nonempty_vars(<context> <var_prefix> <suffix1> [<suffix2> ...])
# For each suffix, verifies ${var_prefix}_${suffix} is defined and non-empty.
# Typical use after cmake_parse_arguments(... PREFIX ...).
#
# Note: do not name the second formal parameter `prefix`; CMake 4 may not pass it correctly
# when this helper is called from another function.
function(cpp_lkm_assert_nonempty_vars context var_prefix)
    foreach(_suffix ${ARGN})
        set(_varname "${var_prefix}_${_suffix}")
        if("${${_varname}}" STREQUAL "")
            message(
                FATAL_ERROR
                    "${context}: ${_suffix} is required (non-empty)."
            )
        endif()
    endforeach()
endfunction()
