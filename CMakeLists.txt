cmake_minimum_required(VERSION 3.7)
project(mtp_lab1)

set(CMAKE_CXX_STANDARD 11)

set(PARSER_FILES src/cli-args/Parser.h src/cli-args/Arguments.h src/cli-args/Parser.cpp)

set(CLI_ARG_DEMO_FILES src/cli-args/demo.cpp ${PARSER_FILES})
add_executable(cli-arg-demo ${CLI_ARG_DEMO_FILES})

set(MT_FILES src/mt/Task.h src/mt/TaskGraph.h src/mt/TaskGraph.cpp)

set(LAB1_FILES src/lab1/main.cpp ${MT_FILES} ${PARSER_FILES})
add_executable(lab1 ${LAB1_FILES})
