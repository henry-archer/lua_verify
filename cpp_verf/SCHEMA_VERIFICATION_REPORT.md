# C++17 JSON Schema Verification Engine & Feature Report

## Executive Overview
A full-featured, zero-dependency **JSON Schema Verification System** targeting **C++17** has been created in [`/home/hen/lab/cpp_verf`](file:///home/hen/lab/cpp_verf). 

Because C++17 does not feature built-in reflection, the verifier uses a type-safe **Algebraic Data Type AST (`JsonValue`)** coupled with a **Fluent Builder DSL (`SchemaBuilder`)** and a **Declarative JSON Schema Parser (`SchemaParser`)**.

> **C++ Standard**: Targeted strictly to C++17 (`-std=c++17`). Uses modern standard library components (`std::variant`, `std::optional`, `std::string_view`, structured bindings, `std::initializer_list`, `<regex>`).

---

## Key Features & Capabilities

### 1. Dual Schema Definition Modes
1. **Declarative Mode (JSON Schema Files)**: Parse schema definition files conforming to standard JSON Schema specifications.
2. **Programmatic Mode (`SchemaBuilder`)**: Define schemas directly in C++ code using fluent method chaining without requiring reflection or preprocessors.

### 2. Supported JSON Schema Keywords

| Category | Keywords | Details |
|---|---|---|
| **Type Checking** | `type` | Supports `string`, `number`, `integer`, `boolean`, `array`, `object`, `null` (single type or type arrays) |
| **Numeric Constraints** | `minimum`, `maximum`, `exclusiveMinimum`, `exclusiveMaximum`, `multipleOf` | Floating point & integer range validation, strict inequalities, divisibility |
| **String Constraints** | `minLength`, `maxLength`, `pattern`, `format` | Character limits, regex pattern matching, and formats (`email`, `ipv4`, `date-time`, `uuid`, `uri`) |
| **Array Constraints** | `minItems`, `maxItems`, `uniqueItems`, `items`, `prefixItems` | Size constraints, duplicate detection, element schema, tuple schemas |
| **Object Constraints** | `properties`, `required`, `additionalProperties`, `patternProperties`, `minProperties`, `maxProperties`, `dependentRequired` | Key validation, mandatory properties, disallowing extra keys, regex property rules, property dependencies |
| **Enumerations & Values** | `enum`, `const` | Set membership and exact value equality |
| **Logical Combinators** | `allOf`, `anyOf`, `oneOf`, `not` | Complex schema compositions |
| **Custom Callbacks** | `custom(fn)` | C++17 lambda hooks `std::function<std::optional<ValidationError>(...)>` for custom business logic |

---

## Code Structure & Key Files

- [`include/json.hpp`](file:///home/hen/lab/cpp_verf/include/json.hpp) & [`src/json.cpp`](file:///home/hen/lab/cpp_verf/src/json.cpp): C++17 JSON AST (`JsonValue`) and fast recursive-descent parser.
- [`include/schema.hpp`](file:///home/hen/lab/cpp_verf/include/schema.hpp) & [`src/schema.cpp`](file:///home/hen/lab/cpp_verf/src/schema.cpp): Core Schema validator and error reporting model.
- [`include/builder.hpp`](file:///home/hen/lab/cpp_verf/include/builder.hpp): Modern C++17 `SchemaBuilder` fluent API.
- [`include/schema_parser.hpp`](file:///home/hen/lab/cpp_verf/include/schema_parser.hpp) & [`src/schema_parser.cpp`](file:///home/hen/lab/cpp_verf/src/schema_parser.cpp): Loader for JSON Schema definition files.
- [`main.cpp`](file:///home/hen/lab/cpp_verf/main.cpp): CLI program `json_verifier` with colorized output and exit codes.
- [`tests/test_main.cpp`](file:///home/hen/lab/cpp_verf/tests/test_main.cpp): Complete unit test suite.
- [`README.md`](file:///home/hen/lab/cpp_verf/README.md) & [`REPORT.md`](file:///home/hen/lab/cpp_verf/REPORT.md): System documentation and architectural details.

---

## Example CLI Usage

```bash
# Compile everything
make

# Run all unit tests
make test

# Interactive demo
./json_verifier --demo

# Validate JSON files
./json_verifier --schema examples/user_schema.json --input examples/valid_user.json
./json_verifier --schema examples/user_schema.json --input examples/invalid_user.json
```

### Sample Output on Invalid Input
```text
INVALID: JSON input 'examples/invalid_user.json' violates schema 'examples/user_schema.json' with 8 error(s):
  1. #/id (minimum): Value -5.000000 is less than minimum 1.000000
  2. #/username (minLength): String length 1 is shorter than minLength 3
  3. #/email (format): String 'not-an-email' is not a valid email address
  4. #/age (minimum): Value 16.000000 is less than minimum 18.000000
  5. #/role (enum): Value is not one of the allowed enum values: ["admin", "user", "guest", "moderator"]
  6. #/tags (uniqueItems): Array items at indices 0 and 1 are duplicate
  7. #/address/zipcode (pattern): String does not match regex pattern '^\d{5}$'
  8. # (additionalProperties): Property 'unauthorized_field' is not allowed by schema
```
