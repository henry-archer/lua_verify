CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
SRC = src/json.cpp src/schema.cpp src/schema_parser.cpp
OBJ = $(SRC:.cpp=.o)

TARGET = json_verifier
TEST_TARGET = run_tests
EXAMPLE_TARGET = programmatic_example

all: $(TARGET) $(TEST_TARGET) $(EXAMPLE_TARGET)

$(TARGET): main.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) main.cpp $(OBJ) -o $(TARGET)

$(TEST_TARGET): tests/test_main.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) tests/test_main.cpp $(OBJ) -o $(TEST_TARGET)

$(EXAMPLE_TARGET): examples/programmatic_example.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) examples/programmatic_example.cpp $(OBJ) -o $(EXAMPLE_TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

demo: $(TARGET)
	./$(TARGET) --demo

verify_examples: $(TARGET)
	@echo "=== Validating Valid Example ==="
	-./$(TARGET) --schema examples/user_schema.json --input examples/valid_user.json
	@echo "\n=== Validating Invalid Example ==="
	-./$(TARGET) --schema examples/user_schema.json --input examples/invalid_user.json

clean:
	rm -f src/*.o main.o $(TARGET) $(TEST_TARGET) $(EXAMPLE_TARGET)

.PHONY: all test demo verify_examples clean
