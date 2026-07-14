#pragma once

#include "Types.h"

#include <QString>

#include <optional>

class QSettings;

namespace lizzie::app {

struct SessionSettings {
    QString lastFile;
    std::optional<lizzie::core::NodeId> currentNodeId;
    std::optional<int> collectionGameIndex;
};

[[nodiscard]] SessionSettings loadSessionSettings(QSettings& settings);
void saveSessionSettings(
    QSettings& settings,
    const std::optional<QString>& currentFile,
    lizzie::core::NodeId currentNodeId,
    std::optional<int> collectionGameIndex = std::nullopt);

[[nodiscard]] bool firstRunComplete(QSettings& settings);
void setFirstRunComplete(QSettings& settings, bool complete);

}  // namespace lizzie::app
