#pragma once

#include "Types.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lizzie::engine {

struct GtpCommand {
    std::optional<int> id;
    std::string name;
    std::vector<std::string> arguments;

    [[nodiscard]] std::string toLine() const;
};

[[nodiscard]] bool isValidGtpCommandName(std::string_view text);

struct GtpResponse {
    bool success = false;
    std::optional<int> id;
    std::string payload;
    std::vector<std::string> lines;
};

struct GtpEvent {
    enum class Type {
        Response,
        RawLine,
    };

    Type type = Type::RawLine;
    std::optional<GtpResponse> response;
    std::string rawLine;
};

class GtpStreamParser {
public:
    [[nodiscard]] std::optional<GtpEvent> feedLine(std::string_view line);
    void reset();

private:
    std::optional<GtpResponse> current_;
};

struct EngineCapabilities {
    bool kataAnalyze = false;
    bool kataSetRules = false;
    bool kataRawNn = false;
    bool kataStop = false;
    bool kataSetParam = false;
    bool genmove = false;

    [[nodiscard]] static EngineCapabilities fromListCommandsPayload(std::string_view payload);
};

struct KataAnalyzeInfo {
    std::string move;
    int visits = 0;
    double winrate = 0.0;
    double scoreMean = 0.0;
    double scoreStdev = 0.0;
    double policy = 0.0;
    std::vector<std::string> pv;
    std::vector<int> pvVisits;
    std::vector<double> ownership;
};

struct KataAnalyzeRootInfo {
    int visits = 0;
    bool hasVisits = false;
    double winrate = 0.0;
    bool hasWinrate = false;
    double scoreMean = 0.0;
    bool hasScoreMean = false;
    std::vector<double> ownership;
};

struct KataAnalyzeLine {
    std::vector<KataAnalyzeInfo> infos;
    std::optional<KataAnalyzeRootInfo> rootInfo;
    std::vector<double> ownership;
};

[[nodiscard]] std::optional<KataAnalyzeInfo> parseKataAnalyzeInfo(std::string_view line);
[[nodiscard]] std::vector<KataAnalyzeInfo> parseKataAnalyzeInfos(std::string_view line);
[[nodiscard]] KataAnalyzeLine parseKataAnalyzeLine(std::string_view line);
[[nodiscard]] bool isGtpBoardSizeSupported(lizzie::core::BoardSize boardSize);
[[nodiscard]] std::string formatPointForGtp(lizzie::core::Point point, lizzie::core::BoardSize boardSize);
[[nodiscard]] std::optional<lizzie::core::Point> parseGtpPoint(std::string_view text, lizzie::core::BoardSize boardSize);

}  // namespace lizzie::engine
