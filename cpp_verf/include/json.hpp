#ifndef JSON_HPP
#define JSON_HPP

#include <string>
#include <vector>
#include <variant>
#include <utility>
#include <optional>
#include <ostream>
#include <string_view>
#include <cstdint>

namespace json {

enum class Type {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

class JsonValue;

using NullType = std::nullptr_t;
using BoolType = bool;
using NumberType = double;
using StringType = std::string;
using ArrayType = std::vector<JsonValue>;
using ObjectType = std::vector<std::pair<std::string, JsonValue>>;

class JsonValue {
public:
    using VariantType = std::variant<NullType, BoolType, NumberType, StringType, ArrayType, ObjectType>;

    JsonValue() : data_(nullptr) {}
    JsonValue(std::nullptr_t) : data_(nullptr) {}
    JsonValue(bool b) : data_(b) {}
    JsonValue(double d) : data_(d) {}
    JsonValue(int i) : data_(static_cast<double>(i)) {}
    JsonValue(int64_t i) : data_(static_cast<double>(i)) {}
    JsonValue(const char* s) : data_(std::string(s)) {}
    JsonValue(std::string s) : data_(std::move(s)) {}
    JsonValue(ArrayType arr) : data_(std::move(arr)) {}
    JsonValue(ObjectType obj) : data_(std::move(obj)) {}

    Type type() const;
    std::string type_name() const;
    
    bool is_null() const { return type() == Type::Null; }
    bool is_bool() const { return type() == Type::Boolean; }
    bool is_number() const { return type() == Type::Number; }
    bool is_integer() const;
    bool is_string() const { return type() == Type::String; }
    bool is_array() const { return type() == Type::Array; }
    bool is_object() const { return type() == Type::Object; }

    bool as_bool() const;
    double as_number() const;
    int64_t as_integer() const;
    const std::string& as_string() const;
    const ArrayType& as_array() const;
    const ObjectType& as_object() const;

    bool has_key(const std::string& key) const;
    const JsonValue* get(const std::string& key) const;

    std::string dump(int indent = -1, int current_indent = 0) const;

    bool operator==(const JsonValue& other) const;
    bool operator!=(const JsonValue& other) const { return !(*this == other); }

    const VariantType& raw_variant() const { return data_; }

private:
    VariantType data_;
};

struct ParseError {
    std::string message;
    size_t line{1};
    size_t column{1};
};

class Parser {
public:
    static std::variant<JsonValue, ParseError> parse(std::string_view json_str);
    static std::variant<JsonValue, ParseError> parse_file(const std::string& filepath);
};

} // namespace json

#endif // JSON_HPP
