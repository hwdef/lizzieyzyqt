#pragma once

#include "KataGoConfig.h"

#include <QProcessEnvironment>
#include <QString>

namespace lizzie::engine {

[[nodiscard]] QString processLibraryPathVariableName();
[[nodiscard]] QString processLibraryPathForDiagnostics(const QProcessEnvironment& environment);
[[nodiscard]] QProcessEnvironment processEnvironmentForConfig(const KataGoConfig& config);

}  // namespace lizzie::engine
