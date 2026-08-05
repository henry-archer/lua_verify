# C++17 Zero-Dependency Schema Validator & Documentation Generator (`schema_verify`)

A modern, header-only C++17 schema validation and document generation library built with **zero external dependencies** (standard C++ library only).

## Key Features

1. **Zero External Dependencies**: Pure C++17 header-only library (`<variant>`, `<map>`, `<vector>`, `<string>`, `<optional>`, `<memory>`).
2. **Generic Structure Agnostic**: Validates dynamic standard structures (`schema_verify::Value`) created from JSON, YAML, Lua, TOML, or constructed in memory.
3. **Strict & Flexible Type Checking**: Supports `Null`, `Bool`, `Int`, `Float`, `Number` (int or float), `String`, `Array`, `Object`, and `Any`.
4. **List Element Validation**: Validates homogeneity or complex nested element schemas in arrays, tracking exact array index paths (e.g. `routes[1].method`).
5. **Optional Parent with Required Children**: Handles optional parent objects seamlessly (if the parent key is omitted, validation succeeds; if present, all required children inside are validated).
6. **Levenshtein Distance Key Suggestions**: Automatically suggests the closest valid property key when an unrecognised/misspelled key is found in an object (e.g. `usr_name` -> `"Did you mean 'username'?"`).
7. **Comprehensive Error Collector**: Gathers **all** validation errors in a single pass rather than failing on the first error.
8. **Schema Documentation Generator**: Automatically exports schema definitions into clean GitHub-flavored Markdown tables or structured Plain Text documentation.

---

## Directory Structure

```text
/home/hen/lab/cpp_verify/
├── include/
│   └── schema_verify/
│       ├── schema_verify.hpp  # Main umbrella header
│       ├── value.hpp          # Dynamic Value tree representation
│       ├── levenshtein.hpp    # Levenshtein distance & key suggestion logic
│       ├── error.hpp          # ValidationError & ValidationResult error collector
│       ├── schema.hpp         # Schema definition & validation engine
│       └── doc_generator.hpp  # Markdown and Text document generator
├── tests/
│   └── test_main.cpp          # Comprehensive unit test suite
├── examples/
│   └── demo.cpp               # Executable real-world example
├── Makefile                   # Build configuration (-std=c++17 -Wall -Wextra -Werror)
└── README.md
```

---

## Quick Start Example

```cpp
#include <iostream>
#include <schema_verify/schema_verify.hpp>

using namespace schema_verify;

int main() {
    // 1. Define nested schema
    auto db_schema = Schema::object({
        {"host", Property::required(Schema::string().description("Database host"))},
        {"port", Property::required(Schema::integer().range(1, 65535).description("Database port"))}
    });

    auto app_schema = Schema::object({
        {"app_name", Property::required(Schema::string().min_len(1))},
        {"database", Property::optional(db_schema)} // Optional parent with required children
    });

    // 2. Build or parse input data into Value
    Value input = Value::object({
        {"app_name", "MyApp"},
        {"database", Value::object({
            {"hst", "127.0.0.1"}, // Misspelled key
            {"port", 70000}       // Out of range port
        })}
    });

    // 3. Validate input
    ValidationResult result = app_schema.validate(input);
    if (!result.is_valid()) {
        std::cout << result.format() << "\n";
    }

    // 4. Generate Markdown Documentation
    std::cout << DocGenerator::generate_markdown(app_schema, "App Config Schema") << "\n";

    return 0;
}
```

---

## Building and Running Tests

```bash
make run_tests
make run_demo
```
