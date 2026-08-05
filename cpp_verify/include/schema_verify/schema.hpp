#ifndef SCHEMA_VERIFY_SCHEMA_HPP
#define SCHEMA_VERIFY_SCHEMA_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <functional>
#include <sstream>

#include "value.hpp"
#include "error.hpp"
#include "levenshtein.hpp"

namespace schema_verify {

enum class SchemaType {
    Null,
    Bool,
    Int,
    Float,
    Number,
    String,
    Array,
    Object,
    Any
};

inline std::string schema_type_to_string(SchemaType type) {
    switch (type) {
        case SchemaType::Null: return "Null";
        case SchemaType::Bool: return "Bool";
        case SchemaType::Int: return "Integer";
        case SchemaType::Float: return "Float";
        case SchemaType::Number: return "Number";
        case SchemaType::String: return "String";
        case SchemaType::Array: return "Array";
        case SchemaType::Object: return "Object";
        case SchemaType::Any: return "Any";
    }
    return "Unknown";
}

class Schema;
struct Property;

class Schema {
private:
    SchemaType type_{SchemaType::Any};
    std::string description_;
    std::optional<double> min_value_;
    std::optional<double> max_value_;
    std::optional<size_t> min_length_;
    std::optional<size_t> max_length_;
    std::vector<Value> allowed_values_;
    
    // Array
    std::shared_ptr<Schema> item_schema_;

    // Object
    std::map<std::string, Property> properties_;
    bool allow_additional_properties_{false};

    // Custom Callback
    std::function<void(const Value&, const std::string&, ValidationResult&)> custom_validator_;

public:
    Schema() = default;
    explicit Schema(SchemaType type) : type_(type) {}

    // Factory builders
    static Schema null() { return Schema(SchemaType::Null); }
    static Schema boolean() { return Schema(SchemaType::Bool); }
    static Schema integer() { return Schema(SchemaType::Int); }
    static Schema float_num() { return Schema(SchemaType::Float); }
    static Schema number() { return Schema(SchemaType::Number); }
    static Schema string() { return Schema(SchemaType::String); }
    static Schema any() { return Schema(SchemaType::Any); }

    static Schema array(Schema item_schema) {
        Schema s(SchemaType::Array);
        s.item_schema_ = std::make_shared<Schema>(std::move(item_schema));
        return s;
    }

    static Schema object(std::map<std::string, Property> properties = {}, bool allow_additional = false);

    // Fluent Modifiers
    Schema& description(std::string desc) {
        description_ = std::move(desc);
        return *this;
    }

    Schema& range(double min_val, double max_val) {
        min_value_ = min_val;
        max_value_ = max_val;
        return *this;
    }

    Schema& min_val(double min_val) {
        min_value_ = min_val;
        return *this;
    }

    Schema& max_val(double max_val) {
        max_value_ = max_val;
        return *this;
    }

    Schema& min_len(size_t min_len) {
        min_length_ = min_len;
        return *this;
    }

    Schema& max_len(size_t max_len) {
        max_length_ = max_len;
        return *this;
    }

    Schema& allowed(std::vector<Value> values) {
        allowed_values_ = std::move(values);
        return *this;
    }

    Schema& allow_additional(bool allow) {
        allow_additional_properties_ = allow;
        return *this;
    }

    Schema& custom(std::function<void(const Value&, const std::string&, ValidationResult&)> validator) {
        custom_validator_ = std::move(validator);
        return *this;
    }

    // Getters
    SchemaType type() const { return type_; }
    const std::string& description() const { return description_; }
    const std::optional<double>& min_value() const { return min_value_; }
    const std::optional<double>& max_value() const { return max_value_; }
    const std::optional<size_t>& min_length() const { return min_length_; }
    const std::optional<size_t>& max_length() const { return max_length_; }
    const std::vector<Value>& allowed_values() const { return allowed_values_; }
    const std::shared_ptr<Schema>& item_schema() const { return item_schema_; }
    const std::map<std::string, Property>& properties() const { return properties_; }
    bool allow_additional_properties() const { return allow_additional_properties_; }

    // Validation engine
    ValidationResult validate(const Value& val) const {
        ValidationResult result;
        validate(val, "", result);
        return result;
    }

    void validate(const Value& val, const std::string& path, ValidationResult& result) const;

private:
    bool check_type(const Value& val) const {
        switch (type_) {
            case SchemaType::Any: return true;
            case SchemaType::Null: return val.is_null();
            case SchemaType::Bool: return val.is_bool();
            case SchemaType::Int: return val.is_int();
            case SchemaType::Float: return val.is_float() || val.is_number();
            case SchemaType::Number: return val.is_number();
            case SchemaType::String: return val.is_string();
            case SchemaType::Array: return val.is_array();
            case SchemaType::Object: return val.is_object();
        }
        return false;
    }
};

struct Property {
    Schema schema;
    bool is_required{false};

    Property() = default;
    Property(Schema s, bool req) : schema(std::move(s)), is_required(req) {}

    static Property required(Schema s) {
        return Property(std::move(s), true);
    }

    static Property optional(Schema s) {
        return Property(std::move(s), false);
    }

    static Property req(Schema s) { return required(std::move(s)); }
    static Property opt(Schema s) { return optional(std::move(s)); }
};

inline Schema Schema::object(std::map<std::string, Property> properties, bool allow_additional) {
    Schema s(SchemaType::Object);
    s.properties_ = std::move(properties);
    s.allow_additional_properties_ = allow_additional;
    return s;
}

inline void Schema::validate(const Value& val, const std::string& path, ValidationResult& result) const {
    // 1. Check type compatibility
    if (!check_type(val)) {
        std::ostringstream ss;
        ss << "Type mismatch: expected " << schema_type_to_string(type_)
           << ", got " << value_type_to_string(val.type());
        result.add_error(path, ss.str());
        return; // Exit early for this field if type doesn't match
    }

    // 2. Check Range / Constraints for Numbers
    if (val.is_number()) {
        double num = val.as_number();
        if (min_value_.has_value() && num < *min_value_) {
            std::ostringstream ss;
            ss << "Value " << num << " is less than minimum allowed value (" << *min_value_ << ")";
            result.add_error(path, ss.str());
        }
        if (max_value_.has_value() && num > *max_value_) {
            std::ostringstream ss;
            ss << "Value " << num << " is greater than maximum allowed value (" << *max_value_ << ")";
            result.add_error(path, ss.str());
        }
    }

    // 3. Check Length Constraints for Strings
    if (val.is_string()) {
        size_t len = val.as_string().length();
        if (min_length_.has_value() && len < *min_length_) {
            std::ostringstream ss;
            ss << "String length " << len << " is less than minimum length (" << *min_length_ << ")";
            result.add_error(path, ss.str());
        }
        if (max_length_.has_value() && len > *max_length_) {
            std::ostringstream ss;
            ss << "String length " << len << " is greater than maximum length (" << *max_length_ << ")";
            result.add_error(path, ss.str());
        }
    }

    // 4. Check Length Constraints for Arrays
    if (val.is_array()) {
        size_t len = val.as_array().size();
        if (min_length_.has_value() && len < *min_length_) {
            std::ostringstream ss;
            ss << "Array size " << len << " is less than minimum size (" << *min_length_ << ")";
            result.add_error(path, ss.str());
        }
        if (max_length_.has_value() && len > *max_length_) {
            std::ostringstream ss;
            ss << "Array size " << len << " is greater than maximum size (" << *max_length_ << ")";
            result.add_error(path, ss.str());
        }
    }

    // 5. Check Allowed Enum Values
    if (!allowed_values_.empty()) {
        bool found = false;
        for (const auto& allowed : allowed_values_) {
            if (val == allowed) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::ostringstream ss;
            ss << "Value " << val.to_string() << " is not in set of allowed values [";
            for (size_t i = 0; i < allowed_values_.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << allowed_values_[i].to_string();
            }
            ss << "]";
            result.add_error(path, ss.str());
        }
    }

    // 6. Array Item Validation
    if (val.is_array() && item_schema_) {
        const auto& arr = val.as_array();
        for (size_t i = 0; i < arr.size(); ++i) {
            std::string item_path = path.empty() ? "[" + std::to_string(i) + "]"
                                                 : path + "[" + std::to_string(i) + "]";
            item_schema_->validate(arr[i], item_path, result);
        }
    }

    // 7. Object Properties & Levenshtein Key Check
    if (val.is_object()) {
        const auto& obj = val.as_object();
        std::vector<std::string> valid_keys;

        // Validate defined properties
        for (const auto& [prop_name, prop] : properties_) {
            valid_keys.push_back(prop_name);
            std::string prop_path = path.empty() ? prop_name : path + "." + prop_name;

            auto it = obj.find(prop_name);
            if (it == obj.end()) {
                if (prop.is_required) {
                    std::ostringstream ss;
                    ss << "Missing required property '" << prop_name << "'";
                    result.add_error(prop_path, ss.str());
                }
            } else {
                // Recursively validate child value against child schema
                prop.schema.validate(it->second, prop_path, result);
            }
        }

        // Detect unknown keys
        for (const auto& [k, v] : obj) {
            if (properties_.find(k) == properties_.end()) {
                if (!allow_additional_properties_) {
                    std::string key_path = path.empty() ? k : path + "." + k;
                    auto suggestion = suggest_closest_key(k, valid_keys, 3);
                    std::ostringstream ss;
                    ss << "Unrecognised property '" << k << "'";
                    result.add_error(key_path, ss.str(), suggestion);
                }
            }
        }
    }

    // 8. Custom Callback
    if (custom_validator_) {
        custom_validator_(val, path, result);
    }
}

} // namespace schema_verify

#endif // SCHEMA_VERIFY_SCHEMA_HPP
