#include "BoardPosition.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <unordered_set>

namespace lizzie::core {

namespace {

constexpr std::uint64_t kFnvOffset = 1469598103934665603ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;

void hashAppend(std::uint64_t* hash, std::uint64_t value)
{
    *hash ^= value;
    *hash *= kFnvPrime;
}

}  // namespace

BoardPosition::BoardPosition(BoardSize size)
    : size_(size)
    , stones_(static_cast<std::size_t>(size.pointCount()), Stone::Empty)
{
    if (!size_.isValid()) {
        throw std::invalid_argument("invalid board size");
    }
    updateHash();
}

BoardSize BoardPosition::size() const
{
    return size_;
}

bool BoardPosition::isOnBoard(Point point) const
{
    return point.x >= 0 && point.y >= 0 && point.x < size_.width && point.y < size_.height;
}

Stone BoardPosition::at(Point point) const
{
    if (!isOnBoard(point)) {
        throw std::out_of_range("point is outside the board");
    }
    return stones_[static_cast<std::size_t>(index(point))];
}

int BoardPosition::blackCaptures() const
{
    return blackCaptures_;
}

int BoardPosition::whiteCaptures() const
{
    return whiteCaptures_;
}

std::optional<Point> BoardPosition::ko() const
{
    return ko_;
}

Color BoardPosition::sideToMove() const
{
    return sideToMove_;
}

std::uint64_t BoardPosition::zobrist() const
{
    return zobrist_;
}

std::vector<Point> BoardPosition::neighbors(Point point) const
{
    static constexpr std::array<Point, 4> kOffsets = {
        Point{-1, 0},
        Point{1, 0},
        Point{0, -1},
        Point{0, 1},
    };

    std::vector<Point> result;
    result.reserve(4);
    for (const Point offset : kOffsets) {
        Point next{point.x + offset.x, point.y + offset.y};
        if (isOnBoard(next)) {
            result.push_back(next);
        }
    }
    return result;
}

bool BoardPosition::canPlay(Color color, Point point, std::string* error) const
{
    BoardPosition copy = *this;
    const PlayResult result = copy.playMove(Move::play(color, point));
    if (!result.ok && error != nullptr) {
        *error = result.error;
    }
    return result.ok;
}

PlayResult BoardPosition::playMove(const Move& move)
{
    if (move.type == MoveType::Pass || move.type == MoveType::Resign) {
        ko_ = std::nullopt;
        sideToMove_ = opponent(move.color);
        updateHash();
        return PlayResult{true, {}, {}};
    }

    if (!isOnBoard(move.point)) {
        return PlayResult{false, {}, "move is outside the board"};
    }
    if (at(move.point) != Stone::Empty) {
        return PlayResult{false, {}, "point is already occupied"};
    }
    if (ko_.has_value() && *ko_ == move.point) {
        return PlayResult{false, {}, "ko recapture is not legal"};
    }

    const BoardPosition before = *this;
    stones_[static_cast<std::size_t>(index(move.point))] = stoneFor(move.color);

    std::vector<Point> captured;
    const Stone opponentStone = stoneFor(opponent(move.color));
    for (Point neighbor : neighbors(move.point)) {
        if (at(neighbor) == opponentStone) {
            const std::vector<Point> group = collectGroup(neighbor);
            if (!hasLiberty(group)) {
                removeGroup(group, &captured);
            }
        }
    }

    const std::vector<Point> ownGroup = collectGroup(move.point);
    if (!hasLiberty(ownGroup)) {
        *this = before;
        return PlayResult{false, {}, "suicide is not legal"};
    }

    if (move.color == Color::Black) {
        blackCaptures_ += static_cast<int>(captured.size());
    } else {
        whiteCaptures_ += static_cast<int>(captured.size());
    }

    if (captured.size() == 1 && ownGroup.size() == 1 && countLiberties(ownGroup) == 1) {
        ko_ = captured.front();
    } else {
        ko_ = std::nullopt;
    }
    sideToMove_ = opponent(move.color);
    updateHash();
    return PlayResult{true, captured, {}};
}

void BoardPosition::clear()
{
    std::ranges::fill(stones_, Stone::Empty);
    blackCaptures_ = 0;
    whiteCaptures_ = 0;
    ko_ = std::nullopt;
    sideToMove_ = Color::Black;
    updateHash();
}

void BoardPosition::setSideToMove(Color color)
{
    sideToMove_ = color;
    updateHash();
}

void BoardPosition::placeSetupStone(Color color, Point point)
{
    if (!isOnBoard(point)) {
        throw std::out_of_range("setup stone is outside the board");
    }
    stones_[static_cast<std::size_t>(index(point))] = stoneFor(color);
    ko_ = std::nullopt;
    updateHash();
}

void BoardPosition::clearPoint(Point point)
{
    if (!isOnBoard(point)) {
        throw std::out_of_range("setup clear point is outside the board");
    }
    stones_[static_cast<std::size_t>(index(point))] = Stone::Empty;
    ko_ = std::nullopt;
    updateHash();
}

int BoardPosition::index(Point point) const
{
    return point.y * size_.width + point.x;
}

std::vector<Point> BoardPosition::collectGroup(Point start) const
{
    const Stone color = at(start);
    if (color == Stone::Empty) {
        return {};
    }

    std::vector<Point> group;
    std::vector<Point> stack{start};
    std::vector<bool> visited(static_cast<std::size_t>(size_.pointCount()), false);
    visited[static_cast<std::size_t>(index(start))] = true;

    while (!stack.empty()) {
        Point point = stack.back();
        stack.pop_back();
        group.push_back(point);

        for (Point neighbor : neighbors(point)) {
            const int neighborIndex = index(neighbor);
            if (!visited[static_cast<std::size_t>(neighborIndex)] && at(neighbor) == color) {
                visited[static_cast<std::size_t>(neighborIndex)] = true;
                stack.push_back(neighbor);
            }
        }
    }
    return group;
}

bool BoardPosition::hasLiberty(const std::vector<Point>& group) const
{
    for (Point point : group) {
        for (Point neighbor : neighbors(point)) {
            if (at(neighbor) == Stone::Empty) {
                return true;
            }
        }
    }
    return false;
}

int BoardPosition::countLiberties(const std::vector<Point>& group) const
{
    std::unordered_set<int> liberties;
    for (Point point : group) {
        for (Point neighbor : neighbors(point)) {
            if (at(neighbor) == Stone::Empty) {
                liberties.insert(index(neighbor));
            }
        }
    }
    return static_cast<int>(liberties.size());
}

void BoardPosition::removeGroup(const std::vector<Point>& group, std::vector<Point>* captured)
{
    for (Point point : group) {
        stones_[static_cast<std::size_t>(index(point))] = Stone::Empty;
        if (captured != nullptr) {
            captured->push_back(point);
        }
    }
}

void BoardPosition::updateHash()
{
    std::uint64_t hash = kFnvOffset;
    hashAppend(&hash, static_cast<std::uint64_t>(size_.width));
    hashAppend(&hash, static_cast<std::uint64_t>(size_.height));
    hashAppend(&hash, sideToMove_ == Color::Black ? 1u : 2u);
    hashAppend(&hash, static_cast<std::uint64_t>(blackCaptures_));
    hashAppend(&hash, static_cast<std::uint64_t>(whiteCaptures_));
    hashAppend(&hash, ko_.has_value() ? static_cast<std::uint64_t>(index(*ko_) + 1) : 0u);
    for (Stone stone : stones_) {
        hashAppend(&hash, static_cast<std::uint64_t>(stone));
    }
    zobrist_ = hash;
}

}  // namespace lizzie::core
