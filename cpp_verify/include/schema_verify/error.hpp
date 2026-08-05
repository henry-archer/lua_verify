#ifndef SCHEMA_VERIFY_ERROR_HPP
#define SCHEMA_VERIFY_ERROR_HPP

#include <string>
#include <vector>
#include <optional>
#include <sstream>

namespace schema_verify {

struct ValidationError {
    std::string path;                      // Field path e.g. "server.port"
    std::string message;                   // Validation error message
    std::optional<std::string> suggestion; // Suggested fix/closest key

    std::string to_string() const {
        std::ostringstream ss;
        ss << "[" << (path.empty() ? "$" : path) << "] " << message;
        if (suggestion.has_value() && !suggestion->empty()) {
            ss << " (Did you mean '" << *suggestion << "'?)";
        }
        return ss.str();
    }
};

class ValidationResult {
private:
    std::vector<ValidationError> errors_;

public:
    ValidationResult() = default;

    void add_error(std::string path, std::string message, std::optional<std::string> suggestion = std::nullopt) {
        errors_.push_back(ValidationError{std::move(path), std::move(message), std::move(suggestion)});
    }

    bool is_valid() const {
        return errors_.empty();
    }

    const std::vector<ValidationError>& errors() const {
        return errors_;
    }

    size_t error_count() const {
        return errors_.size();
    }

    std::string format() const {
        if (is_valid()) {
            return "Validation Succeeded: No errors found.";
        }
        std::ostringstream ss;
        ss << "Validation Failed with " << errors_.size() << " error(s):\n";
        for (size_t i = 0; i < errors_.size(); ++i) {
            ss << "  " << (i + 1) << ". " << errors_[i].to_string() << "\n";
        }
        return ss.str();
    }
};

} // namespace schema_verify

#endif // SCHEMA_VERIFY_ERROR_HPP
