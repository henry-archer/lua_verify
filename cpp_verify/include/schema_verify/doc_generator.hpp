#ifndef SCHEMA_VERIFY_DOC_GENERATOR_HPP
#define SCHEMA_VERIFY_DOC_GENERATOR_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#include "schema.hpp"

namespace schema_verify {

struct DocRow {
    std::string path;
    std::string type_name;
    bool required{false};
    std::string description;
    std::string constraints;
};

class DocGenerator {
public:
    static std::vector<DocRow> collect_rows(const Schema& schema) {
        std::vector<DocRow> rows;
        collect_rows_recursive(schema, "", false, rows);
        return rows;
    }

    static std::string generate_markdown(const Schema& schema,
                                        const std::string& title = "Schema Documentation",
                                        const std::string& description = "") {
        auto rows = collect_rows(schema);
        std::ostringstream ss;

        ss << "# " << title << "\n\n";
        if (!description.empty()) {
            ss << description << "\n\n";
        }

        ss << "| Field Path | Type | Presence | Description | Constraints |\n";
        ss << "| :--- | :--- | :--- | :--- | :--- |\n";

        for (const auto& row : rows) {
            ss << "| `" << (row.path.empty() ? "$" : row.path) << "` "
               << "| " << row.type_name << " "
               << "| " << (row.required ? "**Required**" : "Optional") << " "
               << "| " << (row.description.empty() ? "-" : row.description) << " "
               << "| " << (row.constraints.empty() ? "-" : row.constraints) << " |\n";
        }

        return ss.str();
    }

    static std::string generate_text(const Schema& schema,
                                     const std::string& title = "Schema Documentation",
                                     const std::string& description = "") {
        auto rows = collect_rows(schema);
        std::ostringstream ss;

        ss << "================================================================================\n";
        ss << "  " << title << "\n";
        ss << "================================================================================\n";
        if (!description.empty()) {
            ss << description << "\n\n";
        }

        for (const auto& row : rows) {
            ss << "Field: " << (row.path.empty() ? "Root ($)" : row.path) << "\n";
            ss << "  Type:        " << row.type_name << "\n";
            ss << "  Presence:    " << (row.required ? "Required" : "Optional") << "\n";
            if (!row.description.empty()) {
                ss << "  Description: " << row.description << "\n";
            }
            if (!row.constraints.empty()) {
                ss << "  Constraints: " << row.constraints << "\n";
            }
            ss << "--------------------------------------------------------------------------------\n";
        }

        return ss.str();
    }

private:
    static void collect_rows_recursive(const Schema& schema,
                                       const std::string& current_path,
                                       bool required,
                                       std::vector<DocRow>& rows) {
        std::string type_name;
        if (schema.type() == SchemaType::Array && schema.item_schema()) {
            type_name = "Array of " + schema_type_to_string(schema.item_schema()->type());
        } else {
            type_name = schema_type_to_string(schema.type());
        }

        std::string constraints = format_constraints(schema);

        if (!current_path.empty()) {
            rows.push_back(DocRow{
                current_path,
                type_name,
                required,
                schema.description(),
                constraints
            });
        }

        if (schema.type() == SchemaType::Object) {
            for (const auto& [prop_name, prop] : schema.properties()) {
                std::string child_path = current_path.empty() ? prop_name : current_path + "." + prop_name;
                collect_rows_recursive(prop.schema, child_path, prop.is_required, rows);
            }
        } else if (schema.type() == SchemaType::Array && schema.item_schema()) {
            if (schema.item_schema()->type() == SchemaType::Object) {
                std::string child_path = current_path + "[*]";
                collect_rows_recursive(*schema.item_schema(), child_path, false, rows);
            }
        }
    }

    static std::string format_constraints(const Schema& schema) {
        std::vector<std::string> parts;

        if (schema.min_value().has_value() && schema.max_value().has_value()) {
            std::ostringstream ss;
            ss << "Range: [" << *schema.min_value() << ", " << *schema.max_value() << "]";
            parts.push_back(ss.str());
        } else if (schema.min_value().has_value()) {
            std::ostringstream ss;
            ss << "Min: " << *schema.min_value();
            parts.push_back(ss.str());
        } else if (schema.max_value().has_value()) {
            std::ostringstream ss;
            ss << "Max: " << *schema.max_value();
            parts.push_back(ss.str());
        }

        if (schema.min_length().has_value() && schema.max_length().has_value()) {
            std::ostringstream ss;
            ss << "Length: [" << *schema.min_length() << ", " << *schema.max_length() << "]";
            parts.push_back(ss.str());
        } else if (schema.min_length().has_value()) {
            std::ostringstream ss;
            ss << "Min Length: " << *schema.min_length();
            parts.push_back(ss.str());
        } else if (schema.max_length().has_value()) {
            std::ostringstream ss;
            ss << "Max Length: " << *schema.max_length();
            parts.push_back(ss.str());
        }

        if (!schema.allowed_values().empty()) {
            std::ostringstream ss;
            ss << "Allowed: [";
            for (size_t i = 0; i < schema.allowed_values().size(); ++i) {
                if (i > 0) ss << ", ";
                ss << schema.allowed_values()[i].to_string();
            }
            ss << "]";
            parts.push_back(ss.str());
        }

        std::ostringstream result;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) result << "; ";
            result << parts[i];
        }

        return result.str();
    }
};

} // namespace schema_verify

#endif // SCHEMA_VERIFY_DOC_GENERATOR_HPP
