#pragma once

#include "GtpProtocol.h"
#include "ThreadedAnalysisProcess.h"
#include "ThreadedKataGoProcess.h"

namespace lizzie::app {

class EngineManager {
public:
    EngineManager() = default;
    EngineManager(const EngineManager&) = delete;
    EngineManager& operator=(const EngineManager&) = delete;

    [[nodiscard]] lizzie::engine::ThreadedKataGoProcess& mainEngine();
    [[nodiscard]] const lizzie::engine::ThreadedKataGoProcess& mainEngine() const;
    [[nodiscard]] lizzie::engine::ThreadedKataGoProcess& compareEngine();
    [[nodiscard]] const lizzie::engine::ThreadedKataGoProcess& compareEngine() const;
    [[nodiscard]] lizzie::engine::ThreadedAnalysisProcess& batchAnalysis();
    [[nodiscard]] const lizzie::engine::ThreadedAnalysisProcess& batchAnalysis() const;

    [[nodiscard]] lizzie::engine::EngineCapabilities& mainCapabilities();
    [[nodiscard]] const lizzie::engine::EngineCapabilities& mainCapabilities() const;
    [[nodiscard]] lizzie::engine::EngineCapabilities& compareCapabilities();
    [[nodiscard]] const lizzie::engine::EngineCapabilities& compareCapabilities() const;
    void resetMainCapabilities();
    void resetCompareCapabilities();

    void stopAll();

private:
    lizzie::engine::ThreadedKataGoProcess mainEngine_;
    lizzie::engine::ThreadedKataGoProcess compareEngine_;
    lizzie::engine::ThreadedAnalysisProcess batchAnalysis_;
    lizzie::engine::EngineCapabilities mainCapabilities_;
    lizzie::engine::EngineCapabilities compareCapabilities_;
};

}  // namespace lizzie::app
