#ifndef SCHEMA_PARSER_HPP
#define SCHEMA_PARSER_HPP

#include "schema.hpp"
#include "json.hpp"
#include <string>
#include <variant>

namespace schema {

class SchemaParser {
public:
    static std::variant<SchemaPtr, std::string> parse_json(const json::JsonValue& json_schema);
    static std::variant<SchemaPtr, std::string> parse_string(std::string_view json_str);
    static std::variant<SchemaPtr, std::string> parse_file(const std::string& filepath);
};

} // namespace schema

#endif // SCHEMA_PARSER_HPP
