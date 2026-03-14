separate_arguments(CXX_FLAGS_LIST unix_command ${CXX_FLAGS})
execute_process(
    COMMAND ${COMPILER} ${CXX_FLAGS_LIST} -c ${SOURCE} -o /dev/null
    RESULT_VARIABLE RES
    OUTPUT_VARIABLE OUT
    ERROR_VARIABLE ERR
)
if(RES EQUAL 0)
    message(
        FATAL_ERROR
            "Negative compile test failed (it compiled successfully but was expected to fail):\n${OUT}\n${ERR}"
    )
else()
    message(STATUS "Negative compile test passed (it failed to compile as expected)")
endif()
