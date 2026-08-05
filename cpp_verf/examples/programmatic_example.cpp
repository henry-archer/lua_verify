#include <iostream>
#include "../include/json.hpp"
#include "../include/schema.hpp"
#include "../include/builder.hpp"

int main() {
    std::cout << "--- Programmatic Schema Definition in C++17 ---\n";

    // Define a product schema programmatically
    auto product_schema = schema::SchemaBuilder::object()
        .title("ProductSchema")
        .property("id", schema::SchemaBuilder::integer().minimum(1))
        .property("name", schema::SchemaBuilder::string().minLength(2))
        .property("price", schema::SchemaBuilder::number().exclusiveMinimum(0.0))
        .property("categories", schema::SchemaBuilder::array().items(schema::SchemaBuilder::string()).minItems(1))
        .required({"id", "name", "price"})
        // Custom C++ validator rule
        .custom([](const json::JsonValue& val, const std::string& path) -> std::optional<schema::ValidationError> {
            if (val.is_object() && val.has_key("price")) {
                double price = val.get("price")->as_number();
                if (price > 10000.0) {
                    return schema::ValidationError{path + "/price", "luxury_tax", "Products over 10,000 require executive approval mark"};
                }
            }
            return std::nullopt;
        })
        .build();

    // Verify a sample JSON string
    std::string json_data = R"({
        "id": 42,
        "name": "Supercomputer",
        "price": 15000.0,
        "categories": ["Hardware", "Compute"]
    })";

    auto parsed = json::Parser::parse(json_data);
    if (auto node = std::get_if<json::JsonValue>(&parsed)) {
        auto result = product_schema->validate(*node);
        if (result.is_valid()) {
            std::cout << "Product JSON is VALID!\n";
        } else {
            std::cout << "Product JSON is INVALID:\n";
            for (const auto& err : result.errors()) {
                std::cout << " - " << err.to_string() << "\n";
            }
        }
    }

    return 0;
}
