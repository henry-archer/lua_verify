# Modern C++17 JSON Schema Verifier

A standalone, header-light **JSON Schema Verification Engine** written in standard **C++17** without external dependencies or compiler reflection.

## Key Features

- **Dual Definition Modes**:
  1. **Declarative JSON Schema**: Parse and validate standard JSON Schema definition files.
  2. **Programmatic C++17 Builder API**: Fluent, type-safe C++ API (`SchemaBuilder`) allowing developers to construct schemas directly in code without macros or reflection.
- **Comprehensive Keyword & Constraint Support**:
  - **Type constraints**: `string`, `number`, `integer`, `boolean`, `array`, `object`, `null`
  - **Numeric constraints**: `minimum`, `maximum`, `exclusiveMinimum`, `exclusiveMaximum`, `multipleOf`
  - **String constraints**: `minLength`, `maxLength`, `pattern` (regex), `format` (`email`, `ipv4`, `date-time`, `uuid`, `uri`)
  - **Array constraints**: `minItems`, `maxItems`, `uniqueItems`, `items` element schema, `prefixItems` (tuple validation)
  - **Object constraints**: `properties`, `required`, `additionalProperties` (boolean or schema), `patternProperties`, `minProperties`, `maxProperties`, `dependentRequired`
  - **Enumerations & Values**: `enum`, `const`
  - **Logical Combinators**: `allOf`, `anyOf`, `oneOf`, `not`
- **Extensible C++ Custom Callbacks**: Attach custom `std::function` lambda rules to schemas for complex domain-specific validation logic.
- **Rich Diagnostic Error Reporting**: Provides exact JSON Pointers (e.g. `#/address/zipcode`), schema violation keywords, expected vs. actual values.
- **CLI Utility**: Command-line tool `json_verifier` with colorized terminal output and proper exit codes for automated script / CI workflows.

---

## Build & Test Instructions

### Prerequisites
- C++17 compliant compiler (`g++` 7+, `clang++` 5+, or MSVC 2017+)
- `make`

### Building
```bash
make
```

### Running Unit Tests
```bash
make test
```

### Running Interactive Demo
```bash
./json_verifier --demo
```

### CLI Verification
```bash
./json_verifier --schema examples/user_schema.json --input examples/valid_user.json
./json_verifier --schema examples/user_schema.json --input examples/invalid_user.json
```

---

## Programmatic C++17 Example

```cpp
#include "json.hpp"
#include "schema.hpp"
#include "builder.hpp"

auto my_schema = schema::SchemaBuilder::object()
    .property("username", schema::SchemaBuilder::string().minLength(3).maxLength(20))
    .property("age", schema::SchemaBuilder::integer().minimum(18))
    .property("email", schema::SchemaBuilder::string().format("email"))
    .required({"username", "email", "age"})
    .additionalProperties(false)
    .build();

auto parsed = json::Parser::parse(json_string);
if (auto node = std::get_if<json::JsonValue>(&parsed)) {
    auto result = my_schema->validate(*node);
    if (result.is_valid()) {
        std::cout << "JSON is valid!\n";
    } else {
        for (const auto& err : result.errors()) {
            std::cout << err.to_string() << "\n";
        }
    }
}
```
