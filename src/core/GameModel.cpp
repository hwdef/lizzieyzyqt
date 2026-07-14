#include "GameModel.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>

namespace lizzie::core {

namespace {

[[nodiscard]] bool shouldInferHandicapWhiteToMove(
    const GameInfo& gameInfo,
    const GameNode& node,
    NodeId nodeId,
    NodeId rootId)
{
    return nodeId == rootId && !node.playerToMove.has_value() && !node.move.has_value() &&
        gameInfo.handicap.value_or(0) > 1 && !node.setupStones.black.empty();
}

bool asciiCaseEquals(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }
    return std::equal(left.begin(), left.end(), right.begin(), right.end(), [](char leftChar, char rightChar) {
        return std::tolower(static_cast<unsigned char>(leftChar)) ==
            std::tolower(static_cast<unsigned char>(rightChar));
    });
}

void erasePropertyCaseInsensitive(GameNode* node, std::string_view name)
{
    for (auto iter = node->properties.begin(); iter != node->properties.end();) {
        if (asciiCaseEquals(iter->first, name)) {
            iter = node->properties.erase(iter);
        } else {
            ++iter;
        }
    }
}

bool sameMove(const Move& left, const Move& right)
{
    return left.color == right.color &&
        left.type == right.type &&
        (left.type != MoveType::Play || left.point == right.point);
}

void clearNodeAnalysis(GameNode* node)
{
    node->analysis.reset();
    node->analysisError.clear();
    erasePropertyCaseInsensitive(node, "LZ");
    erasePropertyCaseInsensitive(node, "LZOP");
    erasePropertyCaseInsensitive(node, "LZYERR");
    erasePropertyCaseInsensitive(node, "LZYPERSP");
}

bool erasePoint(std::vector<Point>* points, Point point)
{
    const auto oldSize = points->size();
    points->erase(
        std::remove_if(points->begin(), points->end(), [point](Point existing) {
            return existing == point;
        }),
        points->end());
    return points->size() != oldSize;
}

void addUniquePoint(std::vector<Point>* points, Point point)
{
    if (std::ranges::find(*points, point) == points->end()) {
    points->push_back(point);
    }
}

bool setupPointIsOnBoard(const BoardPosition& position, Point point, const char* message, std::string* error)
{
    if (position.isOnBoard(point)) {
        return true;
    }
    if (error != nullptr) {
        *error = message;
    }
    return false;
}

}  // namespace

bool SetupStones::isEmpty() const
{
    const SetupStones normalized = normalizedSetupStones(*this);
    return normalized.black.empty() && normalized.white.empty() && normalized.empty.empty();
}

SetupStones normalizedSetupStones(const SetupStones& setup)
{
    SetupStones normalized;
    for (Point point : setup.black) {
        addUniquePoint(&normalized.black, point);
    }
    for (Point point : setup.white) {
        erasePoint(&normalized.black, point);
        addUniquePoint(&normalized.white, point);
    }
    for (Point point : setup.empty) {
        erasePoint(&normalized.black, point);
        erasePoint(&normalized.white, point);
        addUniquePoint(&normalized.empty, point);
    }
    return normalized;
}

GameModel::GameModel(BoardSize boardSize)
    : boardSize_(boardSize)
{
    if (!boardSize_.isValid()) {
        throw std::invalid_argument("invalid board size");
    }
    nodes_.emplace(rootId_, GameNode{rootId_, std::nullopt});
}

BoardSize GameModel::boardSize() const
{
    return boardSize_;
}

void GameModel::setBoardSize(BoardSize boardSize)
{
    if (!boardSize.isValid()) {
        throw std::invalid_argument("invalid board size");
    }
    boardSize_ = boardSize;
}

const GameInfo& GameModel::gameInfo() const
{
    return gameInfo_;
}

GameInfo& GameModel::gameInfo()
{
    return gameInfo_;
}

NodeId GameModel::rootId() const
{
    return rootId_;
}

NodeId GameModel::currentId() const
{
    return currentId_;
}

const GameNode& GameModel::root() const
{
    return node(rootId_);
}

GameNode& GameModel::root()
{
    return node(rootId_);
}

const GameNode* GameModel::findNode(NodeId id) const
{
    const auto iter = nodes_.find(id);
    return iter == nodes_.end() ? nullptr : &iter->second;
}

GameNode* GameModel::findNode(NodeId id)
{
    const auto iter = nodes_.find(id);
    return iter == nodes_.end() ? nullptr : &iter->second;
}

const GameNode& GameModel::node(NodeId id) const
{
    const GameNode* result = findNode(id);
    if (result == nullptr) {
        throw std::out_of_range("game node does not exist");
    }
    return *result;
}

GameNode& GameModel::node(NodeId id)
{
    GameNode* result = findNode(id);
    if (result == nullptr) {
        throw std::out_of_range("game node does not exist");
    }
    return *result;
}

NodeId GameModel::addChild(NodeId parent, std::optional<Move> move)
{
    if (findNode(parent) == nullptr) {
        throw std::out_of_range("parent game node does not exist");
    }

    const NodeId id = nextId_++;
    GameNode node;
    node.id = id;
    node.parent = parent;
    node.move = move;
    nodes_.emplace(id, std::move(node));
    nodes_.at(parent).children.push_back(id);
    return id;
}

NodeId GameModel::addMove(Move move)
{
    const NodeId id = addChild(currentId_, move);
    currentId_ = id;
    return id;
}

bool GameModel::canAddMove(const Move& move, std::string* error) const
{
    std::string positionError;
    BoardPosition position = currentPosition(&positionError);
    if (!positionError.empty()) {
        if (error != nullptr) {
            *error = positionError;
        }
        return false;
    }
    if (move.color != position.sideToMove()) {
        if (error != nullptr) {
            *error = "wrong player to move";
        }
        return false;
    }

    const PlayResult result = position.playMove(move);
    if (!result.ok) {
        if (error != nullptr) {
            *error = result.error;
        }
        return false;
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

std::optional<NodeId> GameModel::tryAddMove(Move move, std::string* error)
{
    if (!canAddMove(move, error)) {
        return std::nullopt;
    }
    const GameNode& current = node(currentId_);
    for (NodeId childId : current.children) {
        const GameNode& child = node(childId);
        if (child.move.has_value() && sameMove(*child.move, move)) {
            currentId_ = childId;
            if (error != nullptr) {
                error->clear();
            }
            return childId;
        }
    }
    return addMove(move);
}

bool GameModel::setCurrent(NodeId id)
{
    if (findNode(id) == nullptr) {
        return false;
    }
    currentId_ = id;
    return true;
}

bool GameModel::deleteSubtree(NodeId id)
{
    GameNode* target = findNode(id);
    if (target == nullptr || !target->parent.has_value()) {
        return false;
    }

    const NodeId parentId = *target->parent;
    std::vector<NodeId> toDelete;
    toDelete.push_back(id);
    for (std::size_t index = 0; index < toDelete.size(); ++index) {
        const GameNode& nodeToDelete = node(toDelete[index]);
        toDelete.insert(toDelete.end(), nodeToDelete.children.begin(), nodeToDelete.children.end());
    }

    std::vector<NodeId>& siblings = node(parentId).children;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), id), siblings.end());

    if (std::ranges::find(toDelete, currentId_) != toDelete.end()) {
        currentId_ = parentId;
    }
    for (NodeId nodeId : toDelete) {
        nodes_.erase(nodeId);
    }
    return true;
}

bool GameModel::promoteVariation(NodeId id)
{
    const GameNode* target = findNode(id);
    if (target == nullptr || !target->parent.has_value()) {
        return false;
    }

    std::vector<NodeId>& siblings = node(*target->parent).children;
    const auto found = std::ranges::find(siblings, id);
    if (found == siblings.end()) {
        return false;
    }
    std::rotate(siblings.begin(), found, found + 1);
    return true;
}

bool GameModel::clearAnalysisAt(NodeId id)
{
    GameNode* target = findNode(id);
    if (target == nullptr) {
        return false;
    }
    clearNodeAnalysis(target);
    return true;
}

bool GameModel::clearAnalysisFrom(NodeId id)
{
    if (findNode(id) == nullptr) {
        return false;
    }

    std::vector<NodeId> toClear{id};
    for (std::size_t index = 0; index < toClear.size(); ++index) {
        GameNode& nodeToClear = node(toClear[index]);
        clearNodeAnalysis(&nodeToClear);
        toClear.insert(toClear.end(), nodeToClear.children.begin(), nodeToClear.children.end());
    }
    return true;
}

void GameModel::clearAllAnalysis()
{
    static_cast<void>(clearAnalysisFrom(rootId_));
}

std::vector<NodeId> GameModel::pathTo(NodeId id) const
{
    if (findNode(id) == nullptr) {
        return {};
    }

    std::vector<NodeId> path;
    for (std::optional<NodeId> cursor = id; cursor.has_value();) {
        const GameNode& current = node(*cursor);
        path.push_back(current.id);
        cursor = current.parent;
    }
    std::ranges::reverse(path);
    return path;
}

BoardPosition GameModel::positionAt(NodeId id, std::string* error) const
{
    BoardPosition position(boardSize_);
    const std::vector<NodeId> path = pathTo(id);
    if (path.empty()) {
        if (error != nullptr) {
            *error = "game node does not exist";
        }
        return position;
    }

    for (NodeId nodeId : path) {
        const GameNode& gameNode = node(nodeId);
        const SetupStones setup = normalizedSetupStones(gameNode.setupStones);
        for (Point point : setup.black) {
            if (!setupPointIsOnBoard(position, point, "setup stone is outside the board", error)) {
                return position;
            }
            position.placeSetupStone(Color::Black, point);
        }
        for (Point point : setup.white) {
            if (!setupPointIsOnBoard(position, point, "setup stone is outside the board", error)) {
                return position;
            }
            position.placeSetupStone(Color::White, point);
        }
        for (Point point : setup.empty) {
            if (!setupPointIsOnBoard(position, point, "setup clear point is outside the board", error)) {
                return position;
            }
            position.clearPoint(point);
        }
        if (gameNode.move.has_value()) {
            const PlayResult result = position.playMove(*gameNode.move);
            if (!result.ok) {
                if (error != nullptr) {
                    *error = result.error;
                }
                return position;
            }
        }
        if (gameNode.playerToMove.has_value()) {
            position.setSideToMove(*gameNode.playerToMove);
        } else if (shouldInferHandicapWhiteToMove(gameInfo_, gameNode, nodeId, rootId_)) {
            position.setSideToMove(Color::White);
        }
    }

    if (error != nullptr) {
        error->clear();
    }
    return position;
}

BoardPosition GameModel::currentPosition(std::string* error) const
{
    return positionAt(currentId_, error);
}

}  // namespace lizzie::core
