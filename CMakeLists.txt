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

# One test executable per tests/test_*.cpp (and legacy src/test_*.cpp).
# Tests run from the repo root so they can read data/questions.txt.
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/tests/test_*.cpp
     ${CMAKE_SOURCE_DIR}/src/test_*.cpp)
foreach(test_src ${TEST_SOURCES})
    # Prefix with the folder (tests_/src_) so the same file name in both places can't clash.
    get_filename_component(test_dir ${test_src} DIRECTORY)
    get_filename_component(test_dir ${test_dir} NAME)
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test_name ${test_dir}_${test_name})
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE quiz_modules)
    add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endforeach()
