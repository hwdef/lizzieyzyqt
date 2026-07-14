#pragma once

#include <string>
#include <string_view>

namespace lizzie::engine {

[[nodiscard]] std::string canonicalRulesForKataGo(std::string_view rules);

}  // namespace lizzie::engine
