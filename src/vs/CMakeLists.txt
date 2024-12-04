cmake_minimum_required(VERSION 3.20)
project(MyProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-const-variable -DDEBUG -g")

# Source files
set(SRCS main.cpp dump.cpp)

# Add executables
add_executable(main.bin main.cpp)
add_executable(dump.bin dump.cpp)

# Include directories
include_directories(${CMAKE_SOURCE_DIR})

# Link libraries (if any)
# target_link_libraries(main.bin <library>)
# target_link_libraries(dump.bin <library>)

# Custom targets for running executables
add_custom_target(exec_main
    COMMAND main.bin
    DEPENDS main.bin
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

add_custom_target(exec_dump
    COMMAND dump.bin
    DEPENDS dump.bin
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

# Clean up build files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "main.bin;dump.bin")
