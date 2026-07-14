#pragma once

#include <QProcess>
#include <QString>
#include <QStringList>

#include <string>
#include <vector>

namespace lizzie::engine {

[[nodiscard]] QStringList toQStringList(const std::vector<std::string>& values);
[[nodiscard]] QString commandLineForDiagnostics(const QString& program, const QStringList& arguments);
[[nodiscard]] QString commandLineForDiagnostics(const QProcess& process);

}  // namespace lizzie::engine
