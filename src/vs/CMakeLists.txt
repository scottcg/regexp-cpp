cmake_minimum_required(VERSION 3.20)
project(regex-vs)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-const-variable -DDEBUG -g")

# Source files
set(MAIN_SRCS src/main.cpp)
set(DUMP_SRCS src/dump.cpp)

# Add include directories
include_directories(
        ${CMAKE_SOURCE_DIR}/src
        /opt/homebrew/Cellar/googletest/1.15.2/include
)

# Add executables
add_executable(main.bin ${MAIN_SRCS})
add_executable(dump.bin ${DUMP_SRCS})

# Find all test files
file(GLOB TEST_SRCS test/*.cpp)

# Add include directories for tests
foreach(TEST_SRC ${TEST_SRCS})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${TEST_NAME}
            /opt/homebrew/Cellar/googletest/1.15.2/lib/libgtest.a
            /opt/homebrew/Cellar/googletest/1.15.2/lib/libgtest_main.a
            pthread
    )
endforeach()

# Custom target for running tests
add_custom_target(run_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS ${TEST_SRCS}
)