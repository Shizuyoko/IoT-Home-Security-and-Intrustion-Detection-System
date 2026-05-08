cmake_minimum_required(VERSION 3.16)
project(IOTBackendTest)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-D_WIN32_WINNT=0x0A00)

add_executable(IOTBackendTest
        src/main.cpp
        src/httplib.h
        src/json.hpp
        src/sqlite3.c
        src/sqlite3.h
        src/base64.h
)

target_link_libraries(IOTBackendTest ws2_32)
