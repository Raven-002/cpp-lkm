if(NOT DEFINED SOURCE_DIR OR "${SOURCE_DIR}" STREQUAL "")
    message(FATAL_ERROR "SOURCE_DIR must be provided to CheckNoBug.cmake")
endif()

file(GLOB_RECURSE CPP_LKM_SOURCE_FILES "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.h"
     "${SOURCE_DIR}/*.hpp"
)

set(CPP_LKM_BUG_MATCHES)
foreach(CPP_LKM_FILE IN LISTS CPP_LKM_SOURCE_FILES)
    file(STRINGS "${CPP_LKM_FILE}" CPP_LKM_BUG_LINES REGEX "BUG\\(")
    if(CPP_LKM_BUG_LINES)
        list(APPEND CPP_LKM_BUG_MATCHES "${CPP_LKM_FILE}")
    endif()
endforeach()

if(CPP_LKM_BUG_MATCHES)
    list(JOIN CPP_LKM_BUG_MATCHES "\n  - " CPP_LKM_BUG_MATCHES_TEXT)
    message(FATAL_ERROR "FAIL: BUG() found in module source:\n  - ${CPP_LKM_BUG_MATCHES_TEXT}")
endif()

message(STATUS "OK: no BUG() in module source")
