#pragma once

#include "AppSettings.h"

#include <QDir>
#include <QJsonObject>
#include <QStringList>

namespace lizzie::app {

struct LegacyConfigImportResult {
    EngineUiSettings settings;
    QStringList notes;
    int importedEngines = 0;
    bool hasUsableKataGo = false;
};

[[nodiscard]] LegacyConfigImportResult importLegacyConfigObject(
    const QJsonObject& root,
    const QDir& baseDir,
    const EngineUiSettings& currentSettings);

}  // namespace lizzie::app
