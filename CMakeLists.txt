cmake_minimum_required(VERSION 3.0)
project(goo)

set(CMAKE_C_COMPILER gcc)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -Werror")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -std=c99 -Wall -Wextra ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32 -o goo ")

set(SOURCE_FILES
        goo.c)
add_executable(goo ${SOURCE_FILES})
