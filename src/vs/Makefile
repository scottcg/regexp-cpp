 # Compiler and flags
CXX = clang++
CXXFLAGS = -I. -std=c++20 -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-const-variable
CXXFLAGS +=  -DDEBUG -g

# Source files
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)

# Header files
HEADERS = source.h ctext.h rcimpl.h

# Executable name
TARGET = main.bin

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
