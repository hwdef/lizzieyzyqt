#include "EngineCommandDispatcher.h"

#include "AnalysisStreamGate.h"
#include "AppSettings.h"
#include "PositionSync.h"
#include "ThreadedKataGoProcess.h"

namespace lizzie::app {

namespace {

bool hasBalancedCommandQuotes(const QString& text)
{
    bool inQuotes = false;
    for (qsizetype index = 0; index < text.size();) {
        if (text.at(index) != '"') {
            ++index;
            continue;
        }
        if (index + 2 < text.size() && text.at(index + 1) == '"' && text.at(index + 2) == '"') {
            index += 3;
            continue;
        }
        inQuotes = !inQuotes;
        ++index;
    }
    return !inQuotes;
}

}  // namespace

bool EngineCommandDispatchResult::ok() const
{
    return engineRunning && allCommandsAccepted;
}

EngineCommandDispatchResult dispatchGtpCommands(
    const std::vector<lizzie::engine::GtpCommand>& commands,
    const GtpCommandSender& sendCommand,
    AnalysisStreamGate* gate)
{
    EngineCommandDispatchResult result;
    result.commandIds.reserve(commands.size());
    if (!sendCommand) {
        result.engineRunning = false;
        result.allCommandsAccepted = false;
        if (gate != nullptr) {
            gate->stop();
        }
        return result;
    }

    for (const lizzie::engine::GtpCommand& command : commands) {
        if (!lizzie::engine::isValidGtpCommandName(command.name)) {
            result.allCommandsAccepted = false;
            continue;
        }
        const int commandId = sendCommand(command);
        if (commandId < 0) {
            result.allCommandsAccepted = false;
            continue;
        }
        result.commandIds.push_back(commandId);
        if (gate != nullptr) {
            gate->recordSyncCommand(commandId);
        }
    }
    if (!result.allCommandsAccepted && gate != nullptr) {
        gate->stop();
    }
    return result;
}

EngineCommandDispatchResult dispatchGtpCommands(
    lizzie::engine::ThreadedKataGoProcess& engine,
    const std::vector<lizzie::engine::GtpCommand>& commands,
    AnalysisStreamGate* gate)
{
    if (!engine.isRunning()) {
        if (gate != nullptr) {
            gate->stop();
        }
        return EngineCommandDispatchResult{.engineRunning = false, .allCommandsAccepted = false};
    }
    return dispatchGtpCommands(
        commands,
        [&engine](const lizzie::engine::GtpCommand& command) {
            return engine.sendCommand(command);
        },
        gate);
}

bool dispatchGtpCommand(
    lizzie::engine::ThreadedKataGoProcess& engine,
    const lizzie::engine::GtpCommand& command)
{
    if (!engine.isRunning() || !lizzie::engine::isValidGtpCommandName(command.name)) {
        return false;
    }
    return engine.sendCommand(command) >= 0;
}

bool dispatchConsoleCommand(
    const QString& commandLine,
    const GtpCommandSender& sendCommand)
{
    if (!sendCommand) {
        return false;
    }
    if (!hasBalancedCommandQuotes(commandLine)) {
        return false;
    }

    const std::vector<std::string> tokens = splitCommandArguments(commandLine);
    if (tokens.empty()) {
        return false;
    }

    lizzie::engine::GtpCommand command;
    command.name = tokens.front();
    command.arguments.reserve(static_cast<std::size_t>(tokens.size() - 1));
    for (std::size_t index = 1; index < tokens.size(); ++index) {
        command.arguments.push_back(tokens[index]);
    }
    return lizzie::engine::isValidGtpCommandName(command.name) && sendCommand(command) >= 0;
}

bool dispatchConsoleCommand(
    lizzie::engine::ThreadedKataGoProcess& engine,
    const QString& commandLine)
{
    if (!engine.isRunning()) {
        return false;
    }
    return dispatchConsoleCommand(commandLine, [&engine](const lizzie::engine::GtpCommand& command) {
        return engine.sendCommand(command);
    });
}

bool dispatchStopAnalyze(lizzie::engine::ThreadedKataGoProcess& engine)
{
    return dispatchGtpCommand(engine, lizzie::engine::buildStopAnalyzeCommand());
}

}  // namespace lizzie::app
