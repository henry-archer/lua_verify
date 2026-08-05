#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "json.hpp"
#include "schema.hpp"
#include "schema_parser.hpp"
#include "builder.hpp"

// ANSI Color Codes
constexpr const char* COLOR_RESET   = "\033[0m";
constexpr const char* COLOR_RED     = "\033[31m";
constexpr const char* COLOR_GREEN   = "\033[32m";
constexpr const char* COLOR_YELLOW  = "\033[33m";
constexpr const char* COLOR_CYAN    = "\033[36m";
constexpr const char* COLOR_BOLD    = "\033[1m";

void print_usage(const char* prog_name) {
    std::cout << COLOR_BOLD << "JSON Schema Verifier (C++17 Engine)" << COLOR_RESET << "\n"
              << "Usage:\n"
              << "  " << prog_name << " --schema <schema.json> --input <data.json> [OPTIONS]\n"
              << "  " << prog_name << " -s <schema.json> -i <data.json>\n"
              << "  " << prog_name << " --demo\n\n"
              << "Options:\n"
              << "  -s, --schema <file>    Path to the JSON Schema definition file\n"
              << "  -i, --input <file>     Path to the JSON input file to verify\n"
              << "  -d, --demo             Run programmatic C++17 schema demonstration\n"
              << "  -v, --verbose          Print schema summary details\n"
              << "  -h, --help             Display this help message\n";
}

void run_demo() {
    std::cout << COLOR_BOLD << COLOR_CYAN << "=== Running C++17 Programmatic Schema Builder Demo ===" << COLOR_RESET << "\n\n";

    // Build schema in pure C++17 using SchemaBuilder
    auto user_schema = schema::SchemaBuilder::object()
        .title("UserProfileSchema")
        .description("Schema for user profile verification")
        .property("username", schema::SchemaBuilder::string().minLength(3).maxLength(20).pattern("^[a-zA-Z0-9_]+$"))
        .property("age", schema::SchemaBuilder::integer().minimum(18).maximum(120))
        .property("email", schema::SchemaBuilder::string().format("email"))
        .property("role", schema::SchemaBuilder::string().enumValues({"admin", "user", "guest"}))
        .property("tags", schema::SchemaBuilder::array().items(schema::SchemaBuilder::string()).uniqueItems())
        .required({"username", "email", "age"})
        .additionalProperties(false)
        // Custom C++ validator callback (C++17 functional extension)
        .custom([](const json::JsonValue& val, const std::string& path) -> std::optional<schema::ValidationError> {
            (void)path;
            if (val.is_object() && val.has_key("username") && val.has_key("role")) {
                std::string uname = val.get("username")->as_string();
                std::string r = val.get("role")->as_string();
                if (r == "admin" && uname != "admin_" + uname.substr(6)) {
                    // Check custom domain logic
                }
            }
            return std::nullopt;
        })
        .build();

    // Valid sample JSON
    std::string valid_json_str = R"({
        "username": "john_doe",
        "age": 25,
        "email": "john.doe@example.com",
        "role": "user",
        "tags": ["developer", "cpp"]
    })";

    // Invalid sample JSON
    std::string invalid_json_str = R"({
        "username": "j",
        "age": 15,
        "email": "invalid-email-address",
        "role": "superadmin",
        "tags": ["dev", "dev"],
        "extra_field": 123
    })";

    std::cout << COLOR_BOLD << "Test 1: Valid User Input" << COLOR_RESET << "\n";
    auto val_res1 = json::Parser::parse(valid_json_str);
    if (auto node = std::get_if<json::JsonValue>(&val_res1)) {
        auto result = user_schema->validate(*node);
        if (result.is_valid()) {
            std::cout << COLOR_GREEN << " [SUCCESS] Input JSON is valid against the schema!" << COLOR_RESET << "\n\n";
        } else {
            std::cout << COLOR_RED << " [FAILED] Unexpected validation failure!" << COLOR_RESET << "\n\n";
        }
    }

    std::cout << COLOR_BOLD << "Test 2: Invalid User Input (Testing error detection)" << COLOR_RESET << "\n";
    auto val_res2 = json::Parser::parse(invalid_json_str);
    if (auto node = std::get_if<json::JsonValue>(&val_res2)) {
        auto result = user_schema->validate(*node);
        if (!result.is_valid()) {
            std::cout << COLOR_YELLOW << " [PASSED DEMO] Verification caught " << result.errors().size() << " schema violations as expected:" << COLOR_RESET << "\n";
            for (const auto& err : result.errors()) {
                std::cout << "  " << COLOR_RED << "• " << err.to_string() << COLOR_RESET << "\n";
            }
        } else {
            std::cout << COLOR_RED << " [FAILED] Expected validation errors, but none were caught!" << COLOR_RESET << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    std::string schema_file;
    std::string input_file;
    bool demo_mode = false;
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-d" || arg == "--demo") {
            demo_mode = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if ((arg == "-s" || arg == "--schema") && i + 1 < argc) {
            schema_file = argv[++i];
        } else if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            input_file = argv[++i];
        }
    }

    if (demo_mode) {
        run_demo();
        return 0;
    }

    if (schema_file.empty() || input_file.empty()) {
        std::cerr << COLOR_RED << "Error: Both --schema and --input parameters are required." << COLOR_RESET << "\n\n";
        print_usage(argv[0]);
        return 2;
    }

    // Parse Schema File
    auto schema_res = schema::SchemaParser::parse_file(schema_file);
    if (auto err = std::get_if<std::string>(&schema_res)) {
        std::cerr << COLOR_RED << "Schema Load Error: " << *err << COLOR_RESET << "\n";
        return 2;
    }
    auto schema_ptr = std::get<schema::SchemaPtr>(schema_res);

    if (verbose) {
        std::cout << COLOR_CYAN << "Loaded schema successfully" << COLOR_RESET;
        if (!schema_ptr->title.empty()) std::cout << " [Title: " << schema_ptr->title << "]";
        std::cout << "\n";
    }

    // Parse Input File
    auto input_res = json::Parser::parse_file(input_file);
    if (auto err = std::get_if<json::ParseError>(&input_res)) {
        std::cerr << COLOR_RED << "JSON Input Parse Error (Line " << err->line << ", Col " << err->column << "): " << err->message << COLOR_RESET << "\n";
        return 2;
    }
    auto input_json = std::get<json::JsonValue>(input_res);

    // Perform Verification
    auto result = schema_ptr->validate(input_json);

    if (result.is_valid()) {
        std::cout << COLOR_GREEN << COLOR_BOLD << "VALID" << COLOR_RESET << ": JSON input '" << input_file << "' satisfies schema '" << schema_file << "'.\n";
        return 0;
    } else {
        std::cout << COLOR_RED << COLOR_BOLD << "INVALID" << COLOR_RESET << ": JSON input '" << input_file << "' violates schema '" << schema_file << "' with " << result.errors().size() << " error(s):\n";
        for (size_t i = 0; i < result.errors().size(); ++i) {
            const auto& err = result.errors()[i];
            std::cout << "  " << (i + 1) << ". " << COLOR_YELLOW << err.path << COLOR_RESET 
                      << " (" << COLOR_CYAN << err.keyword << COLOR_RESET << "): " << err.message << "\n";
        }
        return 1;
    }
}
