#include "GameSetup.h"

#include <utility>

namespace lizzie::app {

std::vector<lizzie::core::Point> standardHandicapStones(
    lizzie::core::BoardSize boardSize,
    int handicap)
{
    if (!boardSize.isSquare() || boardSize.width < 7 || handicap < 2 || handicap > 9) {
        return {};
    }

    const int low = boardSize.width < 13 ? 2 : 3;
    const int high = boardSize.width - 1 - low;
    const int middle = boardSize.width / 2;
    if (low >= high) {
        return {};
    }

    const lizzie::core::Point lowerLeft{low, high};
    const lizzie::core::Point upperRight{high, low};
    const lizzie::core::Point lowerRight{high, high};
    const lizzie::core::Point upperLeft{low, low};
    const lizzie::core::Point center{middle, middle};
    const lizzie::core::Point leftSide{low, middle};
    const lizzie::core::Point rightSide{high, middle};
    const lizzie::core::Point upperSide{middle, low};
    const lizzie::core::Point lowerSide{middle, high};

    std::vector<lizzie::core::Point> points{lowerLeft, upperRight};
    if (handicap >= 3) {
        points.push_back(lowerRight);
    }
    if (handicap >= 4) {
        points.push_back(upperLeft);
    }
    if (handicap == 5 || handicap == 7 || handicap == 9) {
        points.push_back(center);
    }
    if (handicap >= 6) {
        points.push_back(leftSide);
        points.push_back(rightSide);
    }
    if (handicap >= 8) {
        points.push_back(upperSide);
        points.push_back(lowerSide);
    }
    return points;
}

lizzie::core::GameModel createNewGameModel(
    lizzie::core::BoardSize boardSize,
    lizzie::core::GameInfo info)
{
    std::vector<lizzie::core::Point> handicapStones;
    if (info.handicap.has_value()) {
        handicapStones = standardHandicapStones(boardSize, *info.handicap);
        if (handicapStones.empty()) {
            info.handicap.reset();
        }
    }

    lizzie::core::GameModel model(boardSize);
    model.gameInfo() = std::move(info);
    model.root().setupStones.black = std::move(handicapStones);
    return model;
}

}  // namespace lizzie::app
