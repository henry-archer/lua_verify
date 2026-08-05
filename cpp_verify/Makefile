CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -Iinclude

all: test_main demo

test_main: tests/test_main.cpp include/schema_verify/*.hpp
	$(CXX) $(CXXFLAGS) tests/test_main.cpp -o test_main

demo: examples/demo.cpp include/schema_verify/*.hpp
	$(CXX) $(CXXFLAGS) examples/demo.cpp -o demo

run_tests: test_main
	./test_main

run_demo: demo
	./demo

clean:
	rm -f test_main demo

.PHONY: all run_tests run_demo clean
