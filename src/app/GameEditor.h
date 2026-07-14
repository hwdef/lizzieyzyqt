#pragma once

#include "GameModel.h"

#include <optional>
#include <string>

namespace lizzie::app {

enum class SetupStoneEdit {
    Black,
    White,
    Empty,
};

struct GameInfoEditResult {
    bool changed = false;
    bool analysisInvalidated = false;
};

struct MoveEditResult {
    bool accepted = false;
    std::optional<lizzie::core::NodeId> nodeId;
    std::string error;
};

class GameEditor {
public:
    explicit GameEditor(lizzie::core::GameModel* model = nullptr);

    void setModel(lizzie::core::GameModel* model);

    [[nodiscard]] bool setCurrent(lizzie::core::NodeId nodeId);
    [[nodiscard]] bool selectParent();
    [[nodiscard]] bool selectFirstChild();
    [[nodiscard]] bool promoteCurrentVariation();
    [[nodiscard]] bool deleteCurrentSubtree();
    [[nodiscard]] std::optional<lizzie::core::Color> currentSideToMove(std::string* error = nullptr) const;
    [[nodiscard]] MoveEditResult tryAddMove(lizzie::core::Move move);
    [[nodiscard]] MoveEditResult tryPlayAt(lizzie::core::Point point);
    [[nodiscard]] MoveEditResult tryPass();
    [[nodiscard]] MoveEditResult tryResign();

    [[nodiscard]] std::optional<std::string> labelAt(
        lizzie::core::NodeId nodeId,
        lizzie::core::Point point) const;
    [[nodiscard]] bool setComment(lizzie::core::NodeId nodeId, std::string comment);
    [[nodiscard]] bool clearMarkupAt(lizzie::core::NodeId nodeId, lizzie::core::Point point);
    [[nodiscard]] bool setLabel(
        lizzie::core::NodeId nodeId,
        lizzie::core::Point point,
        std::string text);
    [[nodiscard]] bool toggleMark(
        lizzie::core::NodeId nodeId,
        lizzie::core::Point point,
        lizzie::core::BoardMarkShape shape);
    [[nodiscard]] bool editSetupStone(
        lizzie::core::NodeId nodeId,
        lizzie::core::Point point,
        SetupStoneEdit edit);
    [[nodiscard]] GameInfoEditResult setGameInfo(lizzie::core::GameInfo info);

private:
    lizzie::core::GameModel* model_ = nullptr;
};

}  // namespace lizzie::app
