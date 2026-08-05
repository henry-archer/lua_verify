#ifndef BUILDER_HPP
#define BUILDER_HPP

#include "schema.hpp"
#include <memory>
#include <string>
#include <vector>
#include <initializer_list>

namespace schema {

class SchemaBuilder {
public:
    SchemaBuilder() : schema_(std::make_shared<Schema>()) {}
    explicit SchemaBuilder(SchemaPtr s) : schema_(std::move(s)) {}

    // Factory static methods
    static SchemaBuilder type(const std::string& t) {
        SchemaBuilder b;
        b.schema_->allowed_type_names.push_back(t);
        return b;
    }

    static SchemaBuilder type(const std::initializer_list<std::string>& types) {
        SchemaBuilder b;
        for (const auto& t : types) {
            b.schema_->allowed_type_names.push_back(t);
        }
        return b;
    }

    static SchemaBuilder string() { return type("string"); }
    static SchemaBuilder number() { return type("number"); }
    static SchemaBuilder integer() { return type("integer"); }
    static SchemaBuilder boolean() { return type("boolean"); }
    static SchemaBuilder nullType() { return type("null"); }
    static SchemaBuilder array() { return type("array"); }
    static SchemaBuilder object() { return type("object"); }
    static SchemaBuilder any() { return SchemaBuilder(); }

    // Metadata
    SchemaBuilder& title(std::string t) { schema_->title = std::move(t); return *this; }
    SchemaBuilder& description(std::string d) { schema_->description = std::move(d); return *this; }

    // Number constraints
    SchemaBuilder& minimum(double min_val) { schema_->minimum = min_val; return *this; }
    SchemaBuilder& maximum(double max_val) { schema_->maximum = max_val; return *this; }
    SchemaBuilder& exclusiveMinimum(double min_val) { schema_->exclusive_minimum = min_val; return *this; }
    SchemaBuilder& exclusiveMaximum(double max_val) { schema_->exclusive_maximum = max_val; return *this; }
    SchemaBuilder& multipleOf(double mult) { schema_->multiple_of = mult; return *this; }

    // String constraints
    SchemaBuilder& minLength(size_t len) { schema_->min_length = len; return *this; }
    SchemaBuilder& maxLength(size_t len) { schema_->max_length = len; return *this; }
    SchemaBuilder& pattern(std::string pat) { schema_->pattern = std::move(pat); return *this; }
    SchemaBuilder& format(std::string fmt) { schema_->format = std::move(fmt); return *this; }

    // Array constraints
    SchemaBuilder& minItems(size_t items) { schema_->min_items = items; return *this; }
    SchemaBuilder& maxItems(size_t items) { schema_->max_items = items; return *this; }
    SchemaBuilder& uniqueItems(bool unique = true) { schema_->unique_items = unique; return *this; }
    SchemaBuilder& items(const SchemaBuilder& item_builder) { schema_->items_schema = item_builder.build(); return *this; }
    SchemaBuilder& items(SchemaPtr item_schema) { schema_->items_schema = std::move(item_schema); return *this; }
    SchemaBuilder& prefixItems(const std::initializer_list<SchemaBuilder>& builders) {
        for (const auto& b : builders) {
            schema_->prefix_items.push_back(b.build());
        }
        return *this;
    }

    // Object constraints
    SchemaBuilder& property(std::string name, const SchemaBuilder& prop_builder) {
        schema_->properties[std::move(name)] = prop_builder.build();
        return *this;
    }
    SchemaBuilder& property(std::string name, SchemaPtr prop_schema) {
        schema_->properties[std::move(name)] = std::move(prop_schema);
        return *this;
    }
    SchemaBuilder& required(std::initializer_list<std::string> reqs) {
        for (const auto& r : reqs) {
            schema_->required_properties.insert(r);
        }
        return *this;
    }
    SchemaBuilder& additionalProperties(bool allow) {
        schema_->allow_additional_properties = allow;
        return *this;
    }
    SchemaBuilder& additionalProperties(const SchemaBuilder& add_builder) {
        schema_->additional_properties_schema = add_builder.build();
        return *this;
    }
    SchemaBuilder& patternProperty(std::string regex_pat, const SchemaBuilder& prop_builder) {
        schema_->pattern_properties.emplace_back(std::move(regex_pat), prop_builder.build());
        return *this;
    }
    SchemaBuilder& minProperties(size_t count) { schema_->min_properties = count; return *this; }
    SchemaBuilder& maxProperties(size_t count) { schema_->max_properties = count; return *this; }
    SchemaBuilder& dependentRequired(std::string prop, std::initializer_list<std::string> deps) {
        schema_->dependent_required[std::move(prop)] = std::vector<std::string>(deps);
        return *this;
    }

    // Enum & Const
    SchemaBuilder& enumValues(std::initializer_list<json::JsonValue> vals) {
        schema_->enum_values = std::vector<json::JsonValue>(vals);
        return *this;
    }
    SchemaBuilder& constValue(json::JsonValue val) {
        schema_->const_value = std::move(val);
        return *this;
    }

    // Combinators
    SchemaBuilder& allOf(std::initializer_list<SchemaBuilder> builders) {
        for (const auto& b : builders) {
            schema_->all_of.push_back(b.build());
        }
        return *this;
    }
    SchemaBuilder& anyOf(std::initializer_list<SchemaBuilder> builders) {
        for (const auto& b : builders) {
            schema_->any_of.push_back(b.build());
        }
        return *this;
    }
    SchemaBuilder& oneOf(std::initializer_list<SchemaBuilder> builders) {
        for (const auto& b : builders) {
            schema_->one_of.push_back(b.build());
        }
        return *this;
    }
    SchemaBuilder& notSchema(const SchemaBuilder& builder) {
        schema_->not_schema = builder.build();
        return *this;
    }

    // Custom C++ Validation callback
    SchemaBuilder& custom(CustomValidator fn) {
        schema_->custom_validators.push_back(std::move(fn));
        return *this;
    }

    SchemaPtr build() const {
        return schema_;
    }

private:
    SchemaPtr schema_;
};

} // namespace schema

#endif // BUILDER_HPP
