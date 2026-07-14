#include "ProcessEnvironment.h"

#include <QDir>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>
#include <vector>

namespace lizzie::engine {

namespace {

QString pathToQString(const std::filesystem::path& path)
{
    return QString::fromStdString(path.string());
}

QString lowerAscii(QString text)
{
    for (QChar& ch : text) {
        ch = QChar(static_cast<char>(std::tolower(static_cast<unsigned char>(ch.toLatin1()))));
    }
    return text;
}

bool pathHasText(const QString& path)
{
    for (const QChar ch : path) {
        if (!ch.isSpace()) {
            return true;
        }
    }
    return false;
}

void appendUnique(QStringList* paths, QString path)
{
    path = path.trimmed();
    if (!pathHasText(path) || paths->contains(path)) {
        return;
    }
    paths->push_back(path);
}

void appendPathList(QStringList* paths, const QString& pathList)
{
    const QChar separator = QDir::listSeparator();
    for (const QString& path : pathList.split(separator, Qt::SkipEmptyParts)) {
        appendUnique(paths, path);
    }
}

bool directoryContainsAny(
    const std::filesystem::path& directory,
    const std::vector<std::filesystem::path>& libraryNames)
{
    std::error_code error;
    if (!std::filesystem::is_directory(directory, error)) {
        return false;
    }
    for (const std::filesystem::path& libraryName : libraryNames) {
        if (std::filesystem::exists(directory / libraryName, error)) {
            return true;
        }
    }
    return false;
}

void appendLibraryDirectoryIfUsable(
    QStringList* paths,
    const std::filesystem::path& directory,
    const std::vector<std::filesystem::path>& libraryNames)
{
    if (directoryContainsAny(directory, libraryNames)) {
        appendUnique(paths, pathToQString(directory));
    }
}

void appendDetectedKataGoLibraryPaths(QStringList* paths, const std::filesystem::path& executable)
{
    const std::filesystem::path executableDirectory = executable.parent_path();
    if (executableDirectory.empty()) {
        return;
    }
    const std::filesystem::path searchRoot = executableDirectory.parent_path();
    if (searchRoot.empty()) {
        return;
    }

    std::error_code error;
    if (!std::filesystem::is_directory(searchRoot, error)) {
        return;
    }

    std::vector<std::filesystem::path> children;
    for (std::filesystem::directory_iterator iter(searchRoot, error), end; !error && iter != end;
         iter.increment(error)) {
        if (iter->is_directory(error)) {
            children.push_back(iter->path());
        }
    }
    std::ranges::sort(children);

    const std::vector<std::filesystem::path> cudaLibraries{
        "libcublas.so.12",
        "libcublasLt.so.12",
        "cublas64_12.dll",
        "cublasLt64_12.dll",
    };
    const std::vector<std::filesystem::path> cudnnLibraries{
        "libcudnn.so.9",
        "cudnn64_9.dll",
    };

    for (const std::filesystem::path& child : children) {
        const QString name = lowerAscii(QString::fromStdString(child.filename().string()));
        if (name.contains("cuda")) {
            appendLibraryDirectoryIfUsable(paths, child / "targets" / "x86_64-linux" / "lib", cudaLibraries);
            appendLibraryDirectoryIfUsable(paths, child / "lib64", cudaLibraries);
            appendLibraryDirectoryIfUsable(paths, child / "lib", cudaLibraries);
            appendLibraryDirectoryIfUsable(paths, child / "bin", cudaLibraries);
        }
        if (name.contains("cudnn")) {
            appendLibraryDirectoryIfUsable(paths, child / "lib", cudnnLibraries);
            appendLibraryDirectoryIfUsable(paths, child / "bin", cudnnLibraries);
        }
    }
}

QStringList libraryPathEntriesForConfig(const KataGoConfig& config, const QProcessEnvironment& environment)
{
    QStringList paths;
    for (const std::filesystem::path& path : config.libraryPaths) {
        appendUnique(&paths, pathToQString(path));
    }

    appendPathList(
        &paths,
        QString::fromLocal8Bit(qgetenv("LIZZIE_KATAGO_LIBRARY_PATH")));
    appendDetectedKataGoLibraryPaths(&paths, config.executable);
    appendPathList(&paths, environment.value(processLibraryPathVariableName()));
    return paths;
}

}  // namespace

QString processLibraryPathVariableName()
{
#ifdef Q_OS_WIN
    return QStringLiteral("PATH");
#else
    return QStringLiteral("LD_LIBRARY_PATH");
#endif
}

QString processLibraryPathForDiagnostics(const QProcessEnvironment& environment)
{
    const QString value = environment.value(processLibraryPathVariableName());
    return value.isEmpty() ? QStringLiteral("(empty)") : value;
}

QProcessEnvironment processEnvironmentForConfig(const KataGoConfig& config)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QStringList libraryPaths = libraryPathEntriesForConfig(config, environment);
    if (!libraryPaths.isEmpty()) {
        environment.insert(processLibraryPathVariableName(), libraryPaths.join(QDir::listSeparator()));
    }
    return environment;
}

}  // namespace lizzie::engine
