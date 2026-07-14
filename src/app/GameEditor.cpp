#include "GameEditor.h"

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string_view>
#include <utility>

namespace lizzie::app {

namespace {

bool erasePoint(std::vector<lizzie::core::Point>* points, lizzie::core::Point point)
{
    const auto originalSize = points->size();
    points->erase(std::remove(points->begin(), points->end(), point), points->end());
    return points->size() != originalSize;
}

std::size_t countPoint(const std::vector<lizzie::core::Point>& points, lizzie::core::Point point)
{
    return static_cast<std::size_t>(std::ranges::count(points, point));
}

void addUniquePoint(std::vector<lizzie::core::Point>* points, lizzie::core::Point point)
{
    if (std::ranges::find(*points, point) == points->end()) {
        points->push_back(point);
    }
}

std::vector<lizzie::core::BoardLabel>::iterator findLabelAt(
    std::vector<lizzie::core::BoardLabel>& labels,
    lizzie::core::Point point)
{
    return std::ranges::find_if(labels, [point](const lizzie::core::BoardLabel& label) {
        return label.point == point;
    });
}

std::vector<lizzie::core::BoardLabel>::const_iterator findLabelAt(
    const std::vector<lizzie::core::BoardLabel>& labels,
    lizzie::core::Point point)
{
    return std::ranges::find_if(labels, [point](const lizzie::core::BoardLabel& label) {
        return label.point == point;
    });
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

void erasePropertyCaseInsensitive(lizzie::core::GameNode* node, std::string_view name)
{
    for (auto iter = node->properties.begin(); iter != node->properties.end();) {
        if (asciiCaseEquals(iter->first, name)) {
            iter = node->properties.erase(iter);
        } else {
            ++iter;
        }
    }
}

bool hasCanonicalCommentProperty(const lizzie::core::GameNode& node, std::string_view comment)
{
    const std::vector<std::string>* canonicalComment = nullptr;
    std::size_t commentPropertyCount = 0;
    for (const auto& [name, values] : node.properties) {
        if (!asciiCaseEquals(name, "C")) {
            continue;
        }
        ++commentPropertyCount;
        if (name == "C") {
            canonicalComment = &values;
        }
    }
    if (comment.empty()) {
        return commentPropertyCount == 0;
    }
    return commentPropertyCount == 1 && canonicalComment != nullptr && canonicalComment->size() == 1 &&
        canonicalComment->front() == comment;
}

}  // namespace

GameEditor::GameEditor(lizzie::core::GameModel* model)
    : model_(model)
{
}

void GameEditor::setModel(lizzie::core::GameModel* model)
{
    model_ = model;
}

bool GameEditor::setCurrent(lizzie::core::NodeId nodeId)
{
    return model_ != nullptr && model_->setCurrent(nodeId);
}

bool GameEditor::selectParent()
{
    if (model_ == nullptr) {
        return false;
    }
    const lizzie::core::GameNode& current = model_->node(model_->currentId());
    return current.parent.has_value() && model_->setCurrent(*current.parent);
}

bool GameEditor::selectFirstChild()
{
    if (model_ == nullptr) {
        return false;
    }
    const lizzie::core::GameNode& current = model_->node(model_->currentId());
    return !current.children.empty() && model_->setCurrent(current.children.front());
}

bool GameEditor::promoteCurrentVariation()
{
    return model_ != nullptr && model_->promoteVariation(model_->currentId());
}

bool GameEditor::deleteCurrentSubtree()
{
    return model_ != nullptr && model_->deleteSubtree(model_->currentId());
}

std::optional<lizzie::core::Color> GameEditor::currentSideToMove(std::string* error) const
{
    if (model_ == nullptr) {
        if (error != nullptr) {
            *error = "game model is not available";
        }
        return std::nullopt;
    }
    std::string positionError;
    const lizzie::core::BoardPosition position = model_->currentPosition(&positionError);
    if (!positionError.empty()) {
        if (error != nullptr) {
            *error = positionError;
        }
        return std::nullopt;
    }
    if (error != nullptr) {
        error->clear();
    }
    return position.sideToMove();
}

MoveEditResult GameEditor::tryAddMove(lizzie::core::Move move)
{
    if (model_ == nullptr) {
        return MoveEditResult{.error = "game model is not available"};
    }
    std::string error;
    const std::optional<lizzie::core::NodeId> nodeId = model_->tryAddMove(move, &error);
    if (!nodeId.has_value()) {
        return MoveEditResult{.error = std::move(error)};
    }
    return MoveEditResult{.accepted = true, .nodeId = nodeId};
}

MoveEditResult GameEditor::tryPlayAt(lizzie::core::Point point)
{
    std::string error;
    const std::optional<lizzie::core::Color> color = currentSideToMove(&error);
    if (!color.has_value()) {
        return MoveEditResult{.error = std::move(error)};
    }
    return tryAddMove(lizzie::core::Move::play(*color, point));
}

MoveEditResult GameEditor::tryPass()
{
    std::string error;
    const std::optional<lizzie::core::Color> color = currentSideToMove(&error);
    if (!color.has_value()) {
        return MoveEditResult{.error = std::move(error)};
    }
    return tryAddMove(lizzie::core::Move::pass(*color));
}

MoveEditResult GameEditor::tryResign()
{
    std::string error;
    const std::optional<lizzie::core::Color> color = currentSideToMove(&error);
    if (!color.has_value()) {
        return MoveEditResult{.error = std::move(error)};
    }
    return tryAddMove(lizzie::core::Move::resign(*color));
}

std::optional<std::string> GameEditor::labelAt(
    lizzie::core::NodeId nodeId,
    lizzie::core::Point point) const
{
    if (model_ == nullptr) {
        return std::nullopt;
    }
    const lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return std::nullopt;
    }
    const auto existing = findLabelAt(node->labels, point);
    if (existing == node->labels.end()) {
        return std::nullopt;
    }
    return existing->text;
}

bool GameEditor::setComment(lizzie::core::NodeId nodeId, std::string comment)
{
    if (model_ == nullptr) {
        return false;
    }
    lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return false;
    }
    if (node->comment == comment && hasCanonicalCommentProperty(*node, comment)) {
        return false;
    }
    node->comment = std::move(comment);
    erasePropertyCaseInsensitive(node, "C");
    if (node->comment.empty()) {
        return true;
    } else {
        node->properties["C"] = {node->comment};
    }
    return true;
}

bool GameEditor::clearMarkupAt(lizzie::core::NodeId nodeId, lizzie::core::Point point)
{
    if (model_ == nullptr) {
        return false;
    }
    lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return false;
    }
    const std::size_t labelsBefore = node->labels.size();
    const std::size_t marksBefore = node->marks.size();
    node->labels.erase(
        std::remove_if(node->labels.begin(), node->labels.end(), [point](const lizzie::core::BoardLabel& label) {
            return label.point == point;
        }),
        node->labels.end());
    node->marks.erase(
        std::remove_if(node->marks.begin(), node->marks.end(), [point](const lizzie::core::BoardMark& mark) {
            return mark.point == point;
        }),
        node->marks.end());
    return node->labels.size() != labelsBefore || node->marks.size() != marksBefore;
}

bool GameEditor::setLabel(
    lizzie::core::NodeId nodeId,
    lizzie::core::Point point,
    std::string text)
{
    if (model_ == nullptr) {
        return false;
    }
    lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return false;
    }
    const std::size_t labelCount = static_cast<std::size_t>(
        std::ranges::count_if(node->labels, [point](const lizzie::core::BoardLabel& label) {
            return label.point == point;
        }));
    const auto existing = findLabelAt(node->labels, point);
    if (text.empty()) {
        if (labelCount == 0) {
            return false;
        }
        node->labels.erase(
            std::remove_if(node->labels.begin(), node->labels.end(), [point](const lizzie::core::BoardLabel& label) {
                return label.point == point;
            }),
            node->labels.end());
        return true;
    }
    if (labelCount == 1 && existing != node->labels.end() && existing->text == text) {
        return false;
    }
    node->labels.erase(
        std::remove_if(node->labels.begin(), node->labels.end(), [point](const lizzie::core::BoardLabel& label) {
            return label.point == point;
        }),
        node->labels.end());
    node->labels.push_back(lizzie::core::BoardLabel{point, std::move(text)});
    return true;
}

bool GameEditor::toggleMark(
    lizzie::core::NodeId nodeId,
    lizzie::core::Point point,
    lizzie::core::BoardMarkShape shape)
{
    if (model_ == nullptr) {
        return false;
    }
    lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return false;
    }
    const std::size_t markCount =
        static_cast<std::size_t>(std::ranges::count_if(node->marks, [point](const lizzie::core::BoardMark& mark) {
            return mark.point == point;
        }));
    const std::size_t matchingMarkCount = static_cast<std::size_t>(std::ranges::count_if(
        node->marks,
        [point, shape](const lizzie::core::BoardMark& mark) {
            return mark.point == point && mark.shape == shape;
        }));
    const bool toggleOffCanonicalMark = markCount == 1 && matchingMarkCount == 1;
    node->marks.erase(
        std::remove_if(node->marks.begin(), node->marks.end(), [point](const lizzie::core::BoardMark& mark) {
            return mark.point == point;
        }),
        node->marks.end());
    if (!toggleOffCanonicalMark) {
        node->marks.push_back(lizzie::core::BoardMark{point, shape});
    }
    return true;
}

bool GameEditor::editSetupStone(
    lizzie::core::NodeId nodeId,
    lizzie::core::Point point,
    SetupStoneEdit edit)
{
    if (model_ == nullptr) {
        return false;
    }
    lizzie::core::GameNode* node = model_->findNode(nodeId);
    if (node == nullptr) {
        return false;
    }
    lizzie::core::SetupStones& setup = node->setupStones;
    const std::size_t blackCount = countPoint(setup.black, point);
    const std::size_t whiteCount = countPoint(setup.white, point);
    const std::size_t emptyCount = countPoint(setup.empty, point);
    const bool hasCanonicalEmpty = blackCount == 0 && whiteCount == 0 && emptyCount == 1;
    if (edit == SetupStoneEdit::Black && blackCount == 1 && whiteCount == 0 && emptyCount == 0) {
        return false;
    }
    if (edit == SetupStoneEdit::White && blackCount == 0 && whiteCount == 1 && emptyCount == 0) {
        return false;
    }

    erasePoint(&setup.black, point);
    erasePoint(&setup.white, point);
    erasePoint(&setup.empty, point);

    if (edit == SetupStoneEdit::Black) {
        addUniquePoint(&setup.black, point);
    } else if (edit == SetupStoneEdit::White) {
        addUniquePoint(&setup.white, point);
    } else if (!hasCanonicalEmpty) {
        addUniquePoint(&setup.empty, point);
    }
    return true;
}

GameInfoEditResult GameEditor::setGameInfo(lizzie::core::GameInfo info)
{
    if (model_ == nullptr) {
        return {};
    }

    lizzie::core::GameInfo& current = model_->gameInfo();
    GameInfoEditResult result;
    result.changed =
        current.blackPlayer != info.blackPlayer ||
        current.whitePlayer != info.whitePlayer ||
        current.blackRank != info.blackRank ||
        current.whiteRank != info.whiteRank ||
        current.result != info.result ||
        current.date != info.date ||
        current.source != info.source ||
        current.rules != info.rules ||
        current.komi != info.komi ||
        current.handicap != info.handicap;
    result.analysisInvalidated =
        current.rules != info.rules ||
        current.komi != info.komi ||
        current.handicap != info.handicap;
    if (result.changed) {
        current = std::move(info);
    }
    return result;
}

}  // namespace lizzie::app
