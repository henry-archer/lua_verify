#include "schema_parser.hpp"
#include "builder.hpp"
#include <iostream>

namespace schema {

std::variant<SchemaPtr, std::string> SchemaParser::parse_json(const json::JsonValue& json_schema) {
    if (json_schema.is_bool()) {
        // Boolean schemas: true accepts everything, false rejects everything
        if (json_schema.as_bool()) {
            return std::make_shared<Schema>();
        } else {
            auto s = std::make_shared<Schema>();
            s->not_schema = std::make_shared<Schema>(); // empty schema = match everything, so not = match nothing
            return s;
        }
    }

    if (!json_schema.is_object()) {
        return "Schema root must be a JSON object or boolean";
    }

    auto s = std::make_shared<Schema>();

    // Title / Description
    if (auto title_val = json_schema.get("title"); title_val && title_val->is_string()) {
        s->title = title_val->as_string();
    }
    if (auto desc_val = json_schema.get("description"); desc_val && desc_val->is_string()) {
        s->description = desc_val->as_string();
    }

    // Type
    if (auto type_val = json_schema.get("type")) {
        if (type_val->is_string()) {
            s->allowed_type_names.push_back(type_val->as_string());
        } else if (type_val->is_array()) {
            for (const auto& elem : type_val->as_array()) {
                if (elem.is_string()) {
                    s->allowed_type_names.push_back(elem.as_string());
                }
            }
        }
    }

    // Number keywords
    if (auto val = json_schema.get("minimum"); val && val->is_number()) {
        s->minimum = val->as_number();
    }
    if (auto val = json_schema.get("maximum"); val && val->is_number()) {
        s->maximum = val->as_number();
    }
    if (auto val = json_schema.get("exclusiveMinimum"); val && val->is_number()) {
        s->exclusive_minimum = val->as_number();
    }
    if (auto val = json_schema.get("exclusiveMaximum"); val && val->is_number()) {
        s->exclusive_maximum = val->as_number();
    }
    if (auto val = json_schema.get("multipleOf"); val && val->is_number()) {
        s->multiple_of = val->as_number();
    }

    // String keywords
    if (auto val = json_schema.get("minLength"); val && val->is_number()) {
        s->min_length = static_cast<size_t>(val->as_number());
    }
    if (auto val = json_schema.get("maxLength"); val && val->is_number()) {
        s->max_length = static_cast<size_t>(val->as_number());
    }
    if (auto val = json_schema.get("pattern"); val && val->is_string()) {
        s->pattern = val->as_string();
    }
    if (auto val = json_schema.get("format"); val && val->is_string()) {
        s->format = val->as_string();
    }

    // Array keywords
    if (auto val = json_schema.get("minItems"); val && val->is_number()) {
        s->min_items = static_cast<size_t>(val->as_number());
    }
    if (auto val = json_schema.get("maxItems"); val && val->is_number()) {
        s->max_items = static_cast<size_t>(val->as_number());
    }
    if (auto val = json_schema.get("uniqueItems"); val && val->is_bool()) {
        s->unique_items = val->as_bool();
    }
    if (auto val = json_schema.get("items")) {
        auto sub_res = parse_json(*val);
        if (auto err = std::get_if<std::string>(&sub_res)) return *err;
        s->items_schema = std::get<SchemaPtr>(sub_res);
    }
    if (auto val = json_schema.get("prefixItems"); val && val->is_array()) {
        for (const auto& item_node : val->as_array()) {
            auto sub_res = parse_json(item_node);
            if (auto err = std::get_if<std::string>(&sub_res)) return *err;
            s->prefix_items.push_back(std::get<SchemaPtr>(sub_res));
        }
    }

    // Object keywords
    if (auto val = json_schema.get("properties"); val && val->is_object()) {
        for (const auto& [prop_name, prop_node] : val->as_object()) {
            auto sub_res = parse_json(prop_node);
            if (auto err = std::get_if<std::string>(&sub_res)) return *err;
            s->properties[prop_name] = std::get<SchemaPtr>(sub_res);
        }
    }
    if (auto val = json_schema.get("required"); val && val->is_array()) {
        for (const auto& elem : val->as_array()) {
            if (elem.is_string()) {
                s->required_properties.insert(elem.as_string());
            }
        }
    }
    if (auto val = json_schema.get("additionalProperties")) {
        if (val->is_bool()) {
            s->allow_additional_properties = val->as_bool();
        } else if (val->is_object()) {
            auto sub_res = parse_json(*val);
            if (auto err = std::get_if<std::string>(&sub_res)) return *err;
            s->additional_properties_schema = std::get<SchemaPtr>(sub_res);
        }
    }
    if (auto val = json_schema.get("patternProperties"); val && val->is_object()) {
        for (const auto& [pat, prop_node] : val->as_object()) {
            auto sub_res = parse_json(prop_node);
            if (auto err = std::get_if<std::string>(&sub_res)) return *err;
            s->pattern_properties.emplace_back(pat, std::get<SchemaPtr>(sub_res));
        }
    }
    if (auto val = json_schema.get("minProperties"); val && val->is_number()) {
        s->min_properties = static_cast<size_t>(val->as_number());
    }
    if (auto val = json_schema.get("maxProperties"); val && val->is_number()) {
        s->max_properties = static_cast<size_t>(val->as_number());
    }
    if (auto val = json_schema.get("dependentRequired"); val && val->is_object()) {
        for (const auto& [prop_name, deps_node] : val->as_object()) {
            if (deps_node.is_array()) {
                std::vector<std::string> deps;
                for (const auto& d : deps_node.as_array()) {
                    if (d.is_string()) deps.push_back(d.as_string());
                }
                s->dependent_required[prop_name] = std::move(deps);
            }
        }
    }

    // Enum & Const
    if (auto val = json_schema.get("enum"); val && val->is_array()) {
        s->enum_values = val->as_array();
    }
    if (auto val = json_schema.get("const")) {
        s->const_value = *val;
    }

    // Combinators
    if (auto val = json_schema.get("allOf"); val && val->is_array()) {
        for (const auto& sub : val->as_array()) {
            auto sub_res = parse_json(sub);
            if (auto err = std::get_if<std::string>(&sub_res)) return *err;
            s->all_of.push_back(std::get<SchemaPtr>(sub_res));
        }
    }
    if (auto val = json_schema.get("anyOf"); val && val->is_array()) {
        for (const auto& sub : val->as_array()) {
            auto sub_res = parse_json(sub);
            if (auto err = std::get_if<std::string>(&sub_res)) return *err;
            s->any_of.push_back(std::get<SchemaPtr>(sub_res));
        }
    }
    if (auto val = json_schema.get("oneOf"); val && val->is_array()) {
        for (const auto& sub : val->as_array()) {
            auto sub_res = parse_json(sub);
            if (auto err = std::get_if<std::string>(&sub_res)) return *err;
            s->one_of.push_back(std::get<SchemaPtr>(sub_res));
        }
    }
    if (auto val = json_schema.get("not")) {
        auto sub_res = parse_json(*val);
        if (auto err = std::get_if<std::string>(&sub_res)) return *err;
        s->not_schema = std::get<SchemaPtr>(sub_res);
    }

    return s;
}

std::variant<SchemaPtr, std::string> SchemaParser::parse_string(std::string_view json_str) {
    auto parsed = json::Parser::parse(json_str);
    if (auto err = std::get_if<json::ParseError>(&parsed)) {
        return "JSON parsing error (line " + std::to_string(err->line) + ", col " + std::to_string(err->column) + "): " + err->message;
    }
    return parse_json(std::get<json::JsonValue>(parsed));
}

std::variant<SchemaPtr, std::string> SchemaParser::parse_file(const std::string& filepath) {
    auto parsed = json::Parser::parse_file(filepath);
    if (auto err = std::get_if<json::ParseError>(&parsed)) {
        return "JSON file read/parse error: " + err->message;
    }
    return parse_json(std::get<json::JsonValue>(parsed));
}

} // namespace schema
