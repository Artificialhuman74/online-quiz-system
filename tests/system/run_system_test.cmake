# Runs the quiz program with a scripted keyboard input and checks its output.
# Called by CTest (see CMakeLists.txt) as:
#   cmake -DQUIZ=<quiz exe> -DINPUT=<name>.in -DEXPECT=<name>.expect -P run_system_test.cmake
#
# <name>.expect: each non-empty line not starting with '#' must appear in the program's
# output, in that order. The program must exit with code 0. Avoid ';', '[' and ']' in
# expect lines: CMake list handling splits or mangles them.

foreach(var QUIZ INPUT EXPECT)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "run_system_test.cmake: ${var} is not set")
    endif()
endforeach()
if(NOT EXISTS "${EXPECT}")
    message(FATAL_ERROR "Missing expectations file: ${EXPECT}")
endif()

execute_process(
    COMMAND "${QUIZ}"
    INPUT_FILE "${INPUT}"
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    RESULT_VARIABLE rc
    TIMEOUT 30)

if(NOT rc STREQUAL "0")
    message(FATAL_ERROR "quiz exited with '${rc}' (expected 0)\n--- stdout ---\n${out}\n--- stderr ---\n${err}")
endif()

file(STRINGS "${EXPECT}" expected_lines)
set(rest "${out}")
foreach(line IN LISTS expected_lines)
    if(line STREQUAL "" OR line MATCHES "^#")
        continue()
    endif()
    string(FIND "${rest}" "${line}" pos)
    if(pos EQUAL -1)
        message(FATAL_ERROR "Expected output not found (in order): '${line}'\n--- stdout ---\n${out}")
    endif()
    string(LENGTH "${line}" len)
    math(EXPR pos "${pos} + ${len}")
    string(SUBSTRING "${rest}" ${pos} -1 rest)
endforeach()

message(STATUS "System test passed: ${INPUT}")
