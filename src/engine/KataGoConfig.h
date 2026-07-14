#pragma once

#include <QMetaType>

#include <filesystem>
#include <string>
#include <vector>

namespace lizzie::engine {

struct KataGoConfig {
    std::filesystem::path executable;
    std::filesystem::path model;
    std::filesystem::path gtpConfig;
    std::filesystem::path analysisConfig;
    std::filesystem::path workingDirectory;
    std::vector<std::filesystem::path> libraryPaths;
    std::vector<std::string> extraArgs;

    [[nodiscard]] bool hasGtpMinimum() const;
    [[nodiscard]] bool hasAnalysisMinimum() const;
    [[nodiscard]] std::vector<std::string> gtpArguments() const;
    [[nodiscard]] std::vector<std::string> analysisArguments() const;
};

}  // namespace lizzie::engine

Q_DECLARE_METATYPE(lizzie::engine::KataGoConfig)
