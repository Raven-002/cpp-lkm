# cmake/cpp/FormatTargets.cmake Defines the `format`, `format-check`, `cmake-format`,
# `cmake-format-check`, and `cmake-lint` targets. Included from the root CMakeLists.txt.

find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(
        GLOB_RECURSE
        FORMAT_SOURCES
        "${CMAKE_SOURCE_DIR}/src/**/*.cpp"
        "${CMAKE_SOURCE_DIR}/src/**/*.h"
        "${CMAKE_SOURCE_DIR}/src/**/*.hpp"
        "${CMAKE_SOURCE_DIR}/cpp-lkm/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/cpp-lkm/include/*.hpp"
        "${CMAKE_SOURCE_DIR}/cpp-lkm/include/*.h"
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
        "${CMAKE_SOURCE_DIR}/tests/*.hpp"
        "${CMAKE_SOURCE_DIR}/tests/*.h")

    add_custom_target(
        format
        COMMAND "${CLANG_FORMAT_EXE}" -i ${FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running clang-format"
        VERBATIM)

    add_custom_target(
        format-check
        COMMAND "${CLANG_FORMAT_EXE}" --dry-run -Werror ${FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Checking clang-format"
        VERBATIM)
else()
    message(STATUS "clang-format not found; format targets disabled")
endif()

# CMake format and lint (cmakelang: cmake-format, cmake-lint)
find_program(
    CMAKE_FORMAT_EXE
    NAMES cmake-format
    PATHS "${CMAKE_SOURCE_DIR}/.venv/bin"
    NO_DEFAULT_PATH)
if(NOT CMAKE_FORMAT_EXE)
    find_program(CMAKE_FORMAT_EXE NAMES cmake-format)
endif()
find_program(
    CMAKE_LINT_EXE
    NAMES cmake-lint
    PATHS "${CMAKE_SOURCE_DIR}/.venv/bin"
    NO_DEFAULT_PATH)
if(NOT CMAKE_LINT_EXE)
    find_program(CMAKE_LINT_EXE NAMES cmake-lint)
endif()
if(CMAKE_FORMAT_EXE)
    file(
        GLOB_RECURSE
        CMAKE_SOURCES
        "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
        "${CMAKE_SOURCE_DIR}/src/CMakeLists.txt"
        "${CMAKE_SOURCE_DIR}/cmake/*.cmake"
        "${CMAKE_SOURCE_DIR}/cmake/*/*.cmake"
        "${CMAKE_SOURCE_DIR}/cpp-lkm/CMakeLists.txt"
        "${CMAKE_SOURCE_DIR}/cpp-lkm/cmake/*.cmake")
    add_custom_target(
        cmake-format
        COMMAND "${CMAKE_FORMAT_EXE}" -i ${CMAKE_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running cmake-format"
        VERBATIM)
    add_custom_target(
        cmake-format-check
        COMMAND "${CMAKE_FORMAT_EXE}" --check ${CMAKE_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Checking cmake-format"
        VERBATIM)
endif()
if(CMAKE_LINT_EXE)
    if(NOT CMAKE_FORMAT_EXE)
        file(
            GLOB_RECURSE
            CMAKE_SOURCES
            "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
            "${CMAKE_SOURCE_DIR}/src/CMakeLists.txt"
            "${CMAKE_SOURCE_DIR}/cmake/*.cmake"
            "${CMAKE_SOURCE_DIR}/cmake/*/*.cmake"
            "${CMAKE_SOURCE_DIR}/cpp-lkm/CMakeLists.txt"
            "${CMAKE_SOURCE_DIR}/cpp-lkm/cmake/*.cmake")
    endif()
    add_custom_target(
        cmake-lint
        COMMAND "${CMAKE_LINT_EXE}" ${CMAKE_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running cmake-lint"
        VERBATIM)
endif()
if(NOT CMAKE_FORMAT_EXE)
    message(STATUS "cmake-format not found; cmake-format targets disabled")
endif()
if(NOT CMAKE_LINT_EXE)
    message(STATUS "cmake-lint not found; cmake-lint target disabled")
endif()
