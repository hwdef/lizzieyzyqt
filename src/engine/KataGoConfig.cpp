#include "KataGoConfig.h"

#include <algorithm>
#include <cctype>

namespace lizzie::engine {

namespace {

bool pathHasNonWhitespaceText(const std::filesystem::path& path)
{
    const std::string text = path.string();
    return std::ranges::any_of(text, [](unsigned char ch) {
        return std::isspace(ch) == 0;
    });
}

}  // namespace

bool KataGoConfig::hasGtpMinimum() const
{
    return pathHasNonWhitespaceText(executable) && pathHasNonWhitespaceText(model) &&
        pathHasNonWhitespaceText(gtpConfig);
}

bool KataGoConfig::hasAnalysisMinimum() const
{
    return pathHasNonWhitespaceText(executable) && pathHasNonWhitespaceText(model) &&
        pathHasNonWhitespaceText(analysisConfig);
}

std::vector<std::string> KataGoConfig::gtpArguments() const
{
    std::vector<std::string> args = {"gtp", "-model", model.string(), "-config", gtpConfig.string()};
    args.insert(args.end(), extraArgs.begin(), extraArgs.end());
    return args;
}

std::vector<std::string> KataGoConfig::analysisArguments() const
{
    std::vector<std::string> args = {
        "analysis",
        "-model",
        model.string(),
        "-config",
        analysisConfig.string(),
        "-quit-without-waiting",
    };
    args.insert(args.end(), extraArgs.begin(), extraArgs.end());
    return args;
}

}  // namespace lizzie::engine
