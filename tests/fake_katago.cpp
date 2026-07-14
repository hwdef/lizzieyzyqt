#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

struct CommandLine {
    std::optional<int> id;
    std::string name;
    std::vector<std::string> args;
};

std::optional<std::string> extractJsonId(const std::string& line)
{
    const std::string key = R"("id":")";
    const std::size_t begin = line.find(key);
    if (begin == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t valueBegin = begin + key.size();
    const std::size_t valueEnd = line.find('"', valueBegin);
    if (valueEnd == std::string::npos) {
        return std::nullopt;
    }
    return line.substr(valueBegin, valueEnd - valueBegin);
}

std::optional<int> extractJsonInt(const std::string& line, const std::string& name)
{
    const std::string key = "\"" + name + "\"";
    const std::size_t keyBegin = line.find(key);
    if (keyBegin == std::string::npos) {
        return std::nullopt;
    }
    const std::size_t colon = line.find(':', keyBegin + key.size());
    if (colon == std::string::npos) {
        return std::nullopt;
    }
    std::size_t valueBegin = colon + 1;
    while (valueBegin < line.size() &&
           std::isspace(static_cast<unsigned char>(line[valueBegin])) != 0) {
        ++valueBegin;
    }
    std::size_t valueEnd = valueBegin;
    while (valueEnd < line.size() && std::isdigit(static_cast<unsigned char>(line[valueEnd])) != 0) {
        ++valueEnd;
    }
    if (valueEnd == valueBegin) {
        return std::nullopt;
    }
    return std::stoi(line.substr(valueBegin, valueEnd - valueBegin));
}

int requestedPointCount(const std::string& line)
{
    const int width = extractJsonInt(line, "boardXSize").value_or(9);
    const int height = extractJsonInt(line, "boardYSize").value_or(9);
    if (width <= 0 || height <= 0 || width > 52 || height > 52) {
        return 81;
    }
    return width * height;
}

std::string ownershipVectorJson(int pointCount, double first, double second)
{
    std::ostringstream output;
    output << '[';
    for (int index = 0; index < pointCount; ++index) {
        if (index > 0) {
            output << ',';
        }
        output << (index % 2 == 0 ? first : second);
    }
    output << ']';
    return output.str();
}

void appendOwnershipValues(std::ostream& output, int pointCount, double first, double second)
{
    for (int index = 0; index < pointCount; ++index) {
        output << ' ' << (index % 2 == 0 ? first : second);
    }
}

bool isInteger(const std::string& text)
{
    if (text.empty()) {
        return false;
    }
    for (char c : text) {
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
            return false;
        }
    }
    return true;
}

CommandLine parseCommandLine(const std::string& line)
{
    std::istringstream stream(line);
    std::string first;
    stream >> first;
    CommandLine command;
    if (isInteger(first)) {
        command.id = std::stoi(first);
        stream >> command.name;
    } else {
        command.name = first;
    }
    std::string arg;
    while (stream >> arg) {
        command.args.push_back(arg);
    }
    return command;
}

std::string uppercase(std::string text)
{
    for (char& ch : text) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return text;
}

std::string chooseMove(const CommandLine& command, const std::set<std::string>& occupied)
{
    const std::string color = command.args.empty() ? "B" : uppercase(command.args.front());
    const std::vector<std::string> blackMoves{"D4", "C3", "F3", "C6", "F6"};
    const std::vector<std::string> whiteMoves{"E5", "D5", "E4", "C5", "F5"};
    const std::vector<std::string>& candidates = color == "W" ? whiteMoves : blackMoves;
    for (const std::string& move : candidates) {
        if (!occupied.contains(move)) {
            return move;
        }
    }
    return "resign";
}

void respond(const CommandLine& command, const std::string& payload, bool success = true)
{
    std::cout << (success ? '=' : '?');
    if (command.id.has_value()) {
        std::cout << *command.id;
    }
    if (!payload.empty()) {
        std::cout << ' ' << payload;
    }
    std::cout << "\n\n" << std::flush;
}

bool containsArg(const std::vector<std::string>& values, const std::string& needle)
{
    for (const std::string& value : values) {
        if (value == needle) {
            return true;
        }
    }
    return false;
}

std::string joinLines(const std::vector<std::string>& lines)
{
    std::ostringstream result;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        if (index > 0) {
            result << '\n';
        }
        result << lines[index];
    }
    return result.str();
}

const char* libraryPathEnvironmentName()
{
#ifdef _WIN32
    return "PATH";
#else
    return "LD_LIBRARY_PATH";
#endif
}

}  // namespace

int main(int argc, char* argv[])
{
    if (argc > 1 && std::string(argv[1]) == "analysis") {
        std::cerr << "analysis fake ready\n" << std::flush;
        bool slow = false;
        for (int index = 2; index < argc; ++index) {
            if (std::string(argv[index]) == "--slow-after-start") {
                slow = true;
            }
        }
        std::string line;
        while (std::getline(std::cin, line)) {
            if (slow) {
                std::cerr << "analysis fake slow request received\n" << std::flush;
                std::this_thread::sleep_for(std::chrono::seconds(30));
                continue;
            }
            const std::optional<std::string> id = extractJsonId(line);
            if (!id.has_value()) {
                std::cout << R"({"id":"","error":"missing id"})" << '\n' << std::flush;
                continue;
            }

            const bool includeOwnership = line.find(R"("includeOwnership":false)") == std::string::npos;
            const int pointCount = requestedPointCount(line);
            std::cout
                << R"({"id":")" << *id
                << R"(","rootInfo":{"winrate":0.58,"scoreMean":1.9,"visits":96},)";
            if (includeOwnership) {
                std::cout << R"("ownership":)" << ownershipVectorJson(pointCount, 0.1, -0.1) << ',';
            }
            std::cout
                << R"("moveInfos":[{"move":"D4","visits":96,"winrate":0.58,"scoreMean":1.9,)"
                << R"("scoreStdev":6.4,"policy":0.19,"pv":["D4","E5"],"pvVisits":[96,48])";
            if (includeOwnership) {
                std::cout << R"(,"ownership":)" << ownershipVectorJson(pointCount, 0.2, -0.2);
            }
            std::cout << R"(}]})" << '\n' << std::flush;
        }
        return 0;
    }

    std::cerr << "CUDA fake backend ready\n" << std::flush;
    bool failListCommands = false;
    bool omitKataAnalyze = false;
    bool omitKataSetRules = false;
    bool omitKataRawNn = false;
    bool omitKataStop = false;
    bool omitKataSetParam = false;
    bool omitGenmove = false;
    bool exitAfterListCommands = false;
    bool exitAfterGenmove = false;
    bool exitAfterKataStop = false;
    bool exitAfterSecondKataStop = false;
    bool alwaysPass = false;
    bool invalidGenmove = false;
    bool slowGenmove = false;
    bool printCwd = false;
    bool printLibraryPath = false;
    bool requireGtpArgv = false;
    bool malformedAnalyzeLine = false;
    bool specialAnalyzeLine = false;
    bool ownershipAnalyzeLine = false;
    bool ownershipOnlyAnalyzeLine = false;
    bool burstAnalyzeLines = false;
    bool backendStderrLines = false;
    std::optional<std::string> fixedGenmove;
    for (int index = 1; index < argc; ++index) {
        if (std::string(argv[index]) == "--exit-with-error") {
            std::cerr << "fatal fake GTP startup failure\n" << std::flush;
            return 9;
        }
        if (std::string(argv[index]) == "--crash-with-error") {
            std::cerr << "fatal fake GTP crash\n" << std::flush;
            std::abort();
        }
        if (std::string(argv[index]) == "--fail-list-commands") {
            failListCommands = true;
        }
        if (std::string(argv[index]) == "--omit-kata-analyze") {
            omitKataAnalyze = true;
        }
        if (std::string(argv[index]) == "--omit-kata-set-rules") {
            omitKataSetRules = true;
        }
        if (std::string(argv[index]) == "--omit-kata-raw-nn") {
            omitKataRawNn = true;
        }
        if (std::string(argv[index]) == "--omit-kata-stop") {
            omitKataStop = true;
        }
        if (std::string(argv[index]) == "--omit-kata-set-param") {
            omitKataSetParam = true;
        }
        if (std::string(argv[index]) == "--omit-genmove") {
            omitGenmove = true;
        }
        if (std::string(argv[index]) == "--exit-after-list-commands") {
            exitAfterListCommands = true;
        }
        if (std::string(argv[index]) == "--exit-after-genmove") {
            exitAfterGenmove = true;
        }
        if (std::string(argv[index]) == "--exit-after-kata-stop") {
            exitAfterKataStop = true;
        }
        if (std::string(argv[index]) == "--exit-after-second-kata-stop") {
            exitAfterSecondKataStop = true;
        }
        if (std::string(argv[index]) == "--always-pass") {
            alwaysPass = true;
        }
        if (std::string(argv[index]) == "--invalid-genmove") {
            invalidGenmove = true;
        }
        if (std::string(argv[index]) == "--fixed-genmove" && index + 1 < argc) {
            fixedGenmove = argv[++index];
        }
        if (std::string(argv[index]) == "--slow-genmove") {
            slowGenmove = true;
        }
        if (std::string(argv[index]) == "--print-cwd") {
            printCwd = true;
        }
        if (std::string(argv[index]) == "--print-library-path-env") {
            printLibraryPath = true;
        }
        if (std::string(argv[index]) == "--raw-protocol-line") {
            std::cout << "raw protocol diagnostic" << '\n' << std::flush;
        }
        if (std::string(argv[index]) == "--require-gtp-argv") {
            requireGtpArgv = true;
        }
        if (std::string(argv[index]) == "--malformed-kata-analyze-line") {
            malformedAnalyzeLine = true;
        }
        if (std::string(argv[index]) == "--special-kata-analyze-line") {
            specialAnalyzeLine = true;
        }
        if (std::string(argv[index]) == "--ownership-kata-analyze-line") {
            ownershipAnalyzeLine = true;
        }
        if (std::string(argv[index]) == "--ownership-only-kata-analyze-line") {
            ownershipOnlyAnalyzeLine = true;
        }
        if (std::string(argv[index]) == "--burst-analyze-lines") {
            burstAnalyzeLines = true;
        }
        if (std::string(argv[index]) == "--backend-stderr-lines") {
            backendStderrLines = true;
        }
    }
    if (requireGtpArgv) {
        std::vector<std::string> args;
        for (int index = 1; index < argc; ++index) {
            args.push_back(argv[index]);
        }
        if (args.size() < 5 || args[0] != "gtp" || args[1] != "-model" || args[2] != "fake-model.bin.gz" ||
            args[3] != "-config" || args[4] != "fake-gtp.cfg" || !containsArg(args, "--require-gtp-argv")) {
            std::cerr << "gtp fake argv mismatch\n" << std::flush;
            return 10;
        }
        std::cerr << "gtp fake argv ok\n" << std::flush;
    }
    if (printCwd) {
        std::cerr << "gtp fake cwd " << std::filesystem::current_path().string() << '\n' << std::flush;
    }
    if (printLibraryPath) {
        const char* value = std::getenv(libraryPathEnvironmentName());
        std::cerr << "gtp fake library path " << (value == nullptr ? "" : value) << '\n' << std::flush;
    }
    if (backendStderrLines) {
        std::cerr << "OpenCL fake backend ready\n"
                  << "TensorRT fake backend unavailable: missing library\n"
                  << std::flush;
    }

    std::set<std::string> occupied;
    int kataStopCount = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const CommandLine command = parseCommandLine(line);

        if (command.name == "name") {
            respond(command, "KataGo Fake");
        } else if (command.name == "version") {
            respond(command, "1.15.0-fake");
        } else if (command.name == "list_commands") {
            if (failListCommands) {
                respond(command, "list_commands disabled by fake engine", false);
            } else {
                std::vector<std::string> commands{"name", "version", "list_commands"};
                if (!omitKataAnalyze) {
                    commands.push_back("kata-analyze");
                }
                if (!omitKataSetRules) {
                    commands.push_back("kata-set-rules");
                }
                if (!omitKataRawNn) {
                    commands.push_back("kata-raw-nn");
                }
                if (!omitGenmove) {
                    commands.push_back("genmove");
                }
                commands.push_back("play");
                commands.push_back("clear_board");
                commands.push_back("boardsize");
                commands.push_back("komi");
                if (!omitKataStop) {
                    commands.push_back("kata-stop");
                }
                if (!omitKataSetParam) {
                    commands.push_back("kata-set-param");
                }
                respond(command, joinLines(commands));
            }
            if (exitAfterListCommands) {
                return 0;
            }
        } else if (command.name == "kata-analyze") {
            if (burstAnalyzeLines) {
                for (int index = 0; index < 120; ++index) {
                    const int x = index % 19;
                    const int y = (index / 19) % 19;
                    const char column = static_cast<char>('A' + x + (x >= 8 ? 1 : 0));
                    const int row = 19 - y;
                    std::cout
                        << "info move " << column << row
                        << " visits " << (index + 1)
                        << " winrate 0.52 scoreMean 1.0 scoreStdev 4.0 policy 0.10 pv "
                        << column << row
                        << " rootInfo visits " << (index + 1)
                        << " winrate 0.52 scoreLead 1.0\n";
                }
                std::cout << std::flush;
                continue;
            }
            if (malformedAnalyzeLine) {
                std::cout << "info move BAD_POINT visits not-a-number scoreMean ??? pv BAD_POINT\n"
                          << std::flush;
            }
            if (ownershipOnlyAnalyzeLine) {
                std::cout << "ownership";
                appendOwnershipValues(std::cout, 81, 0.45, -0.45);
                std::cout << '\n' << std::flush;
            }
            if (specialAnalyzeLine) {
                std::cout
                    << "info move pass visits 16 winrate 0.40 scoreMean -3.5 scoreStdev 4.4 policy 0.05 "
                       "pv pass A9 pvVisits 16 8 "
                       "info move resign visits 8 winrate 0.02 scoreMean -30.0 scoreStdev 2.0 policy 0.01 "
                       "pv resign pvVisits 8 rootInfo visits 24 winrate 0.40 scoreLead -3.5\n"
                    << std::flush;
                continue;
            }
            if (ownershipAnalyzeLine) {
                std::cout
                    << "info move D4 visits 81 winrate 0.57 scoreMean 2.4 scoreStdev 5.1 policy 0.13 "
                       "pv D4 E5 pass pvVisits 81 40 10 movesOwnership";
                appendOwnershipValues(std::cout, 81, 0.35, -0.35);
                std::cout
                    << " info move C3 visits 40 winrate 0.49 scoreMean -0.5 scoreStdev 6.2 policy 0.08 "
                       "pv C3 A1 pvVisits 40 20 movesOwnership";
                appendOwnershipValues(std::cout, 81, -0.15, 0.15);
                std::cout << " rootInfo visits 81 winrate 0.57 scoreLead 2.4 ownership";
                appendOwnershipValues(std::cout, 81, 0.25, -0.25);
                std::cout << '\n' << std::flush;
                continue;
            }
            std::cout
                << "info move D4 visits 64 winrate 0.52 scoreMean 1.4 scoreStdev 6.8 policy 0.11 "
                   "pv D4 E5 pass pvVisits 64 32 16 "
                   "info move E5 visits 32 winrate 0.48 scoreLead -0.6 scoreStdev 7.0 prior 0.08 "
                   "pv E5 D4 pvVisits 32 12 rootInfo visits 96 winrate 0.55 scoreLead 1.8\n"
                << std::flush;
        } else if (command.name == "kata-stop") {
            respond(command, "");
            ++kataStopCount;
            if (exitAfterKataStop || (exitAfterSecondKataStop && kataStopCount >= 2)) {
                return 0;
            }
        } else if (command.name == "clear_board") {
            occupied.clear();
            respond(command, "");
        } else if (command.name == "play") {
            if (command.args.size() >= 2) {
                const std::string point = uppercase(command.args[1]);
                if (point != "PASS" && point != "RESIGN") {
                    occupied.insert(point);
                }
            }
            respond(command, "");
        } else if (command.name == "genmove") {
            if (slowGenmove) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
            const std::string move = invalidGenmove
                ? "not-a-move"
                : fixedGenmove.value_or(alwaysPass ? std::string("pass") : chooseMove(command, occupied));
            if (move != "pass" && move != "resign") {
                occupied.insert(move);
            }
            respond(command, move);
            if (exitAfterGenmove) {
                return 0;
            }
        } else if (command.name == "quit") {
            respond(command, "");
            return 0;
        } else {
            respond(command, "");
        }
    }

    return 0;
}
