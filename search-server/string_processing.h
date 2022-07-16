#pragma once
#include <vector>
#include <string>
#include <set>
#include <string_view>

std::vector<std::string> SplitIntoWords(const std::string& text);
std::vector<std::string_view> SplitIntoWordsView(std::string_view str);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string_view str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(std::string(str));
        }
    }
    return non_empty_strings;
}
