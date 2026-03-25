separate_arguments(CXX_FLAGS_LIST UNIX_COMMAND ${CXX_FLAGS})
separate_arguments(LINK_FLAGS_LIST UNIX_COMMAND ${LINK_FLAGS})
execute_process(
    COMMAND ${COMPILER} ${CXX_FLAGS_LIST} ${SOURCE} ${LIB_TO_LINK} ${LINK_FLAGS_LIST} -o /dev/null
    RESULT_VARIABLE RES
    OUTPUT_VARIABLE OUT
    ERROR_VARIABLE ERR
)
if(RES EQUAL 0)
    message(
        FATAL_ERROR
            "Negative link test failed (it linked successfully but was expected to fail):\n${OUT}\n${ERR}"
    )
else()
    message(STATUS "Negative link test passed (it failed to link as expected)")
endif()
