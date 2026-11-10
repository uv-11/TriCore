# RunGuiTest.cmake: runs one test mode of the Windows program on a COPY of the sample data.
# Used by CTest through "cmake -P". Parameters (-D): EXE, DATA, WORK, MODE.
# The test mode writes selftest.txt next to the exe; this script prints it and fails when the exit code is not 0.

if(NOT EXISTS "${EXE}")
    message(FATAL_ERROR "The program was not found: ${EXE}")
endif()

# a fresh copy of the six data files, so a test never touches Phase3/data
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
file(GLOB DATAFILES "${DATA}/*.csv")
file(COPY ${DATAFILES} DESTINATION "${WORK}")

get_filename_component(EXEDIR "${EXE}" DIRECTORY)
file(REMOVE "${EXEDIR}/selftest.txt")

if(MODE STREQUAL "screenshots")
    set(SHOTS "${WORK}/shots")
    execute_process(COMMAND "${EXE}" "${WORK}" "--screenshots" "${SHOTS}" RESULT_VARIABLE CODE)
else()
    execute_process(COMMAND "${EXE}" "${WORK}" "--${MODE}" RESULT_VARIABLE CODE)
endif()

if(EXISTS "${EXEDIR}/selftest.txt")
    file(READ "${EXEDIR}/selftest.txt" LOG)
    message("${LOG}")
else()
    message(FATAL_ERROR "The test mode wrote no selftest.txt")
endif()

if(NOT CODE EQUAL 0)
    message(FATAL_ERROR "The program ended with exit code ${CODE}")
endif()
