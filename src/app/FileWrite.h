#pragma once

#include <QByteArray>
#include <QString>

namespace lizzie::app {

[[nodiscard]] bool writeFileAtomically(const QString& path, const QByteArray& data, QString* errorMessage = nullptr);

}  // namespace lizzie::app
