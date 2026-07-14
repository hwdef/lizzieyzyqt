#pragma once

#include "GameModel.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace lizzie::core {

class SgfError : public std::runtime_error {
public:
    explicit SgfError(const std::string& message);
};

class SgfParser {
public:
    [[nodiscard]] GameModel parseGame(std::string_view input) const;
    [[nodiscard]] std::vector<GameModel> parseCollection(std::string_view input) const;
};

[[nodiscard]] std::string writeSgf(const GameModel& model);
[[nodiscard]] std::string writeSgfCollection(const std::vector<GameModel>& models);

}  // namespace lizzie::core
