#include "json.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <cctype>
#include <stdexcept>
#include <algorithm>

namespace json {

Type JsonValue::type() const {
    return static_cast<Type>(data_.index());
}

std::string JsonValue::type_name() const {
    switch (type()) {
        case Type::Null: return "null";
        case Type::Boolean: return "boolean";
        case Type::Number: return "number";
        case Type::String: return "string";
        case Type::Array: return "array";
        case Type::Object: return "object";
    }
    return "unknown";
}

bool JsonValue::is_integer() const {
    if (!is_number()) return false;
    double val = std::get<NumberType>(data_);
    double int_part;
    return std::modf(val, &int_part) == 0.0;
}

bool JsonValue::as_bool() const {
    if (!is_bool()) throw std::runtime_error("JsonValue is not a boolean");
    return std::get<BoolType>(data_);
}

double JsonValue::as_number() const {
    if (!is_number()) throw std::runtime_error("JsonValue is not a number");
    return std::get<NumberType>(data_);
}

int64_t JsonValue::as_integer() const {
    if (!is_number()) throw std::runtime_error("JsonValue is not a number");
    return static_cast<int64_t>(std::get<NumberType>(data_));
}

const std::string& JsonValue::as_string() const {
    if (!is_string()) throw std::runtime_error("JsonValue is not a string");
    return std::get<StringType>(data_);
}

const ArrayType& JsonValue::as_array() const {
    if (!is_array()) throw std::runtime_error("JsonValue is not an array");
    return std::get<ArrayType>(data_);
}

const ObjectType& JsonValue::as_object() const {
    if (!is_object()) throw std::runtime_error("JsonValue is not an object");
    return std::get<ObjectType>(data_);
}

bool JsonValue::has_key(const std::string& key) const {
    if (!is_object()) return false;
    const auto& obj = std::get<ObjectType>(data_);
    for (const auto& kv : obj) {
        if (kv.first == key) return true;
    }
    return false;
}

const JsonValue* JsonValue::get(const std::string& key) const {
    if (!is_object()) return nullptr;
    const auto& obj = std::get<ObjectType>(data_);
    for (const auto& kv : obj) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

std::string JsonValue::dump(int indent, int current_indent) const {
    std::string indent_str = (indent >= 0) ? std::string(current_indent, ' ') : "";
    std::string next_indent_str = (indent >= 0) ? std::string(current_indent + indent, ' ') : "";
    std::string newline = (indent >= 0) ? "\n" : "";
    std::string space = (indent >= 0) ? " " : "";

    switch (type()) {
        case Type::Null:
            return "null";
        case Type::Boolean:
            return std::get<BoolType>(data_) ? "true" : "false";
        case Type::Number: {
            double d = std::get<NumberType>(data_);
            if (is_integer()) {
                return std::to_string(static_cast<int64_t>(d));
            }
            std::ostringstream ss;
            ss << d;
            return ss.str();
        }
        case Type::String: {
            std::string res = "\"";
            for (char c : std::get<StringType>(data_)) {
                switch (c) {
                    case '"': res += "\\\""; break;
                    case '\\': res += "\\\\"; break;
                    case '\b': res += "\\b"; break;
                    case '\f': res += "\\f"; break;
                    case '\n': res += "\\n"; break;
                    case '\r': res += "\\r"; break;
                    case '\t': res += "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20) {
                            char buf[8];
                            snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                            res += buf;
                        } else {
                            res += c;
                        }
                        break;
                }
            }
            res += "\"";
            return res;
        }
        case Type::Array: {
            const auto& arr = std::get<ArrayType>(data_);
            if (arr.empty()) return "[]";
            std::string res = "[" + newline;
            for (size_t i = 0; i < arr.size(); ++i) {
                res += next_indent_str + arr[i].dump(indent, current_indent + (indent >= 0 ? indent : 0));
                if (i + 1 < arr.size()) res += ",";
                res += newline;
            }
            res += indent_str + "]";
            return res;
        }
        case Type::Object: {
            const auto& obj = std::get<ObjectType>(data_);
            if (obj.empty()) return "{}";
            std::string res = "{" + newline;
            for (size_t i = 0; i < obj.size(); ++i) {
                JsonValue key_val(obj[i].first);
                res += next_indent_str + key_val.dump(indent, current_indent + (indent >= 0 ? indent : 0)) + ":" + space;
                res += obj[i].second.dump(indent, current_indent + (indent >= 0 ? indent : 0));
                if (i + 1 < obj.size()) res += ",";
                res += newline;
            }
            res += indent_str + "}";
            return res;
        }
    }
    return "null";
}

bool JsonValue::operator==(const JsonValue& other) const {
    if (type() != other.type()) return false;
    switch (type()) {
        case Type::Null:
            return true;
        case Type::Boolean:
            return std::get<BoolType>(data_) == std::get<BoolType>(other.data_);
        case Type::Number:
            return std::get<NumberType>(data_) == std::get<NumberType>(other.data_);
        case Type::String:
            return std::get<StringType>(data_) == std::get<StringType>(other.data_);
        case Type::Array:
            return std::get<ArrayType>(data_) == std::get<ArrayType>(other.data_);
        case Type::Object: {
            const auto& o1 = std::get<ObjectType>(data_);
            const auto& o2 = std::get<ObjectType>(other.data_);
            if (o1.size() != o2.size()) return false;
            for (const auto& kv : o1) {
                const JsonValue* val2 = other.get(kv.first);
                if (!val2 || !(kv.second == *val2)) return false;
            }
            return true;
        }
    }
    return false;
}

// Internal recursive descent parser implementation
namespace {

class InternalParser {
public:
    InternalParser(std::string_view src) : src_(src), pos_(0), line_(1), col_(1) {}

    std::variant<JsonValue, ParseError> parse() {
        skip_whitespace();
        if (pos_ >= src_.size()) {
            return make_error("Empty JSON input");
        }
        auto res = parse_value();
        if (auto err = std::get_if<ParseError>(&res)) {
            return *err;
        }
        skip_whitespace();
        if (pos_ < src_.size()) {
            return make_error("Unexpected characters after JSON root element");
        }
        return res;
    }

private:
    std::string_view src_;
    size_t pos_;
    size_t line_;
    size_t col_;

    char peek() const {
        return pos_ < src_.size() ? src_[pos_] : '\0';
    }

    char get() {
        if (pos_ >= src_.size()) return '\0';
        char c = src_[pos_++];
        if (c == '\n') {
            line_++;
            col_ = 1;
        } else {
            col_++;
        }
        return c;
    }

    void skip_whitespace() {
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                get();
            } else {
                break;
            }
        }
    }

    ParseError make_error(const std::string& msg) {
        return ParseError{msg, line_, col_};
    }

    std::variant<JsonValue, ParseError> parse_value() {
        skip_whitespace();
        char c = peek();
        if (c == 'n') return parse_null();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == '"') return parse_string();
        if (c == '[') return parse_array();
        if (c == '{') return parse_object();
        if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) return parse_number();
        return make_error(std::string("Unexpected character '") + c + "'");
    }

    std::variant<JsonValue, ParseError> parse_null() {
        if (src_.substr(pos_, 4) == "null") {
            for (int i = 0; i < 4; ++i) get();
            return JsonValue(nullptr);
        }
        return make_error("Expected 'null'");
    }

    std::variant<JsonValue, ParseError> parse_bool() {
        if (src_.substr(pos_, 4) == "true") {
            for (int i = 0; i < 4; ++i) get();
            return JsonValue(true);
        }
        if (src_.substr(pos_, 5) == "false") {
            for (int i = 0; i < 5; ++i) get();
            return JsonValue(false);
        }
        return make_error("Expected 'true' or 'false'");
    }

    std::variant<JsonValue, ParseError> parse_string() {
        if (get() != '"') return make_error("Expected string quote");
        std::string str;
        while (pos_ < src_.size()) {
            char c = get();
            if (c == '"') {
                return JsonValue(str);
            }
            if (c == '\\') {
                if (pos_ >= src_.size()) return make_error("Unterminated escape sequence in string");
                char esc = get();
                switch (esc) {
                    case '"': str += '"'; break;
                    case '\\': str += '\\'; break;
                    case '/': str += '/'; break;
                    case 'b': str += '\b'; break;
                    case 'f': str += '\f'; break;
                    case 'n': str += '\n'; break;
                    case 'r': str += '\r'; break;
                    case 't': str += '\t'; break;
                    case 'u': {
                        if (pos_ + 4 > src_.size()) return make_error("Invalid unicode escape");
                        std::string hex_code(src_.substr(pos_, 4));
                        pos_ += 4; col_ += 4;
                        try {
                            uint32_t codepoint = std::stoul(hex_code, nullptr, 16);
                            if (codepoint < 0x80) {
                                str += static_cast<char>(codepoint);
                            } else if (codepoint < 0x800) {
                                str += static_cast<char>(0xC0 | (codepoint >> 6));
                                str += static_cast<char>(0x80 | (codepoint & 0x3F));
                            } else {
                                str += static_cast<char>(0xE0 | (codepoint >> 12));
                                str += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                                str += static_cast<char>(0x80 | (codepoint & 0x3F));
                            }
                        } catch (...) {
                            return make_error("Invalid hex codepoint");
                        }
                        break;
                    }
                    default:
                        return make_error(std::string("Unknown escape sequence \\") + esc);
                }
            } else {
                str += c;
            }
        }
        return make_error("Unterminated string");
    }

    std::variant<JsonValue, ParseError> parse_number() {
        size_t start = pos_;
        if (peek() == '-') get();
        if (!std::isdigit(static_cast<unsigned char>(peek()))) {
            return make_error("Expected digit in number");
        }
        while (std::isdigit(static_cast<unsigned char>(peek()))) get();
        if (peek() == '.') {
            get();
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                return make_error("Expected digit after decimal point");
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) get();
        }
        if (peek() == 'e' || peek() == 'E') {
            get();
            if (peek() == '+' || peek() == '-') get();
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                return make_error("Expected digit in exponent");
            }
            while (std::isdigit(static_cast<unsigned char>(peek()))) get();
        }

        std::string num_str(src_.substr(start, pos_ - start));
        try {
            double val = std::stod(num_str);
            return JsonValue(val);
        } catch (...) {
            return make_error("Invalid number value: " + num_str);
        }
    }

    std::variant<JsonValue, ParseError> parse_array() {
        get(); // consume '['
        skip_whitespace();
        ArrayType arr;
        if (peek() == ']') {
            get();
            return JsonValue(arr);
        }

        while (true) {
            auto val_res = parse_value();
            if (auto err = std::get_if<ParseError>(&val_res)) return *err;
            arr.push_back(std::get<JsonValue>(val_res));

            skip_whitespace();
            char c = peek();
            if (c == ']') {
                get();
                return JsonValue(arr);
            } else if (c == ',') {
                get();
                skip_whitespace();
            } else {
                return make_error("Expected ',' or ']' in array");
            }
        }
    }

    std::variant<JsonValue, ParseError> parse_object() {
        get(); // consume '{'
        skip_whitespace();
        ObjectType obj;
        if (peek() == '}') {
            get();
            return JsonValue(obj);
        }

        while (true) {
            skip_whitespace();
            if (peek() != '"') return make_error("Expected string key in object");
            auto key_res = parse_string();
            if (auto err = std::get_if<ParseError>(&key_res)) return *err;
            std::string key = std::get<JsonValue>(key_res).as_string();

            skip_whitespace();
            if (peek() != ':') return make_error("Expected ':' after key in object");
            get();

            auto val_res = parse_value();
            if (auto err = std::get_if<ParseError>(&val_res)) return *err;
            obj.emplace_back(std::move(key), std::get<JsonValue>(val_res));

            skip_whitespace();
            char c = peek();
            if (c == '}') {
                get();
                return JsonValue(obj);
            } else if (c == ',') {
                get();
                skip_whitespace();
            } else {
                return make_error("Expected ',' or '}' in object");
            }
        }
    }
};

} // anonymous namespace

std::variant<JsonValue, ParseError> Parser::parse(std::string_view json_str) {
    InternalParser p(json_str);
    return p.parse();
}

std::variant<JsonValue, ParseError> Parser::parse_file(const std::string& filepath) {
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        return ParseError{"Could not open file: " + filepath, 0, 0};
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    return parse(buffer.str());
}

} // namespace json
