#include "Types.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>

namespace lizzie::core {

namespace {

[[nodiscard]] int decodeSgfCoordinate(char value)
{
    if (value >= 'a' && value <= 'z') {
        return value - 'a';
    }
    if (value >= 'A' && value <= 'Z') {
        return 26 + value - 'A';
    }
    return -1;
}

[[nodiscard]] char encodeSgfCoordinate(int value)
{
    return value < 26 ? static_cast<char>('a' + value) : static_cast<char>('A' + value - 26);
}

[[nodiscard]] std::string lowercase(std::string_view text)
{
    std::string result(text);
    std::ranges::transform(result, result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

[[nodiscard]] std::string trimmedLowercase(std::string_view text)
{
    const auto begin = std::ranges::find_if(text, [](unsigned char ch) {
        return std::isspace(ch) == 0;
    });
    if (begin == text.end()) {
        return {};
    }
    const auto rend = std::find_if(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch) == 0;
    });
    std::string result(begin, rend.base());
    std::ranges::transform(result, result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

}  // namespace

BoardSize BoardSize::square(int size)
{
    return BoardSize{size, size};
}

bool BoardSize::isValid() const
{
    return width > 0 && height > 0 && width <= 52 && height <= 52;
}

int BoardSize::pointCount() const
{
    return width * height;
}

bool BoardSize::isSquare() const
{
    return width == height;
}

std::size_t PointHash::operator()(const Point& point) const noexcept
{
    return static_cast<std::size_t>((point.x * 1315423911u) ^ point.y);
}

Move Move::play(Color color, Point point)
{
    return Move{color, MoveType::Play, point};
}

Move Move::pass(Color color)
{
    return Move{color, MoveType::Pass, Point{}};
}

Move Move::resign(Color color)
{
    return Move{color, MoveType::Resign, Point{}};
}

bool Move::isPlay() const
{
    return type == MoveType::Play;
}

Color opponent(Color color)
{
    return color == Color::Black ? Color::White : Color::Black;
}

Stone stoneFor(Color color)
{
    return color == Color::Black ? Stone::Black : Stone::White;
}

std::optional<Color> colorFor(Stone stone)
{
    if (stone == Stone::Black) {
        return Color::Black;
    }
    if (stone == Stone::White) {
        return Color::White;
    }
    return std::nullopt;
}

std::string toString(Color color)
{
    return color == Color::Black ? "black" : "white";
}

std::string toString(Ruleset ruleset)
{
    switch (ruleset) {
    case Ruleset::Chinese:
        return "chinese";
    case Ruleset::Japanese:
        return "japanese";
    case Ruleset::TrompTaylor:
        return "tromp-taylor";
    case Ruleset::AGA:
        return "aga";
    case Ruleset::NewZealand:
        return "new-zealand";
    case Ruleset::Korean:
        return "korean";
    }
    return "chinese";
}

std::optional<Ruleset> rulesetFromString(std::string_view text)
{
    std::string normalized = trimmedLowercase(text);
    std::ranges::replace(normalized, '_', '-');
    std::ranges::replace(normalized, ' ', '-');
    if (normalized == "chinese" || normalized == "cn") {
        return Ruleset::Chinese;
    }
    if (normalized == "japanese" || normalized == "jp") {
        return Ruleset::Japanese;
    }
    if (normalized == "tromp-taylor" || normalized == "tromptaylor" || normalized == "tt") {
        return Ruleset::TrompTaylor;
    }
    if (normalized == "aga") {
        return Ruleset::AGA;
    }
    if (normalized == "new-zealand" || normalized == "newzealand" || normalized == "nz") {
        return Ruleset::NewZealand;
    }
    if (normalized == "korean" || normalized == "kr") {
        return Ruleset::Korean;
    }
    return std::nullopt;
}

std::optional<Point> pointFromSgf(std::string_view value, BoardSize boardSize)
{
    if (value.size() < 2 || !boardSize.isValid()) {
        return std::nullopt;
    }

    const int x = decodeSgfCoordinate(value[0]);
    const int y = decodeSgfCoordinate(value[1]);
    if (x < 0 || y < 0 || x >= boardSize.width || y >= boardSize.height) {
        return std::nullopt;
    }
    return Point{x, y};
}

std::string pointToSgf(Point point)
{
    return std::string{encodeSgfCoordinate(point.x), encodeSgfCoordinate(point.y)};
}

std::optional<Move> moveFromSgf(Color color, std::string_view value, BoardSize boardSize)
{
    const std::string normalized = lowercase(value);
    if (value.empty() || normalized == "pass") {
        return Move::pass(color);
    }
    if (normalized == "resign") {
        return Move::resign(color);
    }
    if (const auto point = pointFromSgf(value, boardSize)) {
        return Move::play(color, *point);
    }

    // SGF commonly encodes pass as a coordinate outside the board, for example tt on 19x19.
    return Move::pass(color);
}

std::string moveToSgf(const Move& move)
{
    switch (move.type) {
    case MoveType::Play:
        return pointToSgf(move.point);
    case MoveType::Pass:
        return "";
    case MoveType::Resign:
        return "resign";
    }
    return "";
}

}  // namespace lizzie::core
