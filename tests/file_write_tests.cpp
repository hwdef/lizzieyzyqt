#include "FileWrite.h"
#include "GameDocumentSession.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

QByteArray readAll(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "test file should open for reading");
    return file.readAll();
}

void testAtomicFileWrites()
{
    QTemporaryDir dir;
    require(dir.isValid(), "temporary directory should be valid");

    const QString path = dir.filePath("game.sgf");
    QString error;
    require(lizzie::app::writeFileAtomically(path, QByteArray("(;SZ[9])"), &error), "atomic write should succeed");
    require(error.isEmpty(), "successful write should not report an error");
    require(readAll(path) == QByteArray("(;SZ[9])"), "atomic write should persist data");

    require(
        lizzie::app::writeFileAtomically(path, QByteArray("(;SZ[19])"), &error),
        "atomic overwrite should succeed");
    require(readAll(path) == QByteArray("(;SZ[19])"), "atomic overwrite should replace data");

    error.clear();
    const QString missingParentPath = dir.filePath("missing-parent/game.sgf");
    require(
        !lizzie::app::writeFileAtomically(missingParentPath, QByteArray("new"), &error),
        "atomic write to missing parent should fail");
    require(!error.isEmpty(), "failed atomic write should report an error");
    require(readAll(path) == QByteArray("(;SZ[19])"), "failed write should not damage existing files");
}

void testGameDocumentSessionSerializesSelectedCollectionGame()
{
    lizzie::core::GameModel first(lizzie::core::BoardSize::square(9));
    first.gameInfo().blackPlayer = "First";
    lizzie::core::GameModel second(lizzie::core::BoardSize::square(9));
    second.gameInfo().blackPlayer = "Second";
    lizzie::core::GameModel edited(lizzie::core::BoardSize::square(9));
    edited.gameInfo().blackPlayer = "Edited";
    edited.addMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 2}));

    lizzie::app::GameDocumentSession session;
    session.currentFile = QStringLiteral("/tmp/collection.sgf");
    session.collection = {first, second};
    session.collectionGameIndex = 1;
    session.collectionGameCount = 2;

    require(
        lizzie::app::selectedCollectionGameIndex(session) == 1,
        "document session should expose selected collection index");
    require(
        lizzie::app::analysisSidecarCollectionIndex(session) == 1,
        "document session should expose sidecar collection index");
    require(
        lizzie::app::batchAnalysisOutputPath(session, true) == QStringLiteral("/tmp/collection.sgf.analysis.json"),
        "document session should build sidecar output path");
    require(
        lizzie::app::batchAnalysisOutputPath(session, false) == QStringLiteral("/tmp/collection.sgf"),
        "document session should build SGF writeback output path");

    const std::string collectionSgf = lizzie::app::serializeSgfForSession(edited, session);
    require(collectionSgf.find("PB[First]") != std::string::npos, "collection save should preserve unselected game");
    require(collectionSgf.find("PB[Edited]") != std::string::npos, "collection save should replace selected game");
    require(collectionSgf.find("PB[Second]") == std::string::npos, "collection save should not keep stale selected game");

    const QJsonObject sidecar = lizzie::app::analysisSidecarJsonForSession(edited, session);
    require(sidecar.value("sgf").toString() == QStringLiteral("/tmp/collection.sgf"), "sidecar should keep SGF path");
    require(sidecar.value("collectionGameIndex").toInt(-1) == 1, "sidecar should keep collection game index");
}

void testGameDocumentSessionFallsBackToSingleGameForInvalidCollectionSelection()
{
    lizzie::core::GameModel edited(lizzie::core::BoardSize::square(9));
    edited.gameInfo().blackPlayer = "Edited";
    lizzie::core::GameModel stale(lizzie::core::BoardSize::square(9));
    stale.gameInfo().blackPlayer = "Stale";

    lizzie::app::GameDocumentSession session;
    session.collection = {stale};
    session.collectionGameIndex = 7;
    session.collectionGameCount = 1;

    require(
        !lizzie::app::selectedCollectionGameIndex(session).has_value(),
        "single-game session should not expose a collection index");
    require(
        lizzie::app::batchAnalysisOutputPath(session, true).isEmpty(),
        "unsaved session should not build a batch output path");

    const std::string sgf = lizzie::app::serializeSgfForSession(edited, session);
    require(sgf.find("PB[Edited]") != std::string::npos, "invalid collection selection should save current game");
    require(sgf.find("PB[Stale]") == std::string::npos, "invalid collection selection should not save stale collection");
}

void testGameDocumentSessionRejectsOutOfRangeCollectionIndex()
{
    lizzie::core::GameModel first(lizzie::core::BoardSize::square(9));
    first.gameInfo().blackPlayer = "First";
    lizzie::core::GameModel second(lizzie::core::BoardSize::square(9));
    second.gameInfo().blackPlayer = "Second";
    lizzie::core::GameModel edited(lizzie::core::BoardSize::square(9));
    edited.gameInfo().blackPlayer = "Edited";

    lizzie::app::GameDocumentSession session;
    session.currentFile = QStringLiteral("/tmp/collection.sgf");
    session.collection = {first, second};
    session.collectionGameIndex = 9;
    session.collectionGameCount = 2;

    require(
        !lizzie::app::selectedCollectionGameIndex(session).has_value(),
        "out-of-range collection selection should not expose a selected index");
    require(
        !lizzie::app::analysisSidecarCollectionIndex(session).has_value(),
        "out-of-range collection selection should not expose a sidecar index");

    const QJsonObject sidecar = lizzie::app::analysisSidecarJsonForSession(edited, session);
    require(
        !sidecar.contains("collectionGameIndex"),
        "out-of-range collection selection should not write collectionGameIndex");

    const std::string sgf = lizzie::app::serializeSgfForSession(edited, session);
    require(sgf.find("PB[Edited]") != std::string::npos, "out-of-range selection should save current game");
    require(sgf.find("PB[First]") == std::string::npos, "out-of-range selection should not save stale collection");
    require(sgf.find("PB[Second]") == std::string::npos, "out-of-range selection should not save stale collection");
}

}  // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    try {
        testAtomicFileWrites();
        testGameDocumentSessionSerializesSelectedCollectionGame();
        testGameDocumentSessionFallsBackToSingleGameForInvalidCollectionSelection();
        testGameDocumentSessionRejectsOutOfRangeCollectionIndex();
    } catch (const std::exception& error) {
        std::cerr << "file_write_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "file_write_tests passed\n";
    return EXIT_SUCCESS;
}
