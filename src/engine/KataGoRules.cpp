#include "KataGoRules.h"

#include "Types.h"

#include <algorithm>
#include <cctype>
#include <optional>

namespace lizzie::engine {

namespace {

[[nodiscard]] std::string trimmed(std::string_view text)
{
    const auto begin = std::ranges::find_if(text, [](unsigned char ch) {
        return std::isspace(ch) == 0;
    });
    const auto rend = std::find_if(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch) == 0;
    });
    if (begin == text.end() || rend == text.rend()) {
        return {};
    }
    return std::string(begin, rend.base());
}

}  // namespace

std::string canonicalRulesForKataGo(std::string_view rules)
{
    const std::string value = trimmed(rules);
    if (const std::optional<lizzie::core::Ruleset> parsed = lizzie::core::rulesetFromString(value)) {
        return lizzie::core::toString(*parsed);
    }
    return value;
}

}  // namespace lizzie::engine
