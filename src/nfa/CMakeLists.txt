cmake_minimum_required(VERSION 3.30)
project(nfa)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(nfa
    code.h
    ctext.h
    instance.cpp
    main.cpp
    precedence.h
    rcimpl.h
    regexp.cpp
    regexp.h
    regexpimpl.h
    source.h
    syntax.cpp
    syntax.h)
