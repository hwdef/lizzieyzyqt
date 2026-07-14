#pragma once

#include "GameModel.h"

#include <QJsonObject>
#include <QString>

#include <optional>
#include <string>
#include <vector>

namespace lizzie::app {

struct GameDocumentSession {
    std::optional<QString> currentFile;
    std::vector<lizzie::core::GameModel> collection;
    int collectionGameIndex = 0;
    int collectionGameCount = 1;
};

[[nodiscard]] std::optional<int> selectedCollectionGameIndex(const GameDocumentSession& session);
[[nodiscard]] std::optional<int> analysisSidecarCollectionIndex(const GameDocumentSession& session);
[[nodiscard]] QString batchAnalysisOutputPath(const GameDocumentSession& session, bool writeSidecar);
[[nodiscard]] std::string serializeSgfForSession(
    const lizzie::core::GameModel& game,
    const GameDocumentSession& session);
[[nodiscard]] QJsonObject analysisSidecarJsonForSession(
    const lizzie::core::GameModel& game,
    const GameDocumentSession& session);

}  // namespace lizzie::app
