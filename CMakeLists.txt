cmake_minimum_required(VERSION 3.7)
project(mtp_lab1)

set(CMAKE_CXX_STANDARD 11)

set(PARSER_FILES src/cli-args/Parser.h src/cli-args/Arguments.h src/cli-args/Parser.cpp)

set(CLI_ARG_DEMO_FILES src/cli-args/demo.cpp ${PARSER_FILES})
add_executable(cli-arg-demo ${CLI_ARG_DEMO_FILES})

set(MT_FILES src/mt/Task.h src/mt/TaskGraph.h src/mt/TaskGraph.cpp)

set(LAB1_FILES
        src/lab1/main.cpp src/lab1/tasks.h
        ${MT_FILES} ${PARSER_FILES})
add_executable(lab1 ${LAB1_FILES})

set(LAB1_V2_FILES
        src/lab1_v2/main.cpp src/lab1_v2/tasks.h
        ${MT_FILES} ${PARSER_FILES})
add_executable(lab1_v2 ${LAB1_V2_FILES})

set(LAB2_FILES
        src/lab2/main.cpp src/lab2/tasks.h
        src/lab2/MatrixBuffer.h src/lab2/MatrixBuffer.cpp
        src/lab2/strassen.h src/lab2/strassen.cpp
        ${MT_FILES} ${PARSER_FILES})
add_executable(lab2 ${LAB2_FILES})
