#pragma once

#include "Types.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace lizzie::core {

struct PlayResult {
    bool ok = false;
    std::vector<Point> captured;
    std::string error;
};

class BoardPosition {
public:
    explicit BoardPosition(BoardSize size = BoardSize::square(19));

    [[nodiscard]] BoardSize size() const;
    [[nodiscard]] bool isOnBoard(Point point) const;
    [[nodiscard]] Stone at(Point point) const;
    [[nodiscard]] int blackCaptures() const;
    [[nodiscard]] int whiteCaptures() const;
    [[nodiscard]] std::optional<Point> ko() const;
    [[nodiscard]] Color sideToMove() const;
    [[nodiscard]] std::uint64_t zobrist() const;
    [[nodiscard]] std::vector<Point> neighbors(Point point) const;
    [[nodiscard]] bool canPlay(Color color, Point point, std::string* error = nullptr) const;

    PlayResult playMove(const Move& move);
    void clear();
    void setSideToMove(Color color);
    void placeSetupStone(Color color, Point point);
    void clearPoint(Point point);

private:
    [[nodiscard]] int index(Point point) const;
    [[nodiscard]] std::vector<Point> collectGroup(Point start) const;
    [[nodiscard]] bool hasLiberty(const std::vector<Point>& group) const;
    [[nodiscard]] int countLiberties(const std::vector<Point>& group) const;
    void removeGroup(const std::vector<Point>& group, std::vector<Point>* captured);
    void updateHash();

    BoardSize size_;
    std::vector<Stone> stones_;
    int blackCaptures_ = 0;
    int whiteCaptures_ = 0;
    std::optional<Point> ko_;
    Color sideToMove_ = Color::Black;
    std::uint64_t zobrist_ = 0;
};

}  // namespace lizzie::core
