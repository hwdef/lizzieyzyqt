#include "FileWrite.h"

#include <QIODevice>
#include <QSaveFile>

namespace lizzie::app {

bool writeFileAtomically(const QString& path, const QByteArray& data, QString* errorMessage)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    if (file.write(data) != data.size()) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

}  // namespace lizzie::app
