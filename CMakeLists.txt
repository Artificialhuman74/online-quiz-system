cmake_minimum_required(VERSION 3.16)
project(OnlineQuizSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Every module in src/ except main.cpp and any self-test file (test_*.cpp).
file(GLOB MODULE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(FILTER MODULE_SOURCES EXCLUDE REGEX "/(main|test_[^/]*)\\.cpp$")

if(MODULE_SOURCES)
    add_library(quiz_modules STATIC ${MODULE_SOURCES})
    target_include_directories(quiz_modules PUBLIC ${CMAKE_SOURCE_DIR}/src)
else()
    add_library(quiz_modules INTERFACE)
    target_include_directories(quiz_modules INTERFACE ${CMAKE_SOURCE_DIR}/src)
endif()

add_executable(quiz src/main.cpp)
target_link_libraries(quiz PRIVATE quiz_modules)

# ---------------------------------------------------------------------------
# Tests (see tests/README.md). Every test runs from the repo root so it can read data/.
#   tests/unit/test_*.cpp       one module on its own              -> label "unit"
#   tests/component/test_*.cpp  several modules working together   -> label "component"
#   tests/system/<name>.in      runs the real quiz program with    -> label "system"
#                               <name>.in as keyboard input and checks <name>.expect
# tests/test_*.cpp and src/test_*.cpp are also picked up and labelled "unit".
# New files are found automatically; no need to edit this file.
# ---------------------------------------------------------------------------
enable_testing()

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/test_*.cpp)
file(GLOB LEGACY_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/test_*.cpp)
foreach(test_src ${TEST_SOURCES} ${LEGACY_TEST_SOURCES})
    # Prefix with the folder (unit_, component_, tests_, src_) so equal file names can't clash.
    get_filename_component(test_dir ${test_src} DIRECTORY)
    get_filename_component(test_dir ${test_dir} NAME)
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test_name ${test_dir}_${test_name})
    if(test_dir STREQUAL "tests" OR test_dir STREQUAL "src")
        set(test_label unit)
    else()
        set(test_label ${test_dir})
    endif()
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE quiz_modules)
    add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set_tests_properties(${test_name} PROPERTIES LABELS ${test_label} TIMEOUT 60)
endforeach()

file(GLOB SYSTEM_TEST_INPUTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/system/*.in)
foreach(input ${SYSTEM_TEST_INPUTS})
    get_filename_component(name ${input} NAME_WE)
    get_filename_component(dir ${input} DIRECTORY)
    add_test(NAME system_${name}
             COMMAND ${CMAKE_COMMAND}
                     -DQUIZ=$<TARGET_FILE:quiz>
                     -DINPUT=${input}
                     -DEXPECT=${dir}/${name}.expect
                     -P ${CMAKE_SOURCE_DIR}/tests/system/run_system_test.cmake
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set_tests_properties(system_${name} PROPERTIES LABELS system TIMEOUT 60)
endforeach()
