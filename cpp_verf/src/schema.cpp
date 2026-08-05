#include "schema.hpp"
#include <cmath>
#include <set>
#include <regex>
#include <algorithm>

namespace schema {

ValidationResult Schema::validate(const json::JsonValue& val, const std::string& path) const {
    ValidationResult res;

    validate_type(val, path, res);
    validate_enum_and_const(val, path, res);
    validate_combinators(val, path, res);

    if (val.is_number()) {
        validate_number(val, path, res);
    }
    if (val.is_string()) {
        validate_string(val, path, res);
    }
    if (val.is_array()) {
        validate_array(val, path, res);
    }
    if (val.is_object()) {
        validate_object(val, path, res);
    }

    for (const auto& fn : custom_validators) {
        if (auto err = fn(val, path)) {
            res.add_error(err->path, err->keyword, err->message);
        }
    }

    return res;
}

void Schema::validate_type(const json::JsonValue& val, const std::string& path, ValidationResult& res) const {
    if (allowed_type_names.empty()) return;

    bool match = false;
    std::string actual_type = val.type_name();
    if (val.is_integer() && (std::find(allowed_type_names.begin(), allowed_type_names.end(), "integer") != allowed_type_names.end())) {
        match = true;
    }

    if (!match) {
        for (const auto& t : allowed_type_names) {
            if (t == actual_type) {
                match = true;
                break;
            }
            if (t == "number" && val.is_number()) {
                match = true;
                break;
            }
        }
    }

    if (!match) {
        std::string expected;
        for (size_t i = 0; i < allowed_type_names.size(); ++i) {
            expected += allowed_type_names[i];
            if (i + 1 < allowed_type_names.size()) expected += "|";
        }
        res.add_error(path, "type", "Expected type " + expected + " but got " + actual_type);
    }
}

void Schema::validate_number(const json::JsonValue& val, const std::string& path, ValidationResult& res) const {
    double num = val.as_number();

    if (minimum.has_value() && num < minimum.value()) {
        res.add_error(path, "minimum", "Value " + std::to_string(num) + " is less than minimum " + std::to_string(minimum.value()));
    }
    if (maximum.has_value() && num > maximum.value()) {
        res.add_error(path, "maximum", "Value " + std::to_string(num) + " is greater than maximum " + std::to_string(maximum.value()));
    }
    if (exclusive_minimum.has_value() && num <= exclusive_minimum.value()) {
        res.add_error(path, "exclusiveMinimum", "Value " + std::to_string(num) + " must be strictly greater than " + std::to_string(exclusive_minimum.value()));
    }
    if (exclusive_maximum.has_value() && num >= exclusive_maximum.value()) {
        res.add_error(path, "exclusiveMaximum", "Value " + std::to_string(num) + " must be strictly less than " + std::to_string(exclusive_maximum.value()));
    }
    if (multiple_of.has_value()) {
        double mult = multiple_of.value();
        if (mult != 0) {
            double rem = std::fmod(num, mult);
            if (std::abs(rem) > 1e-9 && std::abs(rem - mult) > 1e-9) {
                res.add_error(path, "multipleOf", "Value " + std::to_string(num) + " is not a multiple of " + std::to_string(mult));
            }
        }
    }
}

void Schema::validate_string(const json::JsonValue& val, const std::string& path, ValidationResult& res) const {
    const std::string& str = val.as_string();
    size_t len = str.length();

    if (min_length.has_value() && len < min_length.value()) {
        res.add_error(path, "minLength", "String length " + std::to_string(len) + " is shorter than minLength " + std::to_string(min_length.value()));
    }
    if (max_length.has_value() && len > max_length.value()) {
        res.add_error(path, "maxLength", "String length " + std::to_string(len) + " exceeds maxLength " + std::to_string(max_length.value()));
    }
    if (pattern.has_value()) {
        try {
            std::regex rx(pattern.value());
            if (!std::regex_search(str, rx)) {
                res.add_error(path, "pattern", "String does not match regex pattern '" + pattern.value() + "'");
            }
        } catch (const std::regex_error& e) {
            res.add_error(path, "pattern", "Invalid regex pattern in schema: " + pattern.value());
        }
    }
    if (format.has_value()) {
        validate_format(str, format.value(), path, res);
    }
}

void Schema::validate_format(const std::string& str, const std::string& fmt, const std::string& path, ValidationResult& res) const {
    if (fmt == "email") {
        static const std::regex email_regex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
        if (!std::regex_match(str, email_regex)) {
            res.add_error(path, "format", "String '" + str + "' is not a valid email address");
        }
    } else if (fmt == "ipv4") {
        static const std::regex ipv4_regex(R"(^((25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])\.){3}(25[0-5]|(2[0-4]|1[0-9]|[1-9]|)[0-9])$)");
        if (!std::regex_match(str, ipv4_regex)) {
            res.add_error(path, "format", "String '" + str + "' is not a valid IPv4 address");
        }
    } else if (fmt == "date-time") {
        // ISO 8601 subset (e.g. 2026-08-05T07:40:00Z)
        static const std::regex datetime_regex(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(\.\d+)?(Z|[+-]\d{2}:\d{2})$)");
        if (!std::regex_match(str, datetime_regex)) {
            res.add_error(path, "format", "String '" + str + "' is not a valid ISO date-time");
        }
    } else if (fmt == "uuid") {
        static const std::regex uuid_regex(R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)");
        if (!std::regex_match(str, uuid_regex)) {
            res.add_error(path, "format", "String '" + str + "' is not a valid UUID");
        }
    } else if (fmt == "uri") {
        static const std::regex uri_regex(R"(^[a-zA-Z][a-zA-Z0-9+-.]*:.+$)");
        if (!std::regex_match(str, uri_regex)) {
            res.add_error(path, "format", "String '" + str + "' is not a valid URI");
        }
    }
}

void Schema::validate_array(const json::JsonValue& val, const std::string& path, ValidationResult& res) const {
    const auto& arr = val.as_array();
    size_t sz = arr.size();

    if (min_items.has_value() && sz < min_items.value()) {
        res.add_error(path, "minItems", "Array size " + std::to_string(sz) + " is less than minItems " + std::to_string(min_items.value()));
    }
    if (max_items.has_value() && sz > max_items.value()) {
        res.add_error(path, "maxItems", "Array size " + std::to_string(sz) + " exceeds maxItems " + std::to_string(max_items.value()));
    }

    if (unique_items) {
        for (size_t i = 0; i < sz; ++i) {
            for (size_t j = i + 1; j < sz; ++j) {
                if (arr[i] == arr[j]) {
                    res.add_error(path, "uniqueItems", "Array items at indices " + std::to_string(i) + " and " + std::to_string(j) + " are duplicate");
                    break;
                }
            }
        }
    }

    for (size_t i = 0; i < sz; ++i) {
        std::string elem_path = path + "/" + std::to_string(i);

        if (i < prefix_items.size()) {
            res.merge(prefix_items[i]->validate(arr[i], elem_path));
        } else if (items_schema) {
            res.merge(items_schema->validate(arr[i], elem_path));
        }
    }
}

void Schema::validate_object(const json::JsonValue& val, const std::string& path, ValidationResult& res) const {
    const auto& obj = val.as_object();
    size_t property_count = obj.size();

    if (min_properties.has_value() && property_count < min_properties.value()) {
        res.add_error(path, "minProperties", "Object property count " + std::to_string(property_count) + " is less than minProperties " + std::to_string(min_properties.value()));
    }
    if (max_properties.has_value() && property_count > max_properties.value()) {
        res.add_error(path, "maxProperties", "Object property count " + std::to_string(property_count) + " exceeds maxProperties " + std::to_string(max_properties.value()));
    }

    // Required fields check
    for (const auto& req : required_properties) {
        if (!val.has_key(req)) {
            res.add_error(path, "required", "Missing required property: '" + req + "'");
        }
    }

    // Dependent required check
    for (const auto& [prop, deps] : dependent_required) {
        if (val.has_key(prop)) {
            for (const auto& dep : deps) {
                if (!val.has_key(dep)) {
                    res.add_error(path, "dependentRequired", "Property '" + prop + "' requires property '" + dep + "' to be present");
                }
            }
        }
    }

    // Property schema validation & additional properties tracking
    std::set<std::string> checked_keys;

    for (const auto& [prop_name, prop_val] : obj) {
        std::string prop_path = path + "/" + prop_name;
        bool matched = false;

        // Explicit properties
        auto it = properties.find(prop_name);
        if (it != properties.end()) {
            matched = true;
            checked_keys.insert(prop_name);
            res.merge(it->second->validate(prop_val, prop_path));
        }

        // Pattern properties
        for (const auto& [pat, schema_ptr] : pattern_properties) {
            try {
                std::regex rx(pat);
                if (std::regex_search(prop_name, rx)) {
                    matched = true;
                    checked_keys.insert(prop_name);
                    res.merge(schema_ptr->validate(prop_val, prop_path));
                }
            } catch (...) {}
        }

        // Additional properties validation
        if (!matched) {
            if (allow_additional_properties.has_value() && !allow_additional_properties.value()) {
                res.add_error(path, "additionalProperties", "Property '" + prop_name + "' is not allowed by schema");
            } else if (additional_properties_schema) {
                res.merge(additional_properties_schema->validate(prop_val, prop_path));
            }
        }
    }
}

void Schema::validate_enum_and_const(const json::JsonValue& val, const std::string& path, ValidationResult& res) const {
    if (const_value.has_value()) {
        if (!(val == const_value.value())) {
            res.add_error(path, "const", "Value does not match expected constant value: " + const_value.value().dump());
        }
    }

    if (enum_values.has_value()) {
        bool match = false;
        for (const auto& enum_val : enum_values.value()) {
            if (val == enum_val) {
                match = true;
                break;
            }
        }
        if (!match) {
            std::string allowed_str;
            for (size_t i = 0; i < enum_values.value().size(); ++i) {
                allowed_str += enum_values.value()[i].dump();
                if (i + 1 < enum_values.value().size()) allowed_str += ", ";
            }
            res.add_error(path, "enum", "Value is not one of the allowed enum values: [" + allowed_str + "]");
        }
    }
}

void Schema::validate_combinators(const json::JsonValue& val, const std::string& path, ValidationResult& res) const {
    // allOf
    for (size_t i = 0; i < all_of.size(); ++i) {
        auto sub_res = all_of[i]->validate(val, path);
        if (!sub_res.is_valid()) {
            res.add_error(path, "allOf", "Failed allOf schema rule #" + std::to_string(i));
            res.merge(sub_res);
        }
    }

    // anyOf
    if (!any_of.empty()) {
        bool pass = false;
        for (const auto& schema_ptr : any_of) {
            if (schema_ptr->validate(val, path).is_valid()) {
                pass = true;
                break;
            }
        }
        if (!pass) {
            res.add_error(path, "anyOf", "Value did not match any of the subschemas in anyOf");
        }
    }

    // oneOf
    if (!one_of.empty()) {
        size_t pass_count = 0;
        for (const auto& schema_ptr : one_of) {
            if (schema_ptr->validate(val, path).is_valid()) {
                pass_count++;
            }
        }
        if (pass_count != 1) {
            res.add_error(path, "oneOf", "Value matched " + std::to_string(pass_count) + " subschemas in oneOf, expected exactly 1");
        }
    }

    // not
    if (not_schema) {
        if (not_schema->validate(val, path).is_valid()) {
            res.add_error(path, "not", "Value matched schema in 'not' clause, but should NOT match");
        }
    }
}

} // namespace schema
