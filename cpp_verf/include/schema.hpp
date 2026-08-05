#ifndef SCHEMA_HPP
#define SCHEMA_HPP

#include "json.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <map>
#include <functional>
#include <set>
#include <regex>

namespace schema {

struct ValidationError {
    std::string path;    // JSON pointer, e.g. "#/users/0/email"
    std::string keyword; // JSON Schema keyword, e.g. "type", "minimum", "required"
    std::string message; // Readable error message

    std::string to_string() const {
        return "[" + (path.empty() ? "#" : path) + "] " + keyword + " error: " + message;
    }
};

class ValidationResult {
public:
    ValidationResult() = default;

    bool is_valid() const { return errors_.empty(); }
    const std::vector<ValidationError>& errors() const { return errors_; }

    void add_error(std::string path, std::string keyword, std::string message) {
        errors_.push_back({std::move(path), std::move(keyword), std::move(message)});
    }

    void merge(const ValidationResult& other) {
        errors_.insert(errors_.end(), other.errors_.begin(), other.errors_.end());
    }

private:
    std::vector<ValidationError> errors_;
};

class Schema;
using SchemaPtr = std::shared_ptr<Schema>;
using CustomValidator = std::function<std::optional<ValidationError>(const json::JsonValue& value, const std::string& path)>;

class Schema {
public:
    Schema() = default;

    // Type checking
    std::vector<std::string> allowed_type_names;

    // Number keywords
    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<double> exclusive_minimum;
    std::optional<double> exclusive_maximum;
    std::optional<double> multiple_of;

    // String keywords
    std::optional<size_t> min_length;
    std::optional<size_t> max_length;
    std::optional<std::string> pattern;
    std::optional<std::string> format;

    // Array keywords
    std::optional<size_t> min_items;
    std::optional<size_t> max_items;
    bool unique_items{false};
    SchemaPtr items_schema;
    std::vector<SchemaPtr> prefix_items;

    // Object keywords
    std::map<std::string, SchemaPtr> properties;
    std::set<std::string> required_properties;
    std::optional<bool> allow_additional_properties;
    SchemaPtr additional_properties_schema;
    std::vector<std::pair<std::string, SchemaPtr>> pattern_properties;
    std::optional<size_t> min_properties;
    std::optional<size_t> max_properties;
    std::map<std::string, std::vector<std::string>> dependent_required;

    // Enum & Const keywords
    std::optional<std::vector<json::JsonValue>> enum_values;
    std::optional<json::JsonValue> const_value;

    // Logical combinators
    std::vector<SchemaPtr> all_of;
    std::vector<SchemaPtr> any_of;
    std::vector<SchemaPtr> one_of;
    SchemaPtr not_schema;

    // C++ Custom Functional Validator
    std::vector<CustomValidator> custom_validators;

    // Title / Description metadata
    std::string title;
    std::string description;

    ValidationResult validate(const json::JsonValue& val, const std::string& path = "#") const;

private:
    void validate_type(const json::JsonValue& val, const std::string& path, ValidationResult& res) const;
    void validate_number(const json::JsonValue& val, const std::string& path, ValidationResult& res) const;
    void validate_string(const json::JsonValue& val, const std::string& path, ValidationResult& res) const;
    void validate_array(const json::JsonValue& val, const std::string& path, ValidationResult& res) const;
    void validate_object(const json::JsonValue& val, const std::string& path, ValidationResult& res) const;
    void validate_enum_and_const(const json::JsonValue& val, const std::string& path, ValidationResult& res) const;
    void validate_combinators(const json::JsonValue& val, const std::string& path, ValidationResult& res) const;
    void validate_format(const std::string& str, const std::string& fmt, const std::string& path, ValidationResult& res) const;
};

} // namespace schema

#endif // SCHEMA_HPP
