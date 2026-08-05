# Feature & Architecture Report: C++17 JSON Schema Verifier

## 1. Executive Summary

This project delivers a high-performance, standalone **JSON Schema Verification Engine and CLI Utility** written in **C++17**. Because C++17 does not provide built-in compile-time or runtime reflection, the system uses a **Type AST (Abstract Syntax Tree)** representation coupled with a **Fluent Builder Pattern** and **Polymorphic Rule Composite Architecture**.

The system enables users to:
1. **Declaratively** define schemas using standard JSON files or JSON strings.
2. **Programmatically** construct type-safe schemas in C++17 code via a macro-free builder interface.
3. **Extend** validation using custom C++ functional lambdas.
4. **Verify** JSON input files with precise, JSON-Pointer path error reporting.

---

## 2. Architecture & Design Patterns (C++17 Idioms)

### 2.1 Overcoming the Lack of Reflection in C++17
Without reflection (which remains absent prior to C++26), C++ cannot automatically inspect C++ struct field names or types at runtime. To solve this cleanly:
- **`json::JsonValue`**: Built around `std::variant<std::nullptr_t, bool, double, std::string, std::vector<JsonValue>, std::vector<std::pair<std::string, JsonValue>>>`. This algebraic data type represents dynamic JSON trees without void pointer casts or unsafe unions.
- **`schema::Schema` Composite & `SchemaBuilder`**: Encapsulates validation constraints as explicit data members and functional hooks.
- **Fluent Builder Interface**: Provides a natural, declarative-feeling C++ DSL using standard C++ syntax chaining.

### 2.2 Modern C++17 Language Features Employed
- **`std::variant` & `std::holds_alternative` / `std::get_if`**: Safe tagged unions for JSON AST nodes and parser returns.
- **`std::optional`**: Expressing optional schema constraints (`minimum`, `maxItems`, etc.) without sentinel values like `-1` or `std::numeric_limits`.
- **`std::string_view`**: Zero-allocation string slicing in the JSON parser.
- **Structured Bindings (`auto [key, val] : obj`)**: Clean iteration over object properties and pattern maps.
- **`std::initializer_list`**: Concise vector and map initialization in `SchemaBuilder`.
- **Standard `<regex>`**: High-performance regular expression matching for `pattern`, string formats, and `patternProperties`.

---

## 3. Comprehensive Feature & Keyword Support Matrix

| Keyword Category | Supported Keywords / Features | Description |
|---|---|---|
| **Types** | `type` (`string`, `number`, `integer`, `boolean`, `array`, `object`, `null`) | Strict or union type checking |
| **Numeric** | `minimum`, `maximum`, `exclusiveMinimum`, `exclusiveMaximum`, `multipleOf` | Inclusive/exclusive range checks and divisibility |
| **String** | `minLength`, `maxLength`, `pattern`, `format` | Character count bounds, regex matching, and standard formats (`email`, `ipv4`, `date-time`, `uuid`, `uri`) |
| **Array** | `minItems`, `maxItems`, `uniqueItems`, `items`, `prefixItems` | Item count limits, duplicate item detection, uniform item schemas, and tuple schemas |
| **Object** | `properties`, `required`, `additionalProperties`, `patternProperties`, `minProperties`, `maxProperties`, `dependentRequired` | Property validation, mandatory key enforcement, extra key disallowing, regex property rules, property count limits, and key dependency rules |
| **Values** | `enum`, `const` | Set membership and exact value matching |
| **Combinators** | `allOf`, `anyOf`, `oneOf`, `not` | Logical conjunction, disjunction, exclusive disjunction, and negation |
| **Custom** | `custom(fn)` | C++17 functional callbacks `std::function<std::optional<ValidationError>(...)>` for domain rules |

---

## 4. Error Diagnostics & Path Reporting

When a JSON document violates a schema, the verifier collects all error details without stopping at the first failure. Each error includes:
- **`path`**: Standard JSON Pointer (e.g. `#/address/zipcode` or `#/tags/2`) indicating the exact position of the failure.
- **`keyword`**: The violated JSON Schema rule (e.g. `pattern`, `required`, `minimum`).
- **`message`**: Human-readable diagnosis explaining why the value failed validation.

---

## 5. Verification & Test Suite Summary

The included test suite in [`tests/test_main.cpp`](file:///home/hen/lab/cpp_verf/tests/test_main.cpp) covers:
1. **JSON Parser Unit Tests**: Verifies numbers, strings with escape sequences (`\n`, `\t`, `\"`, `\uXXXX`), booleans, null, nested arrays, and objects.
2. **Type Validator Tests**: Tests primitive and complex type checks.
3. **Numeric Range & Divisibility Tests**: Tests inclusive/exclusive bounds and floating-point `multipleOf`.
4. **String & Regex Tests**: Validates string lengths, regex patterns, and string format validators (`email`, `ipv4`, `uuid`).
5. **Array Constraint Tests**: Verifies `minItems`, `maxItems`, duplicate item detection (`uniqueItems`), and element schemas.
6. **Object Property Tests**: Verifies required keys, disallowing unexpected keys (`additionalProperties: false`), regex `patternProperties`, and `dependentRequired`.
7. **Enum & Const Tests**: Validates allowable string/number enumerations and constant values.
8. **Logical Combinator Tests**: Tests `allOf`, `anyOf`, `oneOf`, `not` logic.
9. **Declarative Schema Parser Tests**: Tests parsing a full JSON Schema document and validating sample payloads against it.
10. **C++17 Custom Validation Callback Tests**: Verifies user-defined lambda validation functions.

---

## 6. Project Structure

```
cpp_verf/
├── include/
│   ├── json.hpp             # JSON AST, JsonValue, Parser interfaces
│   ├── schema.hpp           # Schema validator & ValidationError models
│   ├── builder.hpp          # C++17 Fluent SchemaBuilder API
│   └── schema_parser.hpp    # Declarative JSON Schema file parser
├── src/
│   ├── json.cpp             # Recursive-descent JSON parser implementation
│   ├── schema.cpp           # Keyword validation logic & format algorithms
│   └── schema_parser.cpp    # JSON Schema loader implementation
├── tests/
│   └── test_main.cpp        # Complete unit test suite (11 test suites)
├── examples/
│   ├── user_schema.json     # Example declarative schema
│   ├── valid_user.json      # Valid test payload
│   ├── invalid_user.json    # Payload triggering multiple schema errors
│   └── programmatic_example.cpp # C++ programmatic usage example
├── main.cpp                 # CLI application entrypoint (json_verifier)
├── Makefile                 # Build system configuration
├── README.md                # Usage guide & API reference
└── REPORT.md                # Architecture & feature report
```
