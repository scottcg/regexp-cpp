CXX = clang++
CXXFLAGS = -I. -std=c++20 -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-const-variable
# CXXFLAGS += -DDEBUG -g

TARGET = main.bin
SRC = main.cpp

all: build

build: $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

exec: clean build
	./$(TARGET)

clean:
	rm -f $(TARGET)