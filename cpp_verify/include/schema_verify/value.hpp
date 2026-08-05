#ifndef SCHEMA_VERIFY_VALUE_HPP
#define SCHEMA_VERIFY_VALUE_HPP

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <initializer_list>
#include <utility>
#include <type_traits>
#include <cmath>

namespace schema_verify {

enum class ValueType {
    Null,
    Bool,
    Int,
    Float,
    String,
    Array,
    Object
};

inline std::string value_type_to_string(ValueType type) {
    switch (type) {
        case ValueType::Null: return "Null";
        case ValueType::Bool: return "Bool";
        case ValueType::Int: return "Integer";
        case ValueType::Float: return "Float";
        case ValueType::String: return "String";
        case ValueType::Array: return "Array";
        case ValueType::Object: return "Object";
    }
    return "Unknown";
}

class Value {
public:
    using ArrayType = std::vector<Value>;
    using ObjectType = std::map<std::string, Value>;
    using VariantType = std::variant<
        std::monostate,
        bool,
        int64_t,
        double,
        std::string,
        ArrayType,
        ObjectType
    >;

private:
    VariantType data_;

public:
    Value() : data_(std::monostate{}) {}
    Value(std::nullptr_t) : data_(std::monostate{}) {}
    Value(bool b) : data_(b) {}
    Value(int i) : data_(static_cast<int64_t>(i)) {}
    Value(long i) : data_(static_cast<int64_t>(i)) {}
    Value(long long i) : data_(static_cast<int64_t>(i)) {}
    Value(double d) : data_(d) {}
    Value(const char* s) : data_(std::string(s)) {}
    Value(std::string s) : data_(std::move(s)) {}
    Value(ArrayType arr) : data_(std::move(arr)) {}
    Value(ObjectType obj) : data_(std::move(obj)) {}

    Value(std::initializer_list<Value> list) : data_(ArrayType(list)) {}

    static Value object(ObjectType obj = {}) {
        return Value(std::move(obj));
    }

    static Value array(ArrayType arr = {}) {
        return Value(std::move(arr));
    }

    ValueType type() const {
        return std::visit([](auto&& arg) -> ValueType {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) return ValueType::Null;
            else if constexpr (std::is_same_v<T, bool>) return ValueType::Bool;
            else if constexpr (std::is_same_v<T, int64_t>) return ValueType::Int;
            else if constexpr (std::is_same_v<T, double>) return ValueType::Float;
            else if constexpr (std::is_same_v<T, std::string>) return ValueType::String;
            else if constexpr (std::is_same_v<T, ArrayType>) return ValueType::Array;
            else if constexpr (std::is_same_v<T, ObjectType>) return ValueType::Object;
        }, data_);
    }

    bool is_null() const { return std::holds_alternative<std::monostate>(data_); }
    bool is_bool() const { return std::holds_alternative<bool>(data_); }
    bool is_int() const { return std::holds_alternative<int64_t>(data_); }
    bool is_float() const { return std::holds_alternative<double>(data_); }
    bool is_number() const { return is_int() || is_float(); }
    bool is_string() const { return std::holds_alternative<std::string>(data_); }
    bool is_array() const { return std::holds_alternative<ArrayType>(data_); }
    bool is_object() const { return std::holds_alternative<ObjectType>(data_); }

    bool as_bool() const { return std::get<bool>(data_); }
    int64_t as_int() const { return std::get<int64_t>(data_); }
    double as_float() const { return std::get<double>(data_); }
    double as_number() const {
        if (is_int()) return static_cast<double>(as_int());
        return as_float();
    }
    const std::string& as_string() const { return std::get<std::string>(data_); }
    const ArrayType& as_array() const { return std::get<ArrayType>(data_); }
    ArrayType& as_array() { return std::get<ArrayType>(data_); }
    const ObjectType& as_object() const { return std::get<ObjectType>(data_); }
    ObjectType& as_object() { return std::get<ObjectType>(data_); }

    bool has(const std::string& key) const {
        if (!is_object()) return false;
        const auto& obj = as_object();
        return obj.find(key) != obj.end();
    }

    const Value* get(const std::string& key) const {
        if (!is_object()) return nullptr;
        const auto& obj = as_object();
        auto it = obj.find(key);
        if (it != obj.end()) return &it->second;
        return nullptr;
    }

    Value& operator[](const std::string& key) {
        if (!is_object()) {
            data_ = ObjectType{};
        }
        return std::get<ObjectType>(data_)[key];
    }

    bool operator==(const Value& other) const {
        if (type() != other.type()) {
            if (is_number() && other.is_number()) {
                return std::abs(as_number() - other.as_number()) < 1e-9;
            }
            return false;
        }
        switch (type()) {
            case ValueType::Null: return true;
            case ValueType::Bool: return as_bool() == other.as_bool();
            case ValueType::Int: return as_int() == other.as_int();
            case ValueType::Float: return std::abs(as_float() - other.as_float()) < 1e-9;
            case ValueType::String: return as_string() == other.as_string();
            case ValueType::Array: return as_array() == other.as_array();
            case ValueType::Object: return as_object() == other.as_object();
        }
        return false;
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    std::string to_string() const {
        std::ostringstream ss;
        switch (type()) {
            case ValueType::Null: ss << "null"; break;
            case ValueType::Bool: ss << (as_bool() ? "true" : "false"); break;
            case ValueType::Int: ss << as_int(); break;
            case ValueType::Float: ss << as_float(); break;
            case ValueType::String: ss << "\"" << as_string() << "\""; break;
            case ValueType::Array: {
                ss << "[";
                const auto& arr = as_array();
                for (size_t i = 0; i < arr.size(); ++i) {
                    if (i > 0) ss << ", ";
                    ss << arr[i].to_string();
                }
                ss << "]";
                break;
            }
            case ValueType::Object: {
                ss << "{";
                const auto& obj = as_object();
                size_t i = 0;
                for (const auto& [k, v] : obj) {
                    if (i++ > 0) ss << ", ";
                    ss << "\"" << k << "\": " << v.to_string();
                }
                ss << "}";
                break;
            }
        }
        return ss.str();
    }
};

} // namespace schema_verify

#endif // SCHEMA_VERIFY_VALUE_HPP
