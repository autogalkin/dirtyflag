cmake_minimum_required(VERSION 3.16)

PROJECT(dirtyflag

        )

set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)


add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(testdirtyflag ${CMAKE_CURRENT_SOURCE_DIR}/tests/testdirtyflag/main.cpp)
target_include_directories(testdirtyflag PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_hardlink "${CMAKE_BINARY_DIR}/compile_commands.json" "${CMAKE_SOURCE_DIR}/compile_commands.json"
     COMMAND_ECHO STDOUT
    )