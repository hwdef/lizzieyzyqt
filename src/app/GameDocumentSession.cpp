#include "GameDocumentSession.h"

#include "BatchAnalysis.h"
#include "Sgf.h"

namespace lizzie::app {

std::optional<int> selectedCollectionGameIndex(const GameDocumentSession& session)
{
    const int collectionCount = session.collection.empty()
        ? session.collectionGameCount
        : static_cast<int>(session.collection.size());
    if (collectionCount <= 1 ||
        session.collectionGameIndex < 0 ||
        session.collectionGameIndex >= collectionCount) {
        return std::nullopt;
    }
    return session.collectionGameIndex;
}

std::optional<int> analysisSidecarCollectionIndex(const GameDocumentSession& session)
{
    return selectedCollectionGameIndex(session);
}

QString batchAnalysisOutputPath(const GameDocumentSession& session, bool writeSidecar)
{
    if (!session.currentFile.has_value()) {
        return {};
    }
    return writeSidecar ? *session.currentFile + ".analysis.json" : *session.currentFile;
}

std::string serializeSgfForSession(
    const lizzie::core::GameModel& game,
    const GameDocumentSession& session)
{
    if (session.collection.size() > 1 &&
        session.collectionGameIndex >= 0 &&
        session.collectionGameIndex < static_cast<int>(session.collection.size())) {
        std::vector<lizzie::core::GameModel> collection = session.collection;
        collection[static_cast<std::size_t>(session.collectionGameIndex)] = game;
        return lizzie::core::writeSgfCollection(collection);
    }
    return lizzie::core::writeSgf(game);
}

QJsonObject analysisSidecarJsonForSession(
    const lizzie::core::GameModel& game,
    const GameDocumentSession& session)
{
    return lizzie::engine::analysisSidecarToJson(
        game,
        session.currentFile.value_or(QString()).toStdString(),
        analysisSidecarCollectionIndex(session));
}

}  // namespace lizzie::app
