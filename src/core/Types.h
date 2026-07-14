#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lizzie::core {

enum class Color {
    Black,
    White,
};

enum class Stone {
    Empty,
    Black,
    White,
};

enum class MoveType {
    Play,
    Pass,
    Resign,
};

enum class Ruleset {
    Chinese,
    Japanese,
    TrompTaylor,
    AGA,
    NewZealand,
    Korean,
};

struct BoardSize {
    int width = 19;
    int height = 19;

    [[nodiscard]] static BoardSize square(int size);
    [[nodiscard]] bool isValid() const;
    [[nodiscard]] int pointCount() const;
    [[nodiscard]] bool isSquare() const;
};

struct Point {
    int x = 0;
    int y = 0;

    [[nodiscard]] bool operator==(const Point& other) const = default;
};

struct PointHash {
    [[nodiscard]] std::size_t operator()(const Point& point) const noexcept;
};

struct Move {
    Color color = Color::Black;
    MoveType type = MoveType::Play;
    Point point{};

    [[nodiscard]] static Move play(Color color, Point point);
    [[nodiscard]] static Move pass(Color color);
    [[nodiscard]] static Move resign(Color color);
    [[nodiscard]] bool isPlay() const;
};

struct GameInfo {
    std::string blackPlayer;
    std::string whitePlayer;
    std::string blackRank;
    std::string whiteRank;
    std::string result;
    std::string date;
    std::string source;
    std::string rules;
    std::optional<double> komi;
    std::optional<int> handicap;
};

struct MoveCandidate {
    Move move;
    int visits = 0;
    double winrate = 0.0;
    double scoreMean = 0.0;
    double scoreStdev = 0.0;
    double policy = 0.0;
    std::vector<Move> pv;
    std::vector<int> pvVisits;
    std::vector<double> ownership;
};

struct AnalysisSnapshot {
    std::vector<MoveCandidate> candidates;
    std::vector<double> ownership;
    double rootWinrate = 0.0;
    double rootScoreMean = 0.0;
    int rootVisits = 0;
    // KataGo analysis values are reported from this player when the protocol does not force Black.
    std::optional<Color> winratePerspective;
};

using NodeId = std::uint64_t;

[[nodiscard]] Color opponent(Color color);
[[nodiscard]] Stone stoneFor(Color color);
[[nodiscard]] std::optional<Color> colorFor(Stone stone);
[[nodiscard]] std::string toString(Color color);
[[nodiscard]] std::string toString(Ruleset ruleset);
[[nodiscard]] std::optional<Ruleset> rulesetFromString(std::string_view text);
[[nodiscard]] std::optional<Point> pointFromSgf(std::string_view value, BoardSize boardSize);
[[nodiscard]] std::string pointToSgf(Point point);
[[nodiscard]] std::optional<Move> moveFromSgf(Color color, std::string_view value, BoardSize boardSize);
[[nodiscard]] std::string moveToSgf(const Move& move);

}  // namespace lizzie::core
