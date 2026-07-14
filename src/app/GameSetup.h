#pragma once

#include "GameModel.h"

#include <vector>

namespace lizzie::app {

[[nodiscard]] std::vector<lizzie::core::Point> standardHandicapStones(
    lizzie::core::BoardSize boardSize,
    int handicap);

[[nodiscard]] lizzie::core::GameModel createNewGameModel(
    lizzie::core::BoardSize boardSize,
    lizzie::core::GameInfo info);

}  // namespace lizzie::app
