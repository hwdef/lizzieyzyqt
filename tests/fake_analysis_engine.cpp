#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::optional<std::string> extractId(const std::string& line)
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

bool contains(const std::vector<std::string>& values, const std::string& needle)
{
    for (const std::string& value : values) {
        if (value == needle) {
            return true;
        }
    }
    return false;
}

bool containsText(const std::string& value, const std::string& needle)
{
    return value.find(needle) != std::string::npos;
}

const char* libraryPathEnvironmentName()
{
#ifdef _WIN32
    return "PATH";
#else
    return "LD_LIBRARY_PATH";
#endif
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
    const int width = extractJsonInt(line, "boardXSize").value_or(19);
    const int height = extractJsonInt(line, "boardYSize").value_or(19);
    if (width <= 0 || height <= 0 || width > 52 || height > 52) {
        return 361;
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

}  // namespace

int main(int argc, char* argv[])
{
    std::cerr << "analysis fake ready\n" << std::flush;
    bool malformed = false;
    bool slow = false;
    bool printCwd = false;
    bool error = false;
    bool exitAfterResponse = false;
    bool requireAnalysisArgv = false;
    bool echoRequest = false;
    bool requireConfiguredRequest = false;
    bool requireHandicapRequest = false;
    bool slowAfterFirstResponse = false;
    bool duplicateResponse = false;
    bool unknownResponse = false;
    bool printLibraryPath = false;
    int delayResponseMs = 0;
    for (int index = 1; index < argc; ++index) {
        if (std::string(argv[index]) == "--fail-after-start") {
            std::cerr << "analysis fake fatal\n" << std::flush;
            return 9;
        }
        if (std::string(argv[index]) == "--crash-after-start") {
            std::cerr << "analysis fake crash\n" << std::flush;
            std::abort();
        }
        if (std::string(argv[index]) == "--malformed-after-start") {
            malformed = true;
        }
        if (std::string(argv[index]) == "--slow-after-start") {
            slow = true;
        }
        if (std::string(argv[index]) == "--print-cwd") {
            printCwd = true;
        }
        if (std::string(argv[index]) == "--error-after-start") {
            error = true;
        }
        if (std::string(argv[index]) == "--exit-after-response") {
            exitAfterResponse = true;
        }
        if (std::string(argv[index]) == "--require-analysis-argv") {
            requireAnalysisArgv = true;
        }
        if (std::string(argv[index]) == "--echo-request") {
            echoRequest = true;
        }
        if (std::string(argv[index]) == "--require-configured-request") {
            requireConfiguredRequest = true;
        }
        if (std::string(argv[index]) == "--require-handicap-request") {
            requireHandicapRequest = true;
        }
        if (std::string(argv[index]) == "--slow-after-first-response") {
            slowAfterFirstResponse = true;
        }
        if (std::string(argv[index]) == "--duplicate-response") {
            duplicateResponse = true;
        }
        if (std::string(argv[index]) == "--unknown-response") {
            unknownResponse = true;
        }
        if (std::string(argv[index]) == "--print-library-path-env") {
            printLibraryPath = true;
        }
        if (std::string(argv[index]) == "--delay-response-ms" && index + 1 < argc) {
            delayResponseMs = std::stoi(argv[++index]);
        }
    }

    if (requireAnalysisArgv) {
        std::vector<std::string> args;
        for (int index = 1; index < argc; ++index) {
            args.push_back(argv[index]);
        }
        if (args.size() < 6 || args[0] != "analysis" || args[1] != "-model" || args[2] != "fake-model.bin.gz" ||
            args[3] != "-config" || args[4] != "fake-analysis.cfg" || !contains(args, "-quit-without-waiting")) {
            std::cerr << "analysis fake argv mismatch\n" << std::flush;
            return 10;
        }
        std::cerr << "analysis fake argv ok\n" << std::flush;
    }

    if (printCwd) {
        std::cerr << "analysis fake cwd " << std::filesystem::current_path().string() << '\n' << std::flush;
    }
    if (printLibraryPath) {
        const char* value = std::getenv(libraryPathEnvironmentName());
        std::cerr << "analysis fake library path " << (value == nullptr ? "" : value) << '\n' << std::flush;
    }

    std::string line;
    int responsesSent = 0;
    while (std::getline(std::cin, line)) {
        if (echoRequest) {
            std::cerr << "analysis fake request " << line << '\n' << std::flush;
        }
        if (slow || (slowAfterFirstResponse && responsesSent > 0)) {
            std::cerr << "analysis fake slow request received\n" << std::flush;
            std::this_thread::sleep_for(std::chrono::seconds(30));
            continue;
        }
        if (malformed) {
            std::cout << "not-json" << '\n' << std::flush;
            continue;
        }
        const std::optional<std::string> id = extractId(line);
        if (!id.has_value()) {
            std::cout << R"({"id":"","error":"missing id"})" << '\n' << std::flush;
            continue;
        }
        if (error) {
            std::cout << R"({"id":")" << *id << R"(","error":"analysis fake node error"})" << '\n' << std::flush;
            return 0;
        }
        if (delayResponseMs > 0) {
            std::cerr << "analysis fake delayed response\n" << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayResponseMs));
        }
        if (requireConfiguredRequest) {
            const std::vector<std::string> requiredSnippets = {
                R"("includeOwnership":false)",
                R"("maxVisits":222)",
                R"("maxPlayouts":333)",
                R"("maxTime":4.5)",
                R"("playoutDoublingAdvantage":-0.5)",
                R"("wideRootNoise":0.6)",
                R"("reportAnalysisWinratesAs":"BLACK")",
            };
            for (const std::string& snippet : requiredSnippets) {
                if (!containsText(line, snippet)) {
                    std::cerr << "analysis fake request missing " << snippet << '\n' << std::flush;
                    std::cout << R"({"id":")" << *id << R"(","error":"analysis fake configured request mismatch"})"
                              << '\n'
                              << std::flush;
                    return 11;
                }
            }
            std::cerr << "analysis fake configured request ok\n" << std::flush;
        }
        if (requireHandicapRequest) {
            const std::vector<std::string> requiredSnippets = {
                R"("boardXSize":9)",
                R"("boardYSize":9)",
                R"("rules":"chinese")",
                R"("komi":7.5)",
                R"("initialPlayer":"W")",
                R"("moves":[])",
                R"("initialStones":[["B","D6"],["B","F6"]])",
            };
            for (const std::string& snippet : requiredSnippets) {
                if (!containsText(line, snippet)) {
                    std::cerr << "analysis fake handicap request missing " << snippet << '\n' << std::flush;
                    std::cout << R"({"id":")" << *id << R"(","error":"analysis fake handicap request mismatch"})"
                              << '\n'
                              << std::flush;
                    return 12;
                }
            }
            std::cerr << "analysis fake handicap request ok\n" << std::flush;
        }

        const int visits = requireConfiguredRequest ? 222 : 128;
        const bool includeOwnership = !containsText(line, R"("includeOwnership":false)");
        const int pointCount = requestedPointCount(line);
        std::ostringstream response;
        response << R"({"id":")" << *id
                 << R"(","rootInfo":{"winrate":0.57,"scoreMean":2.2,"visits":)" << visits << R"(},)";
        if (includeOwnership) {
            response << R"("ownership":)" << ownershipVectorJson(pointCount, 0.1, -0.1);
        } else {
            response << R"("ownership":[])";
        }
        response << R"(,"moveInfos":[{"move":"D4","visits":)" << visits << R"(,"winrate":0.57,"scoreMean":2.2,)"
                 << R"("scoreStdev":7.1,"policy":0.21,"pv":["D4","E5"],"pvVisits":[)" << visits << ','
                 << (visits / 2) << R"(])";
        if (includeOwnership) {
            response << R"(,"ownership":)" << ownershipVectorJson(pointCount, 0.2, -0.2);
        }
        response << R"(}]})";
        if (unknownResponse) {
            std::cout << R"({"id":"node:unknown","rootInfo":{"winrate":0.49,"scoreMean":-0.2,"visits":1},"moveInfos":[]})"
                      << '\n'
                      << std::flush;
        }
        std::cout << response.str() << '\n' << std::flush;
        if (duplicateResponse) {
            std::cout << response.str() << '\n' << std::flush;
        }
        ++responsesSent;
        if (exitAfterResponse) {
            return 0;
        }
    }

    return 0;
}
