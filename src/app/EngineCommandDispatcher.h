#pragma once

#include "GtpProtocol.h"

#include <QString>

#include <functional>
#include <vector>

namespace lizzie::engine {
class ThreadedKataGoProcess;
}

namespace lizzie::app {

class AnalysisStreamGate;

struct EngineCommandDispatchResult {
    bool engineRunning = true;
    bool allCommandsAccepted = true;
    std::vector<int> commandIds;

    [[nodiscard]] bool ok() const;
};

using GtpCommandSender = std::function<int(const lizzie::engine::GtpCommand&)>;

EngineCommandDispatchResult dispatchGtpCommands(
    const std::vector<lizzie::engine::GtpCommand>& commands,
    const GtpCommandSender& sendCommand,
    AnalysisStreamGate* gate = nullptr);

EngineCommandDispatchResult dispatchGtpCommands(
    lizzie::engine::ThreadedKataGoProcess& engine,
    const std::vector<lizzie::engine::GtpCommand>& commands,
    AnalysisStreamGate* gate = nullptr);

bool dispatchGtpCommand(
    lizzie::engine::ThreadedKataGoProcess& engine,
    const lizzie::engine::GtpCommand& command);

bool dispatchConsoleCommand(
    const QString& commandLine,
    const GtpCommandSender& sendCommand);

bool dispatchConsoleCommand(
    lizzie::engine::ThreadedKataGoProcess& engine,
    const QString& commandLine);

bool dispatchStopAnalyze(lizzie::engine::ThreadedKataGoProcess& engine);

}  // namespace lizzie::app
