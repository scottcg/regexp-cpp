#include <iostream>
#include <cassert>
#include "precedence.h"

void test_precedence_vec() {
    // Create an instance of re_precedence_vec with default initialization
    precedence_vector<int, NUM_LEVELS> vec;

    // Check the initial values
    std::cout << "Initial values: ";
    for (int i = 0; i < NUM_LEVELS; ++i) {
        std::cout << vec[i] << " ";
        assert(vec[i] == 0);
    }
    std::cout << std::endl;

    // Modify the values
    for (int i = 0; i < NUM_LEVELS; ++i) {
        vec[i] = i * 10;
    }

    // Check the modified values
    std::cout << "Modified values: ";
    for (int i = 0; i < NUM_LEVELS; ++i) {
        std::cout << vec[i] << " ";
        assert(vec[i] == i * 10);
    }
    std::cout << std::endl;
}


void test_precedence_stack() {
    // Create an instance of re_precedence_stack
    precedence_stack stack;

    // Check the initial current precedence value
    std::cout << "Initial current precedence: " << stack.current() << std::endl;
    assert(stack.current() == 0);

    // Check the initial start value
    std::cout << "Initial start value: " << stack.start() << std::endl;
    assert(stack.start() == 0);

    // Modify the current precedence value
    stack.current(2);
    std::cout << "Modified current precedence: " << stack.current() << std::endl;
    assert(stack.current() == 2);

    // Modify the start value
    stack.start(10);
    std::cout << "Modified start value: " << stack.start() << std::endl;
    assert(stack.start() == 10);

    // Push a new element onto the stack
    stack.push(precedence_element());
    std::cout << "Pushed new element onto the stack." << std::endl;

    // Check the current precedence value after push
    std::cout << "Current precedence after push: " << stack.current() << std::endl;
    assert(stack.current() == 2);

    // Check the start value after push
    std::cout << "Start value after push: " << stack.start() << std::endl;
    assert(stack.start() == 0);

    // Pop the element from the stack
    stack.pop();
    std::cout << "Popped element from the stack." << std::endl;

    // Check the current precedence value after pop
    std::cout << "Current precedence after pop: " << stack.current() << std::endl;
    assert(stack.current() == 2);

    // Check the start value after pop
    std::cout << "Start value after pop: " << stack.start() << std::endl;
    assert(stack.start() == 10);
}

int main() {
  test_precedence_vec();
  test_precedence_stack();
  return 0;
}