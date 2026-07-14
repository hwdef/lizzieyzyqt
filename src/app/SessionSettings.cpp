#include "SessionSettings.h"

#include <QMetaType>
#include <QSettings>
#include <QVariant>

namespace lizzie::app {

namespace {

std::optional<bool> boolFromStoredValue(const QVariant& value)
{
    if (value.metaType().id() == QMetaType::Bool) {
        return value.toBool();
    }
    const QString text = value.toString().trimmed().toLower();
    if (text == "true" || text == "1") {
        return true;
    }
    if (text == "false" || text == "0") {
        return false;
    }
    return std::nullopt;
}

}  // namespace

SessionSettings loadSessionSettings(QSettings& settings)
{
    SessionSettings session;
    session.lastFile = settings.value("session/lastFile").toString();

    bool ok = false;
    const qulonglong nodeId = settings.value("session/currentNodeId").toULongLong(&ok);
    if (ok && nodeId > 0) {
        session.currentNodeId = static_cast<lizzie::core::NodeId>(nodeId);
    }

    ok = false;
    const int collectionGameIndex = settings.value("session/collectionGameIndex").toInt(&ok);
    if (ok && collectionGameIndex >= 0) {
        session.collectionGameIndex = collectionGameIndex;
    }
    return session;
}

void saveSessionSettings(
    QSettings& settings,
    const std::optional<QString>& currentFile,
    lizzie::core::NodeId currentNodeId,
    std::optional<int> collectionGameIndex)
{
    if (!currentFile.has_value()) {
        settings.remove("session/lastFile");
        settings.remove("session/currentNodeId");
        settings.remove("session/collectionGameIndex");
        return;
    }

    settings.setValue("session/lastFile", *currentFile);
    settings.setValue("session/currentNodeId", QVariant::fromValue<qulonglong>(currentNodeId));
    if (collectionGameIndex.has_value() && *collectionGameIndex >= 0) {
        settings.setValue("session/collectionGameIndex", *collectionGameIndex);
    } else {
        settings.remove("session/collectionGameIndex");
    }
}

bool firstRunComplete(QSettings& settings)
{
    if (!settings.contains("startup/firstRunComplete")) {
        return false;
    }
    return boolFromStoredValue(settings.value("startup/firstRunComplete")).value_or(false);
}

void setFirstRunComplete(QSettings& settings, bool complete)
{
    settings.setValue("startup/firstRunComplete", complete);
}

}  // namespace lizzie::app
