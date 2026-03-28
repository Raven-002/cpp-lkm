separate_arguments(CXX_FLAGS_LIST UNIX_COMMAND ${CXX_FLAGS})
separate_arguments(LINK_FLAGS_LIST UNIX_COMMAND ${LINK_FLAGS})
# -nodefaultlibs prevents the host libstdc++ from supplying operator new,
# which would mask the intended linker-trap failure. -lc and -lgcc provide
# only the minimal C runtime needed for a well-formed (but expected-to-fail) link.
execute_process(
    COMMAND
        ${COMPILER} ${CXX_FLAGS_LIST} ${SOURCE} ${LIB_TO_LINK} ${LINK_FLAGS_LIST}
        -nodefaultlibs -lc -lgcc -o /dev/null
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
