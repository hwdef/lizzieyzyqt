#include "EngineManager.h"

namespace lizzie::app {

lizzie::engine::ThreadedKataGoProcess& EngineManager::mainEngine()
{
    return mainEngine_;
}

const lizzie::engine::ThreadedKataGoProcess& EngineManager::mainEngine() const
{
    return mainEngine_;
}

lizzie::engine::ThreadedKataGoProcess& EngineManager::compareEngine()
{
    return compareEngine_;
}

const lizzie::engine::ThreadedKataGoProcess& EngineManager::compareEngine() const
{
    return compareEngine_;
}

lizzie::engine::ThreadedAnalysisProcess& EngineManager::batchAnalysis()
{
    return batchAnalysis_;
}

const lizzie::engine::ThreadedAnalysisProcess& EngineManager::batchAnalysis() const
{
    return batchAnalysis_;
}

lizzie::engine::EngineCapabilities& EngineManager::mainCapabilities()
{
    return mainCapabilities_;
}

const lizzie::engine::EngineCapabilities& EngineManager::mainCapabilities() const
{
    return mainCapabilities_;
}

lizzie::engine::EngineCapabilities& EngineManager::compareCapabilities()
{
    return compareCapabilities_;
}

const lizzie::engine::EngineCapabilities& EngineManager::compareCapabilities() const
{
    return compareCapabilities_;
}

void EngineManager::resetMainCapabilities()
{
    mainCapabilities_ = {};
}

void EngineManager::resetCompareCapabilities()
{
    compareCapabilities_ = {};
}

void EngineManager::stopAll()
{
    batchAnalysis_.cancelAndWait();
    mainEngine_.stop();
    compareEngine_.stop();
}

}  // namespace lizzie::app
