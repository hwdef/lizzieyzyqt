#include "ProcessDiagnostics.h"

#include <QRegularExpression>

namespace lizzie::engine {

QStringList toQStringList(const std::vector<std::string>& values)
{
    QStringList result;
    for (const std::string& value : values) {
        result.push_back(QString::fromStdString(value));
    }
    return result;
}

namespace {

QString quoteCommandPart(const QString& value)
{
    if (value.isEmpty()) {
        return "\"\"";
    }
    if (!value.contains(QRegularExpression(R"(\s|["\\])"))) {
        return value;
    }
    QString escaped = value;
    escaped.replace('\\', "\\\\");
    escaped.replace('"', "\\\"");
    return '"' + escaped + '"';
}

}  // namespace

QString commandLineForDiagnostics(const QString& program, const QStringList& arguments)
{
    QStringList parts;
    parts.push_back(quoteCommandPart(program));
    for (const QString& argument : arguments) {
        parts.push_back(quoteCommandPart(argument));
    }
    return parts.join(' ');
}

QString commandLineForDiagnostics(const QProcess& process)
{
    return commandLineForDiagnostics(process.program(), process.arguments());
}

}  // namespace lizzie::engine
