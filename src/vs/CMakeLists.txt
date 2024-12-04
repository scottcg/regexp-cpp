cmake_minimum_required(VERSION 3.20)
project(MyProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-const-variable -DDEBUG -g")

# Source files
set(MAIN_SRCS src/main.cpp)
set(DUMP_SRCS src/dump.cpp)
set(TESTS test/test_code_vector.cpp)

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    /opt/homebrew/Cellar/googletest/1.15.2/include
)

# Add executables
add_executable(main.bin ${MAIN_SRCS})
add_executable(dump.bin ${DUMP_SRCS})
add_executable(test_code_vector ${TESTS})

# Link Google Test manually
target_link_libraries(test_code_vector
    /opt/homebrew/Cellar/googletest/1.15.2/lib/libgtest.a
    /opt/homebrew/Cellar/googletest/1.15.2/lib/libgtest_main.a
    pthread
)

# Custom target for running tests
add_custom_target(run_tests
    COMMAND ${CMAKE_BINARY_DIR}/test_code_vector
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
