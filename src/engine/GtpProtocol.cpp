#include "GtpProtocol.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <sstream>
#include <unordered_set>

namespace lizzie::engine {

namespace {

constexpr std::string_view kGtpColumns = "ABCDEFGHJKLMNOPQRSTUVWXYZ";

[[nodiscard]] std::vector<std::string> splitWhitespace(std::string_view text)
{
    std::vector<std::string> tokens;
    std::size_t position = 0;
    while (position < text.size()) {
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position])) != 0) {
            ++position;
        }
        const std::size_t begin = position;
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position])) == 0) {
            ++position;
        }
        if (begin < position) {
            tokens.emplace_back(text.substr(begin, position - begin));
        }
    }
    return tokens;
}

[[nodiscard]] std::optional<int> parseInt(std::string_view text)
{
    int value = 0;
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::optional<int> parseNonNegativeInt(std::string_view text)
{
    const std::optional<int> value = parseInt(text);
    if (!value.has_value() || *value < 0) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::optional<double> parseDouble(std::string_view text)
{
    double value = 0.0;
    const auto* begin = text.data();
    const auto* end = text.data() + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] std::string uppercase(std::string_view text)
{
    std::string result(text);
    std::ranges::transform(result, result.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return result;
}

[[nodiscard]] std::string lowercase(std::string_view text)
{
    std::string result(text);
    std::ranges::transform(result, result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

[[nodiscard]] std::optional<GtpResponse> parseResponseHeader(std::string_view line)
{
    if (line.empty() || (line.front() != '=' && line.front() != '?')) {
        return std::nullopt;
    }

    GtpResponse response;
    response.success = line.front() == '=';

    std::size_t position = 1;
    while (position < line.size() && line[position] == ' ') {
        ++position;
    }

    const std::size_t idBegin = position;
    while (position < line.size() && std::isdigit(static_cast<unsigned char>(line[position])) != 0) {
        ++position;
    }
    if (idBegin < position) {
        if (position == line.size() || line[position] == ' ') {
            const std::optional<int> id = parseInt(line.substr(idBegin, position - idBegin));
            if (!id.has_value()) {
                return std::nullopt;
            }
            response.id = *id;
        } else {
            position = idBegin;
        }
    }
    while (position < line.size() && line[position] == ' ') {
        ++position;
    }

    response.payload = std::string(line.substr(position));
    if (!response.payload.empty()) {
        response.lines.push_back(response.payload);
    }
    return response;
}

[[nodiscard]] std::string singleLineToken(std::string_view text)
{
    std::string result(text);
    std::ranges::replace(result, '\r', ' ');
    std::ranges::replace(result, '\n', ' ');
    return result;
}

void appendPayloadLine(GtpResponse* response, std::string_view line)
{
    if (!response->payload.empty()) {
        response->payload.push_back('\n');
    }
    response->payload += line;
    response->lines.emplace_back(line);
}

bool asciiCaseEquals(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }
    return std::equal(left.begin(), left.end(), right.begin(), right.end(), [](char leftChar, char rightChar) {
        return std::tolower(static_cast<unsigned char>(leftChar)) ==
            std::tolower(static_cast<unsigned char>(rightChar));
    });
}

bool isKataAnalyzeInfoKey(const std::string& token)
{
    static const std::unordered_set<std::string> keys = {
        "move",
        "edgevisits",
        "visits",
        "winrate",
        "scoremean",
        "scorelead",
        "scorestdev",
        "scoreselfplay",
        "policy",
        "prior",
        "noresultvalue",
        "pv",
        "pvvisits",
        "pvedgevisits",
        "ownership",
        "ownershipstdev",
        "movesownership",
        "movesownershipstdev",
        "rootinfo",
        "order",
        "lcb",
        "utility",
        "utilitylcb",
        "weight",
        "issymmetryof",
        "rawstwerror",
        "rawstscoreerror",
        "rawvartimeleft",
    };
    return keys.contains(lowercase(token));
}

}  // namespace

bool isValidGtpCommandName(std::string_view text)
{
    if (text.empty()) {
        return false;
    }
    return std::ranges::none_of(text, [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
}

std::optional<KataAnalyzeInfo> parseKataAnalyzeInfoBlock(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t end);
std::optional<KataAnalyzeRootInfo> parseKataAnalyzeRootInfoBlock(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t end);
std::vector<double> parseKataAnalyzeOwnershipBlock(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t* next);

std::string GtpCommand::toLine() const
{
    std::string line;
    if (id.has_value() && *id > 0) {
        line += std::to_string(*id);
        line.push_back(' ');
    }
    line += singleLineToken(name);
    for (const std::string& argument : arguments) {
        line.push_back(' ');
        line += singleLineToken(argument);
    }
    line.push_back('\n');
    return line;
}

std::optional<GtpEvent> GtpStreamParser::feedLine(std::string_view line)
{
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }

    if (current_.has_value()) {
        if (line.empty()) {
            GtpEvent event;
            event.type = GtpEvent::Type::Response;
            event.response = std::move(current_);
            current_.reset();
            return event;
        }
        appendPayloadLine(&*current_, line);
        return std::nullopt;
    }

    if (line.empty()) {
        return std::nullopt;
    }

    if (std::optional<GtpResponse> header = parseResponseHeader(line)) {
        current_ = std::move(header);
        return std::nullopt;
    }

    return GtpEvent{GtpEvent::Type::RawLine, std::nullopt, std::string(line)};
}

void GtpStreamParser::reset()
{
    current_.reset();
}

EngineCapabilities EngineCapabilities::fromListCommandsPayload(std::string_view payload)
{
    const std::vector<std::string> tokens = splitWhitespace(payload);
    std::unordered_set<std::string> commands;
    commands.reserve(tokens.size());
    for (const std::string& token : tokens) {
        commands.insert(lowercase(token));
    }

    EngineCapabilities capabilities;
    capabilities.kataAnalyze = commands.contains("kata-analyze");
    capabilities.kataSetRules = commands.contains("kata-set-rules");
    capabilities.kataRawNn = commands.contains("kata-raw-nn");
    capabilities.kataStop = commands.contains("kata-stop");
    capabilities.kataSetParam = commands.contains("kata-set-param");
    capabilities.genmove = commands.contains("genmove");
    return capabilities;
}

std::optional<KataAnalyzeInfo> parseKataAnalyzeInfo(std::string_view line)
{
    const std::vector<KataAnalyzeInfo> infos = parseKataAnalyzeInfos(line);
    if (infos.empty()) {
        return std::nullopt;
    }
    return infos.front();
}

std::vector<KataAnalyzeInfo> parseKataAnalyzeInfos(std::string_view line)
{
    return parseKataAnalyzeLine(line).infos;
}

KataAnalyzeLine parseKataAnalyzeLine(std::string_view line)
{
    std::vector<std::string> tokens = splitWhitespace(line);
    KataAnalyzeLine result;
    if (tokens.empty()) {
        return result;
    }

    auto nextBlock = [&](std::size_t begin) {
        std::size_t end = begin;
        while (end < tokens.size() && !asciiCaseEquals(tokens[end], "info") &&
               !asciiCaseEquals(tokens[end], "rootInfo")) {
            ++end;
        }
        return end;
    };

    std::size_t blockBegin = 0;
    bool hasTopLevelOwnership = false;
    while (blockBegin < tokens.size()) {
        if (asciiCaseEquals(tokens[blockBegin], "info")) {
            const std::size_t blockEnd = nextBlock(blockBegin + 1);
            if (std::optional<KataAnalyzeInfo> info = parseKataAnalyzeInfoBlock(tokens, blockBegin + 1, blockEnd)) {
                result.infos.push_back(std::move(*info));
            }
            blockBegin = blockEnd;
        } else if (asciiCaseEquals(tokens[blockBegin], "rootInfo")) {
            const std::size_t blockEnd = nextBlock(blockBegin + 1);
            result.rootInfo = parseKataAnalyzeRootInfoBlock(tokens, blockBegin + 1, blockEnd);
            if (!hasTopLevelOwnership && result.rootInfo.has_value() && !result.rootInfo->ownership.empty()) {
                result.ownership = result.rootInfo->ownership;
            }
            blockBegin = blockEnd;
        } else if (asciiCaseEquals(tokens[blockBegin], "ownership")) {
            std::size_t blockEnd = blockBegin + 1;
            std::vector<double> ownership = parseKataAnalyzeOwnershipBlock(tokens, blockBegin + 1, &blockEnd);
            if (!ownership.empty()) {
                result.ownership = std::move(ownership);
                hasTopLevelOwnership = true;
            }
            blockBegin = blockEnd > blockBegin ? blockEnd : blockBegin + 1;
        } else {
            ++blockBegin;
        }
    }
    return result;
}

std::vector<double> parseKataAnalyzeOwnershipBlock(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t* next)
{
    std::vector<double> ownership;
    std::size_t index = begin;
    while (index < tokens.size()) {
        const auto value = parseDouble(tokens[index]);
        if (!value.has_value()) {
            break;
        }
        ownership.push_back(*value);
        ++index;
    }
    if (next != nullptr) {
        *next = index;
    }
    return ownership;
}

std::optional<KataAnalyzeInfo> parseKataAnalyzeInfoBlock(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t end)
{
    KataAnalyzeInfo info;
    bool hasVisits = false;
    bool hasMovesOwnership = false;
    for (std::size_t index = begin; index < end; ++index) {
        const std::string& key = tokens[index];
        if (asciiCaseEquals(key, "move") && index + 1 < end) {
            info.move = tokens[++index];
        } else if (asciiCaseEquals(key, "visits") && index + 1 < end) {
            if (const std::optional<int> visits = parseNonNegativeInt(tokens[++index])) {
                info.visits = *visits;
                hasVisits = true;
            }
        } else if (asciiCaseEquals(key, "winrate") && index + 1 < end) {
            info.winrate = parseDouble(tokens[++index]).value_or(0.0);
        } else if ((asciiCaseEquals(key, "scoreMean") || asciiCaseEquals(key, "scoreLead")) && index + 1 < end) {
            info.scoreMean = parseDouble(tokens[++index]).value_or(0.0);
        } else if (asciiCaseEquals(key, "scoreStdev") && index + 1 < end) {
            info.scoreStdev = parseDouble(tokens[++index]).value_or(0.0);
        } else if ((asciiCaseEquals(key, "policy") || asciiCaseEquals(key, "prior")) && index + 1 < end) {
            info.policy = parseDouble(tokens[++index]).value_or(0.0);
        } else if (asciiCaseEquals(key, "pv")) {
            while (index + 1 < end && !isKataAnalyzeInfoKey(tokens[index + 1])) {
                info.pv.push_back(tokens[++index]);
            }
        } else if (asciiCaseEquals(key, "pvVisits")) {
            std::vector<int> pvVisits;
            bool validPvVisits = true;
            while (index + 1 < end && !isKataAnalyzeInfoKey(tokens[index + 1])) {
                if (const auto value = parseNonNegativeInt(tokens[index + 1])) {
                    pvVisits.push_back(*value);
                } else {
                    validPvVisits = false;
                }
                ++index;
            }
            if (validPvVisits && !pvVisits.empty()) {
                info.pvVisits.insert(info.pvVisits.end(), pvVisits.begin(), pvVisits.end());
            }
        } else if (asciiCaseEquals(key, "ownership") || asciiCaseEquals(key, "movesOwnership")) {
            const bool isMovesOwnership = asciiCaseEquals(key, "movesOwnership");
            std::vector<double> ownership;
            while (index + 1 < end) {
                const auto value = parseDouble(tokens[index + 1]);
                if (!value.has_value()) {
                    break;
                }
                ownership.push_back(*value);
                ++index;
            }
            if (!ownership.empty() && (isMovesOwnership || !hasMovesOwnership)) {
                info.ownership = std::move(ownership);
                hasMovesOwnership = isMovesOwnership;
            }
        }
    }

    if (info.move.empty() || !hasVisits) {
        return std::nullopt;
    }
    return info;
}

std::optional<KataAnalyzeRootInfo> parseKataAnalyzeRootInfoBlock(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t end)
{
    KataAnalyzeRootInfo info;
    bool parsed = false;
    for (std::size_t index = begin; index < end; ++index) {
        const std::string& key = tokens[index];
        if (asciiCaseEquals(key, "visits") && index + 1 < end) {
            if (const std::optional<int> visits = parseNonNegativeInt(tokens[++index])) {
                info.visits = *visits;
                info.hasVisits = true;
                parsed = true;
            }
        } else if (asciiCaseEquals(key, "winrate") && index + 1 < end) {
            if (const std::optional<double> winrate = parseDouble(tokens[++index])) {
                info.winrate = *winrate;
                info.hasWinrate = true;
                parsed = true;
            }
        } else if ((asciiCaseEquals(key, "scoreMean") || asciiCaseEquals(key, "scoreLead")) && index + 1 < end) {
            if (const std::optional<double> scoreMean = parseDouble(tokens[++index])) {
                info.scoreMean = *scoreMean;
                info.hasScoreMean = true;
                parsed = true;
            }
        } else if (asciiCaseEquals(key, "ownership") || asciiCaseEquals(key, "movesOwnership")) {
            const std::size_t ownershipBeginSize = info.ownership.size();
            while (index + 1 < end) {
                const auto value = parseDouble(tokens[index + 1]);
                if (!value.has_value()) {
                    break;
                }
                info.ownership.push_back(*value);
                ++index;
            }
            if (info.ownership.size() > ownershipBeginSize) {
                parsed = true;
            }
        }
    }
    if (!parsed) {
        return std::nullopt;
    }
    return info;
}

std::string formatPointForGtp(lizzie::core::Point point, lizzie::core::BoardSize boardSize)
{
    if (point.x < 0 || point.y < 0 || point.x >= boardSize.width || point.y >= boardSize.height ||
        point.x >= static_cast<int>(kGtpColumns.size())) {
        return {};
    }
    return std::string{kGtpColumns[static_cast<std::size_t>(point.x)]} + std::to_string(boardSize.height - point.y);
}

bool isGtpBoardSizeSupported(lizzie::core::BoardSize boardSize)
{
    return boardSize.isValid() && boardSize.width <= static_cast<int>(kGtpColumns.size());
}

std::optional<lizzie::core::Point> parseGtpPoint(std::string_view text, lizzie::core::BoardSize boardSize)
{
    if (!boardSize.isValid() || lowercase(text) == "pass" || text.size() < 2) {
        return std::nullopt;
    }

    const std::string normalized = uppercase(text);
    const auto column = kGtpColumns.find(normalized.front());
    if (column == std::string_view::npos) {
        return std::nullopt;
    }
    const int x = static_cast<int>(column);
    const auto row = parseInt(normalized.substr(1));
    if (!row.has_value()) {
        return std::nullopt;
    }
    const int y = boardSize.height - *row;
    lizzie::core::Point point{x, y};
    if (x < 0 || y < 0 || x >= boardSize.width || y >= boardSize.height) {
        return std::nullopt;
    }
    return point;
}

}  // namespace lizzie::engine
