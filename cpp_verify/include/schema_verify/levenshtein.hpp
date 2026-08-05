#ifndef SCHEMA_VERIFY_LEVENSHTEIN_HPP
#define SCHEMA_VERIFY_LEVENSHTEIN_HPP

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <optional>

namespace schema_verify {

inline size_t levenshtein_distance(std::string_view s1, std::string_view s2) {
    const size_t m = s1.length();
    const size_t n = s2.length();

    if (m == 0) return n;
    if (n == 0) return m;

    std::vector<size_t> dp(n + 1);
    for (size_t j = 0; j <= n; ++j) {
        dp[j] = j;
    }

    for (size_t i = 1; i <= m; ++i) {
        size_t prev_diag = dp[0];
        dp[0] = i;
        for (size_t j = 1; j <= n; ++j) {
            size_t temp = dp[j];
            size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[j] = std::min({ dp[j] + 1, dp[j - 1] + 1, prev_diag + cost });
            prev_diag = temp;
        }
    }

    return dp[n];
}

inline std::optional<std::string> suggest_closest_key(
    std::string_view unknown_key,
    const std::vector<std::string>& valid_keys,
    size_t max_distance = 3)
{
    if (valid_keys.empty()) return std::nullopt;

    std::optional<std::string> best_match;
    size_t min_dist = max_distance + 1;

    for (const auto& key : valid_keys) {
        size_t dist = levenshtein_distance(unknown_key, key);
        if (dist <= max_distance && dist < min_dist) {
            min_dist = dist;
            best_match = key;
        }
    }

    return best_match;
}

} // namespace schema_verify

#endif // SCHEMA_VERIFY_LEVENSHTEIN_HPP
