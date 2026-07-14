#pragma once

#include "BoardPosition.h"
#include "Types.h"

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace lizzie::core {

struct SetupStones {
    std::vector<Point> black;
    std::vector<Point> white;
    std::vector<Point> empty;

    [[nodiscard]] bool isEmpty() const;
};

[[nodiscard]] SetupStones normalizedSetupStones(const SetupStones& setup);

enum class BoardMarkShape {
    Circle,
    Square,
    Triangle,
    Cross,
};

struct BoardLabel {
    Point point{};
    std::string text;
};

struct BoardMark {
    Point point{};
    BoardMarkShape shape = BoardMarkShape::Circle;
};

struct GameNode {
    NodeId id = 0;
    std::optional<NodeId> parent;
    std::vector<NodeId> children;
    std::optional<Move> move;
    std::optional<Color> playerToMove;
    SetupStones setupStones;
    std::vector<BoardLabel> labels;
    std::vector<BoardMark> marks;
    std::optional<int> moveNumber;
    std::string comment;
    std::map<std::string, std::vector<std::string>> properties;
    std::optional<AnalysisSnapshot> analysis;
    std::string analysisError;
};

class GameModel {
public:
    explicit GameModel(BoardSize boardSize = BoardSize::square(19));

    [[nodiscard]] BoardSize boardSize() const;
    void setBoardSize(BoardSize boardSize);

    [[nodiscard]] const GameInfo& gameInfo() const;
    [[nodiscard]] GameInfo& gameInfo();

    [[nodiscard]] NodeId rootId() const;
    [[nodiscard]] NodeId currentId() const;
    [[nodiscard]] const GameNode& root() const;
    [[nodiscard]] GameNode& root();
    [[nodiscard]] const GameNode* findNode(NodeId id) const;
    [[nodiscard]] GameNode* findNode(NodeId id);
    [[nodiscard]] const GameNode& node(NodeId id) const;
    [[nodiscard]] GameNode& node(NodeId id);

    NodeId addChild(NodeId parent, std::optional<Move> move = std::nullopt);
    NodeId addMove(Move move);
    [[nodiscard]] bool canAddMove(const Move& move, std::string* error = nullptr) const;
    [[nodiscard]] std::optional<NodeId> tryAddMove(Move move, std::string* error = nullptr);
    bool setCurrent(NodeId id);
    bool deleteSubtree(NodeId id);
    bool promoteVariation(NodeId id);
    bool clearAnalysisAt(NodeId id);
    bool clearAnalysisFrom(NodeId id);
    void clearAllAnalysis();

    [[nodiscard]] std::vector<NodeId> pathTo(NodeId id) const;
    [[nodiscard]] BoardPosition positionAt(NodeId id, std::string* error = nullptr) const;
    [[nodiscard]] BoardPosition currentPosition(std::string* error = nullptr) const;

private:
    BoardSize boardSize_;
    GameInfo gameInfo_;
    std::unordered_map<NodeId, GameNode> nodes_;
    NodeId rootId_ = 1;
    NodeId currentId_ = 1;
    NodeId nextId_ = 2;
};

}  // namespace lizzie::core
