#include <iostream>
#include <cassert>
#include <string>
#include <vector>

#include "schema_verify/schema_verify.hpp"

using namespace schema_verify;

void test_type_validation() {
    std::cout << "[Test] Type Validation..." << std::endl;

    auto schema = Schema::object({
        {"is_active", Property::required(Schema::boolean())},
        {"age", Property::required(Schema::integer())},
        {"score", Property::required(Schema::number())},
        {"name", Property::required(Schema::string())}
    });

    Value valid_input = Value::object({
        {"is_active", true},
        {"age", int64_t(30)},
        {"score", 98.5},
        {"name", "Alice"}
    });

    auto res1 = schema.validate(valid_input);
    assert(res1.is_valid());

    Value invalid_input = Value::object({
        {"is_active", "yes"}, // String instead of Bool
        {"age", 30.5},       // Float instead of Int
        {"score", "high"},   // String instead of Number
        {"name", 12345}      // Int instead of String
    });

    auto res2 = schema.validate(invalid_input);
    assert(!res2.is_valid());
    assert(res2.error_count() == 4);

    std::cout << "  ✓ Passed (" << res2.error_count() << " type errors gathered correctly)" << std::endl;
}

void test_list_element_validation() {
    std::cout << "[Test] List Element Validation..." << std::endl;

    auto item_schema = Schema::object({
        {"id", Property::required(Schema::integer())},
        {"role", Property::required(Schema::string().allowed({Value("admin"), Value("user")}))}
    });

    auto schema = Schema::object({
        {"users", Property::required(Schema::array(item_schema).min_len(1))}
    });

    Value valid_input = Value::object({
        {"users", Value::array({
            Value::object({{"id", 1}, {"role", "admin"}}),
            Value::object({{"id", 2}, {"role", "user"}})
        })}
    });

    assert(schema.validate(valid_input).is_valid());

    Value invalid_input = Value::object({
        {"users", Value::array({
            Value::object({{"id", 1}, {"role", "admin"}}),
            Value::object({{"id", "two"}, {"role", "superuser"}}) // Error at users[1].id and users[1].role
        })}
    });

    auto res = schema.validate(invalid_input);
    assert(!res.is_valid());
    assert(res.error_count() == 2);
    assert(res.errors()[0].path == "users[1].id");
    assert(res.errors()[1].path == "users[1].role");

    std::cout << "  ✓ Passed (Array element paths correctly identified: users[1].id, users[1].role)" << std::endl;
}

void test_optional_parent_required_children() {
    std::cout << "[Test] Optional Parent and Required Children..." << std::endl;

    auto db_schema = Schema::object({
        {"host", Property::required(Schema::string())},
        {"port", Property::required(Schema::integer().range(1, 65535))}
    });

    auto app_schema = Schema::object({
        {"app_name", Property::required(Schema::string())},
        {"database", Property::optional(db_schema)} // OPTIONAL PARENT
    });

    // Case A: Optional parent 'database' is completely omitted -> Should pass!
    Value input_no_db = Value::object({
        {"app_name", "MyApp"}
    });
    auto res_a = app_schema.validate(input_no_db);
    assert(res_a.is_valid());

    // Case B: Optional parent 'database' IS present, but missing required child 'port' -> Should fail!
    Value input_missing_child = Value::object({
        {"app_name", "MyApp"},
        {"database", Value::object({
            {"host", "localhost"} // missing required 'port'
        })}
    });
    auto res_b = app_schema.validate(input_missing_child);
    assert(!res_b.is_valid());
    assert(res_b.error_count() == 1);
    assert(res_b.errors()[0].path == "database.port");

    // Case C: Optional parent 'database' IS present with all required children -> Should pass!
    Value input_full = Value::object({
        {"app_name", "MyApp"},
        {"database", Value::object({
            {"host", "localhost"},
            {"port", 5432}
        })}
    });
    auto res_c = app_schema.validate(input_full);
    assert(res_c.is_valid());

    std::cout << "  ✓ Passed (Optional parent omitted -> OK, present with missing child -> Error)" << std::endl;
}

void test_levenshtein_suggestions() {
    std::cout << "[Test] Levenshtein Key Suggestions..." << std::endl;

    auto schema = Schema::object({
        {"username", Property::required(Schema::string())},
        {"email_address", Property::required(Schema::string())},
        {"timeout_seconds", Property::optional(Schema::integer())}
    }, false); // allow_additional = false

    Value input = Value::object({
        {"usrname", "alice"},           // Typo for username
        {"email_adress", "a@b.com"}    // Typo for email_address
    });

    auto res = schema.validate(input);
    assert(!res.is_valid());

    // Missing 'username', missing 'email_address', plus unrecognised 'usrname' and 'email_adress'
    bool found_username_suggestion = false;
    bool found_email_suggestion = false;

    for (const auto& err : res.errors()) {
        if (err.suggestion.has_value()) {
            if (*err.suggestion == "username") found_username_suggestion = true;
            if (*err.suggestion == "email_address") found_email_suggestion = true;
        }
    }

    assert(found_username_suggestion);
    assert(found_email_suggestion);

    std::cout << "  ✓ Passed (Suggested 'username' for 'usrname' and 'email_address' for 'email_adress')" << std::endl;
}

void test_gather_all_errors() {
    std::cout << "[Test] Gathering All Validation Errors..." << std::endl;

    auto schema = Schema::object({
        {"req_str", Property::required(Schema::string().min_len(5))},
        {"req_num", Property::required(Schema::number().range(10, 100))},
        {"enum_val", Property::required(Schema::string().allowed({Value("A"), Value("B")}))}
    }, false);

    Value bad_input = Value::object({
        {"req_str", "abc"},          // Error 1: String length < 5
        {"req_num", 5},              // Error 2: Number < 10
        {"enum_val", "C"},           // Error 3: Enum not allowed
        {"unknown_key", "whatever"}  // Error 4: Unrecognised key
        // Error 5: (none missing here, but 4 distinct errors)
    });

    auto res = schema.validate(bad_input);
    assert(!res.is_valid());
    assert(res.error_count() == 4);

    std::cout << "  ✓ Passed (Gathered all " << res.error_count() << " errors without aborting early)" << std::endl;
}

void test_doc_generator() {
    std::cout << "[Test] Document Generator..." << std::endl;

    auto db_schema = Schema::object({
        {"host", Property::required(Schema::string().description("Database hostname or IP"))},
        {"port", Property::required(Schema::integer().range(1, 65535).description("Database port"))}
    }).description("Database connection settings");

    auto root_schema = Schema::object({
        {"app_name", Property::required(Schema::string().min_len(1).description("Name of the application"))},
        {"environment", Property::required(Schema::string().allowed({Value("dev"), Value("prod")}).description("Deployment environment"))},
        {"database", Property::optional(db_schema)}
    });

    std::string markdown = DocGenerator::generate_markdown(root_schema, "App Config Schema", "Configuration specification for App service");
    std::string text = DocGenerator::generate_text(root_schema, "App Config Schema", "Configuration specification for App service");

    assert(!markdown.empty());
    assert(!text.empty());
    assert(markdown.find("# App Config Schema") != std::string::npos);
    assert(markdown.find("`database.host`") != std::string::npos);
    assert(text.find("Field: database.port") != std::string::npos);

    std::cout << "  ✓ Passed (Markdown & Text docs generated cleanly)" << std::endl;
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   RUNNING SCHEMA_VERIFY COMPREHENSIVE TEST SUITE  " << std::endl;
    std::cout << "==================================================" << std::endl;

    test_type_validation();
    test_list_element_validation();
    test_optional_parent_required_children();
    test_levenshtein_suggestions();
    test_gather_all_errors();
    test_doc_generator();

    std::cout << "==================================================" << std::endl;
    std::cout << "   ALL TESTS PASSED SUCCESSFULLY!                 " << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}
