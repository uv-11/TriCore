# CommunityBridge Phase 3: backend, tests and the Windows GUI on top of the Phase 2 sources.
# Phase 2 is read-only: its sources are compiled from ../Phase2/src and never copied or changed.
cmake_minimum_required(VERSION 3.16)
project(communitybridge LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra>")

# --- phase2core: the Phase 2 production sources without the console view and its main ---
file(GLOB PHASE2_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../Phase2/src/*.cpp")
list(FILTER PHASE2_SOURCES EXCLUDE REGEX "/(ConsoleView|main)\\.cpp$")
message(STATUS "phase2core sources:")
foreach(source ${PHASE2_SOURCES})
    message(STATUS "  ${source}")
endforeach()

add_library(phase2core STATIC ${PHASE2_SOURCES})
# the include folders are for the C++ compiler only: the resource compiler cannot handle a space in a path
target_include_directories(phase2core PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/../Phase2/include>")

# --- phase3core: the plain C++ backend (no windows.h) ---
file(GLOB PHASE3_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(FILTER PHASE3_SOURCES EXCLUDE REGEX "/(Gui[^/]*|Page[^/]*|Theme|gui_main|AppState)\\.cpp$")
message(STATUS "phase3core sources:")
foreach(source ${PHASE3_SOURCES})
    message(STATUS "  ${source}")
endforeach()

if(PHASE3_SOURCES)
    add_library(phase3core STATIC ${PHASE3_SOURCES})
    target_include_directories(phase3core PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include>")
    target_link_libraries(phase3core PUBLIC phase2core)
else()
    # nothing to compile yet, the name still exists so the tests can link against it
    add_library(phase3core INTERFACE)
    target_include_directories(phase3core INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include>")
    target_link_libraries(phase3core INTERFACE phase2core)
endif()

# --- the Windows program: Gui*.cpp, Page*.cpp, Theme.cpp, gui_main.cpp and AppState.cpp (the ones phase3core leaves out) ---
file(GLOB GUI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Gui*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/Page*.cpp")
set(GUI_SOURCES ${GUI_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Theme.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/gui_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/AppState.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/res/gui.rc")

# gui.rc names app.manifest without a folder, and the resource compiler runs in the build folder, so the file is copied there
# (a copy is made again whenever the original changes)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/app.manifest" "${CMAKE_CURRENT_BINARY_DIR}/app.manifest" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/res/app.ico" "${CMAKE_CURRENT_BINARY_DIR}/app.ico" COPYONLY)
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/res/gui.rc" PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/app.manifest;${CMAKE_CURRENT_BINARY_DIR}/app.ico")

add_executable(communitybridge_gui WIN32 ${GUI_SOURCES})
target_include_directories(communitybridge_gui PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/include>")
# the old Windows headers show the list view declarations only for Internet Explorer 5 and Windows XP or newer
target_compile_definitions(communitybridge_gui PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:_WIN32_IE=0x0500;WINVER=0x0501;_WIN32_WINNT=0x0501>")
target_link_libraries(communitybridge_gui PRIVATE phase3core phase2core user32 gdi32 comctl32 uxtheme shell32 ole32)
# static linking, so the program runs on a computer without the MinGW dll files
target_link_options(communitybridge_gui PRIVATE -static -static-libgcc -static-libstdc++)

# the release build (make_dist.bat) is optimised with -O2 and stripped of its symbols
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
target_link_options(communitybridge_gui PRIVATE "$<$<CONFIG:Release>:-s>")

# --- packaging: "cmake --install" makes the folder that starts by double click ---
# The six sample files come from ../Phase2/data (only read). The working copy is data/, made only when it is not there yet,
# so making the folder again never overwrites the changes a person made; sample_data/ always holds the original files.
set(SAMPLE_DATA_FILES donors.csv recipients.csv graph_nodes.csv graph_edges.csv donations.csv transactions.csv)
install(TARGETS communitybridge_gui RUNTIME DESTINATION .)

foreach(sample ${SAMPLE_DATA_FILES})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../Phase2/data/${sample}" DESTINATION sample_data)
endforeach()

install(CODE "
    foreach(sample ${SAMPLE_DATA_FILES})
        if(NOT EXISTS \"\${CMAKE_INSTALL_PREFIX}/data/\${sample}\")
            file(INSTALL \"${CMAKE_CURRENT_SOURCE_DIR}/../Phase2/data/\${sample}\" DESTINATION \"\${CMAKE_INSTALL_PREFIX}/data\")
        endif()
    endforeach()
")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/dist_reset_data.bat" DESTINATION . RENAME reset_data.bat)

# --- tests: each tests/<name>.cpp is its own program, registered with CTest ---
enable_testing()

# Adds one test program and passes the test data folder as a compile definition.
function(add_cb_test name)
    add_executable(${name} "${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}.cpp")
    target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_link_libraries(${name} PRIVATE phase3core phase2core)
    target_compile_definitions(${name} PRIVATE CB_TESTDATA="${CMAKE_BINARY_DIR}/testdata/")
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_cb_test(test_p2link)
add_cb_test(test_offer)
add_cb_test(test_offerstore)
add_cb_test(test_queries)
add_cb_test(test_tables)
add_cb_test(test_recipientupdate)
add_cb_test(test_workflow)
add_cb_test(test_dashboard)
add_cb_test(test_text)
add_cb_test(test_notes)
# --- the test modes of the Windows program, run on a copy of the sample data (see tests/RunGuiTest.cmake) ---
# Registers one test mode of the program with CTest.
function(add_gui_test name mode)
    add_test(NAME ${name}
             COMMAND ${CMAKE_COMMAND} "-DEXE=$<TARGET_FILE:communitybridge_gui>" "-DDATA=${CMAKE_CURRENT_SOURCE_DIR}/data"
                     "-DWORK=${CMAKE_BINARY_DIR}/testdata/${name}" "-DMODE=${mode}" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/RunGuiTest.cmake")
endfunction()

add_gui_test(gui_selftest selftest)
add_gui_test(gui_selftest_flow selftest-flow)

# Windows asks for administrator rights when a 32-bit program without a manifest has "update" in
# its file name, so the test keeps its name in CTest but the file gets another name.
set_target_properties(test_recipientupdate PROPERTIES OUTPUT_NAME test_recipient_edit)
