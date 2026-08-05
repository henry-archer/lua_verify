#include <iostream>
#include "schema_verify/schema_verify.hpp"

using namespace schema_verify;

// Helper simulating reading dynamic structure from a JSON/YAML/Lua parser
Value parse_config_file_mock() {
    return Value::object({
        {"app_name", "AntigravityService"},
        {"version", "2.5.0"},
        {"workers", int64_t(8)},
        {"log_level", "DEBUG"},
        // Optional parent 'database' IS provided, but has typos/errors
        {"database", Value::object({
            {"hst", "127.0.0.1"},        // Misspelled 'host' -> Levenshtein suggestion 'host'
            {"port", int64_t(70000)},     // Out of range (max 65535)
            {"timeout", int64_t(30)}
        })},
        // Optional parent 'cache' is OMITTED entirely -> Should cause 0 errors!
        {"routes", Value::array({
            Value::object({{"path", "/api/v1/health"}, {"method", "GET"}}),
            Value::object({{"path", "/api/v1/data"}, {"method", "INVALID_VERB"}}) // Invalid enum
        })}
    });
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "   C++17 Schema Validator & Documentation Generator Demo\n";
    std::cout << "========================================================\n\n";

    // 1. Define Database Sub-Schema
    auto db_schema = Schema::object({
        {"host", Property::required(Schema::string().description("Database server hostname or IP address"))},
        {"port", Property::required(Schema::integer().range(1, 65535).description("Database port number"))},
        {"timeout", Property::optional(Schema::integer().range(1, 300).description("Connection timeout in seconds"))}
    }).description("Database configuration section");

    // 2. Define Cache Sub-Schema (Optional Parent with Required Children)
    auto cache_schema = Schema::object({
        {"redis_url", Property::required(Schema::string().description("Redis connection URI"))},
        {"ttl_seconds", Property::required(Schema::integer().range(1, 86400).description("Default cache TTL"))}
    }).description("Redis caching settings");

    // 3. Define Route Item Schema (Array Element Schema)
    auto route_schema = Schema::object({
        {"path", Property::required(Schema::string().min_len(1).description("HTTP Endpoint Path"))},
        {"method", Property::required(Schema::string().allowed({
            Value("GET"), Value("POST"), Value("PUT"), Value("DELETE"), Value("PATCH")
        }).description("HTTP Verb"))}
    });

    // 4. Define Root Master Schema
    auto root_schema = Schema::object({
        {"app_name", Property::required(Schema::string().min_len(1).description("Application Name"))},
        {"version", Property::required(Schema::string().description("Semantic version string"))},
        {"workers", Property::required(Schema::integer().range(1, 64).description("Worker thread count"))},
        {"log_level", Property::required(Schema::string().allowed({
            Value("TRACE"), Value("DEBUG"), Value("INFO"), Value("WARN"), Value("ERROR")
        }).description("Logging verbosity"))},
        {"database", Property::optional(db_schema)}, // OPTIONAL parent object!
        {"cache", Property::optional(cache_schema)},  // OPTIONAL parent object (omitted in input)!
        {"routes", Property::required(Schema::array(route_schema).min_len(1).description("Registered API routes"))}
    }, false); // allow_additional_properties = false

    // 5. Generate and Display Documentation
    std::cout << ">>> GENERATED MARKDOWN DOCUMENTATION:\n";
    std::cout << "--------------------------------------------------------\n";
    std::cout << DocGenerator::generate_markdown(root_schema, "Microservice Config Schema", "System settings parsed from JSON/YAML/Lua files.") << "\n";

    // 6. Validate Input Data
    std::cout << ">>> VALIDATING MOCK INPUT DATA...\n";
    Value input = parse_config_file_mock();

    ValidationResult result = root_schema.validate(input);

    std::cout << "Is Input Valid? " << (result.is_valid() ? "YES" : "NO") << "\n\n";
    std::cout << result.format() << "\n";

    std::cout << "--------------------------------------------------------\n";
    std::cout << "Demo Completed Successfully.\n";

    return 0;
}
