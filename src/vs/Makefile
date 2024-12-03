# Compiler and flags
CXX = clang++
CXXFLAGS = -I. -std=c++20 -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-const-variable
CXXFLAGS +=  -DDEBUG -g

# Source files
SRCS = main.cpp dump.cpp
OBJS = $(SRCS:.cpp=.o)

# Header files
HEADERS = $(wildcard *.h)

# Executable names
MAIN_TARGET = main.bin
DUMP_TARGET = dump.bin

# Default target
all: $(MAIN_TARGET) $(DUMP_TARGET)

# Link the main executable
$(MAIN_TARGET): main.o
	$(CXX) $(CXXFLAGS) -o $(MAIN_TARGET) main.o

# Link the dump executable
$(DUMP_TARGET): dump.o
	$(CXX) $(CXXFLAGS) -o $(DUMP_TARGET) dump.o

# Compile source files into object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the main executable
exec_main: $(MAIN_TARGET)
	./$(MAIN_TARGET)

# Run the dump executable
exec_dump: $(DUMP_TARGET)
	./$(DUMP_TARGET)

# Clean up build files
clean:
	rm -f $(OBJS) $(MAIN_TARGET) $(DUMP_TARGET)

# Phony targets
.PHONY: all clean exec_main exec_dump