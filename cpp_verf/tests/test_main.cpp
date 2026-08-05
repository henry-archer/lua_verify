#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "json.hpp"
#include "schema.hpp"
#include "builder.hpp"
#include "schema_parser.hpp"

// Test harness macro
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "\033[31m[FAILED]\033[0m " << __func__ << ":" << __LINE__ << " - " << msg << std::endl; \
            std::exit(1); \
        } \
    } while(0)

int g_tests_passed = 0;

void RUN_TEST(void(*func)(), const std::string& name) {
    std::cout << "[RUNNING] " << name << "... ";
    func();
    std::cout << "\033[32m[PASSED]\033[0m" << std::endl;
    g_tests_passed++;
}

void test_json_parser_primitives() {
    auto res1 = json::Parser::parse("123.45");
    TEST_ASSERT(std::holds_alternative<json::JsonValue>(res1), "Failed to parse number");
    TEST_ASSERT(std::get<json::JsonValue>(res1).as_number() == 123.45, "Number value mismatch");

    auto res2 = json::Parser::parse("\"hello \\\"world\\\"\\n\"");
    TEST_ASSERT(std::holds_alternative<json::JsonValue>(res2), "Failed to parse string");
    TEST_ASSERT(std::get<json::JsonValue>(res2).as_string() == "hello \"world\"\n", "String value mismatch");

    auto res3 = json::Parser::parse("true");
    TEST_ASSERT(std::get<json::JsonValue>(res3).as_bool() == true, "Bool mismatch");

    auto res4 = json::Parser::parse("null");
    TEST_ASSERT(std::get<json::JsonValue>(res4).is_null(), "Null mismatch");
}

void test_json_parser_complex() {
    std::string src = R"({
        "number": 42,
        "arr": [1, 2, "three"],
        "nested": { "key": true }
    })";
    auto res = json::Parser::parse(src);
    TEST_ASSERT(std::holds_alternative<json::JsonValue>(res), "Failed complex json parse");
    auto val = std::get<json::JsonValue>(res);
    TEST_ASSERT(val.is_object(), "Root is not object");
    TEST_ASSERT(val.get("number")->as_number() == 42, "Number field mismatch");
    TEST_ASSERT(val.get("arr")->as_array().size() == 3, "Array size mismatch");
    TEST_ASSERT(val.get("nested")->get("key")->as_bool() == true, "Nested object field mismatch");
}

void test_type_validation() {
    auto s_str = schema::SchemaBuilder::string().build();
    auto s_num = schema::SchemaBuilder::number().build();
    auto s_int = schema::SchemaBuilder::integer().build();

    TEST_ASSERT(s_str->validate(json::JsonValue("test")).is_valid(), "String type valid case failed");
    TEST_ASSERT(!s_str->validate(json::JsonValue(123)).is_valid(), "String type invalid case failed");

    TEST_ASSERT(s_num->validate(json::JsonValue(3.14)).is_valid(), "Number type valid double failed");
    TEST_ASSERT(s_num->validate(json::JsonValue(10)).is_valid(), "Number type valid int failed");

    TEST_ASSERT(s_int->validate(json::JsonValue(10)).is_valid(), "Integer valid int failed");
    TEST_ASSERT(!s_int->validate(json::JsonValue(10.5)).is_valid(), "Integer invalid float failed");
}

void test_number_validation() {
    auto s = schema::SchemaBuilder::number()
        .minimum(10.0)
        .maximum(20.0)
        .exclusiveMinimum(10.0)
        .multipleOf(2.5)
        .build();

    TEST_ASSERT(s->validate(json::JsonValue(15.0)).is_valid(), "Valid number failed");
    TEST_ASSERT(!s->validate(json::JsonValue(10.0)).is_valid(), "Exclusive minimum failed");
    TEST_ASSERT(!s->validate(json::JsonValue(25.0)).is_valid(), "Maximum failed");
    TEST_ASSERT(!s->validate(json::JsonValue(12.0)).is_valid(), "MultipleOf failed");
}

void test_string_validation() {
    auto s = schema::SchemaBuilder::string()
        .minLength(3)
        .maxLength(10)
        .pattern("^[a-z]+$")
        .format("email")
        .build();

    TEST_ASSERT(s->validate(json::JsonValue("abc@d.com")).is_valid() == false, "Pattern regex constraint check");

    auto s_email = schema::SchemaBuilder::string().format("email").build();
    TEST_ASSERT(s_email->validate(json::JsonValue("user@example.com")).is_valid(), "Valid email failed");
    TEST_ASSERT(!s_email->validate(json::JsonValue("not-an-email")).is_valid(), "Invalid email failed");

    auto s_ipv4 = schema::SchemaBuilder::string().format("ipv4").build();
    TEST_ASSERT(s_ipv4->validate(json::JsonValue("192.168.1.1")).is_valid(), "Valid IPv4 failed");
    TEST_ASSERT(!s_ipv4->validate(json::JsonValue("256.0.0.1")).is_valid(), "Invalid IPv4 failed");

    auto s_uuid = schema::SchemaBuilder::string().format("uuid").build();
    TEST_ASSERT(s_uuid->validate(json::JsonValue("123e4567-e89b-12d3-a456-426614174000")).is_valid(), "Valid UUID failed");
    TEST_ASSERT(!s_uuid->validate(json::JsonValue("invalid-uuid-str")).is_valid(), "Invalid UUID failed");
}

void test_array_validation() {
    auto s = schema::SchemaBuilder::array()
        .minItems(2)
        .maxItems(4)
        .uniqueItems()
        .items(schema::SchemaBuilder::integer())
        .build();

    json::JsonValue valid_arr(json::ArrayType{ json::JsonValue(1), json::JsonValue(2), json::JsonValue(3) });
    json::JsonValue duplicate_arr(json::ArrayType{ json::JsonValue(1), json::JsonValue(2), json::JsonValue(1) });
    json::JsonValue wrong_type_arr(json::ArrayType{ json::JsonValue(1), json::JsonValue("two") });

    TEST_ASSERT(s->validate(valid_arr).is_valid(), "Valid array failed");
    TEST_ASSERT(!s->validate(duplicate_arr).is_valid(), "Unique items failed");
    TEST_ASSERT(!s->validate(wrong_type_arr).is_valid(), "Items type validation failed");
}

void test_object_validation() {
    auto s = schema::SchemaBuilder::object()
        .property("name", schema::SchemaBuilder::string())
        .property("age", schema::SchemaBuilder::integer())
        .required({"name"})
        .additionalProperties(false)
        .dependentRequired("credit_card", {"billing_address"})
        .build();

    json::ObjectType obj1{ {"name", json::JsonValue("Alice")}, {"age", json::JsonValue(30)} };
    json::ObjectType obj_missing_req{ {"age", json::JsonValue(30)} };
    json::ObjectType obj_extra{ {"name", json::JsonValue("Bob")}, {"extra", json::JsonValue(true)} };
    json::ObjectType obj_dep{ {"name", json::JsonValue("Charlie")}, {"credit_card", json::JsonValue("1234")} };

    TEST_ASSERT(s->validate(json::JsonValue(obj1)).is_valid(), "Valid object failed");
    TEST_ASSERT(!s->validate(json::JsonValue(obj_missing_req)).is_valid(), "Required field failed");
    TEST_ASSERT(!s->validate(json::JsonValue(obj_extra)).is_valid(), "Additional properties false failed");
    TEST_ASSERT(!s->validate(json::JsonValue(obj_dep)).is_valid(), "Dependent required failed");
}

void test_enum_and_const() {
    auto s_enum = schema::SchemaBuilder::any()
        .enumValues({json::JsonValue("red"), json::JsonValue("green"), json::JsonValue("blue")})
        .build();

    TEST_ASSERT(s_enum->validate(json::JsonValue("red")).is_valid(), "Enum valid case failed");
    TEST_ASSERT(!s_enum->validate(json::JsonValue("yellow")).is_valid(), "Enum invalid case failed");

    auto s_const = schema::SchemaBuilder::any().constValue(json::JsonValue(42)).build();
    TEST_ASSERT(s_const->validate(json::JsonValue(42)).is_valid(), "Const valid failed");
    TEST_ASSERT(!s_const->validate(json::JsonValue(43)).is_valid(), "Const invalid failed");
}

void test_combinators() {
    // allOf
    auto s_all = schema::SchemaBuilder::any()
        .allOf({
            schema::SchemaBuilder::number().minimum(10),
            schema::SchemaBuilder::number().maximum(20)
        })
        .build();

    TEST_ASSERT(s_all->validate(json::JsonValue(15)).is_valid(), "allOf valid failed");
    TEST_ASSERT(!s_all->validate(json::JsonValue(5)).is_valid(), "allOf invalid failed");

    // oneOf
    auto s_one = schema::SchemaBuilder::any()
        .oneOf({
            schema::SchemaBuilder::number().multipleOf(3),
            schema::SchemaBuilder::number().multipleOf(5)
        })
        .build();

    TEST_ASSERT(s_one->validate(json::JsonValue(9)).is_valid(), "oneOf valid single match 9 failed");
    TEST_ASSERT(s_one->validate(json::JsonValue(10)).is_valid(), "oneOf valid single match 10 failed");
    TEST_ASSERT(!s_one->validate(json::JsonValue(15)).is_valid(), "oneOf double match 15 should fail");
}

void test_declarative_schema_parser() {
    std::string schema_json = R"({
        "title": "TestSchema",
        "type": "object",
        "properties": {
            "id": { "type": "integer", "minimum": 1 },
            "status": { "type": "string", "enum": ["active", "pending"] }
        },
        "required": ["id", "status"],
        "additionalProperties": false
    })";

    auto parse_res = schema::SchemaParser::parse_string(schema_json);
    TEST_ASSERT(std::holds_alternative<schema::SchemaPtr>(parse_res), "Failed to parse schema JSON");
    auto schema_ptr = std::get<schema::SchemaPtr>(parse_res);

    std::string valid_input = R"({"id": 100, "status": "active"})";
    std::string invalid_input = R"({"id": 0, "status": "unknown"})";

    auto input_v = json::Parser::parse(valid_input);
    auto input_inv = json::Parser::parse(invalid_input);

    TEST_ASSERT(schema_ptr->validate(std::get<json::JsonValue>(input_v)).is_valid(), "Declarative schema valid input failed");
    TEST_ASSERT(!schema_ptr->validate(std::get<json::JsonValue>(input_inv)).is_valid(), "Declarative schema invalid input failed");
}

void test_custom_validator_callback() {
    auto s = schema::SchemaBuilder::object()
        .property("val", schema::SchemaBuilder::integer())
        .custom([](const json::JsonValue& val, const std::string& path) -> std::optional<schema::ValidationError> {
            if (val.is_object() && val.has_key("val")) {
                if (val.get("val")->as_integer() % 2 != 0) {
                    return schema::ValidationError{path + "/val", "custom_even", "Value must be an even number"};
                }
            }
            return std::nullopt;
        })
        .build();

    json::ObjectType even_obj{ {"val", json::JsonValue(4)} };
    json::ObjectType odd_obj{ {"val", json::JsonValue(5)} };

    TEST_ASSERT(s->validate(json::JsonValue(even_obj)).is_valid(), "Custom validator even failed");
    TEST_ASSERT(!s->validate(json::JsonValue(odd_obj)).is_valid(), "Custom validator odd failed");
}

int main() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << " RUNNING C++17 JSON VERIFIER UNIT TESTS" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    RUN_TEST(test_json_parser_primitives, "JSON Parser Primitives");
    RUN_TEST(test_json_parser_complex, "JSON Parser Nested Structure");
    RUN_TEST(test_type_validation, "Type Constraint Validation");
    RUN_TEST(test_number_validation, "Number Range & MultipleOf Validation");
    RUN_TEST(test_string_validation, "String Length, Regex & Format Validation");
    RUN_TEST(test_array_validation, "Array Bounds, Items & Uniqueness");
    RUN_TEST(test_object_validation, "Object Properties & Dependencies");
    RUN_TEST(test_enum_and_const, "Enum & Const Validation");
    RUN_TEST(test_combinators, "Logical Combinators (allOf, oneOf)");
    RUN_TEST(test_declarative_schema_parser, "Declarative JSON Schema Parsing");
    RUN_TEST(test_custom_validator_callback, "C++17 Custom Validation Callbacks");

    std::cout << "\n------------------------------------------" << std::endl;
    std::cout << "\033[32mALL " << g_tests_passed << " UNIT TESTS PASSED SUCCESSFULLY!\033[0m" << std::endl;
    std::cout << "------------------------------------------\n" << std::endl;

    return 0;
}
