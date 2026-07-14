#include "Sgf.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>

namespace lizzie::core {

namespace {

using PropertyMap = std::map<std::string, std::vector<std::string>>;
constexpr std::string_view kWinratePerspectiveProperty = "LZYPERSP";

struct RawNode {
    PropertyMap properties;
    std::vector<std::unique_ptr<RawNode>> children;
};

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

[[nodiscard]] bool isWritablePropertyName(std::string_view name)
{
    if (name.empty() || std::isalpha(static_cast<unsigned char>(name.front())) == 0) {
        return false;
    }
    for (const char current : name.substr(1)) {
        if (std::isalnum(static_cast<unsigned char>(current)) == 0) {
            return false;
        }
    }
    return true;
}

void erasePropertyCaseInsensitive(PropertyMap* properties, std::string_view name)
{
    for (auto iter = properties->begin(); iter != properties->end();) {
        if (asciiCaseEquals(iter->first, name)) {
            iter = properties->erase(iter);
        } else {
            ++iter;
        }
    }
}

class Parser {
public:
    explicit Parser(std::string_view input)
        : input_(input)
    {
    }

    std::vector<std::unique_ptr<RawNode>> parseCollection()
    {
        std::vector<std::unique_ptr<RawNode>> games;
        skipWhitespace();
        while (!eof()) {
            games.push_back(parseTree());
            skipWhitespace();
        }
        return games;
    }

private:
    [[nodiscard]] bool eof() const
    {
        return position_ >= input_.size();
    }

    [[nodiscard]] char peek() const
    {
        return eof() ? '\0' : input_[position_];
    }

    char get()
    {
        if (eof()) {
            throw SgfError("unexpected end of SGF");
        }
        return input_[position_++];
    }

    void expect(char expected)
    {
        skipWhitespace();
        const char actual = get();
        if (actual != expected) {
            std::ostringstream message;
            message << "expected '" << expected << "' but found '" << actual << "'";
            throw SgfError(message.str());
        }
    }

    void skipWhitespace()
    {
        while (!eof() && std::isspace(static_cast<unsigned char>(peek())) != 0) {
            ++position_;
        }
    }

    std::unique_ptr<RawNode> parseTree()
    {
        expect('(');
        skipWhitespace();
        auto root = parseNode();
        RawNode* cursor = root.get();

        skipWhitespace();
        while (peek() == ';') {
            auto child = parseNode();
            RawNode* childPtr = child.get();
            cursor->children.push_back(std::move(child));
            cursor = childPtr;
            skipWhitespace();
        }

        while (peek() == '(') {
            cursor->children.push_back(parseTree());
            skipWhitespace();
        }

        expect(')');
        return root;
    }

    std::unique_ptr<RawNode> parseNode()
    {
        expect(';');
        auto node = std::make_unique<RawNode>();

        for (;;) {
            skipWhitespace();
            const char next = peek();
            if (next == '\0' || next == ';' || next == '(' || next == ')') {
                break;
            }

            std::string ident;
            if (std::isalpha(static_cast<unsigned char>(peek())) == 0) {
                throw SgfError("expected SGF property identifier");
            }
            ident.push_back(get());
            while (!eof() && std::isalnum(static_cast<unsigned char>(peek())) != 0) {
                ident.push_back(get());
            }

            skipWhitespace();
            if (peek() != '[') {
                throw SgfError("expected SGF property value");
            }
            do {
                node->properties[ident].push_back(parseValue());
                skipWhitespace();
            } while (peek() == '[');
        }

        return node;
    }

    std::string parseValue()
    {
        expect('[');
        std::string value;
        while (!eof()) {
            const char current = get();
            if (current == ']') {
                return value;
            }
            if (current == '\\') {
                if (eof()) {
                    return value;
                }
                const char escaped = get();
                if (escaped == '\n') {
                    continue;
                }
                if (escaped == '\r') {
                    if (!eof() && peek() == '\n') {
                        get();
                    }
                    continue;
                }
                value.push_back(escaped);
            } else {
                value.push_back(current);
            }
        }
        throw SgfError("unterminated SGF property value");
    }

    std::string_view input_;
    std::size_t position_ = 0;
};

[[nodiscard]] const std::vector<std::string>* valuesFor(const PropertyMap& properties, const std::string& name)
{
    const auto iter = properties.find(name);
    return iter == properties.end() ? nullptr : &iter->second;
}

[[nodiscard]] const std::vector<std::string>* valuesForCaseInsensitive(
    const PropertyMap& properties,
    std::string_view name)
{
    if (const std::vector<std::string>* values = valuesFor(properties, std::string(name))) {
        return values;
    }
    for (const auto& [propertyName, values] : properties) {
        if (asciiCaseEquals(propertyName, name)) {
            return &values;
        }
    }
    return nullptr;
}

[[nodiscard]] std::string firstValue(const PropertyMap& properties, const std::string& name)
{
    const std::vector<std::string>* values = valuesFor(properties, name);
    return values == nullptr || values->empty() ? std::string{} : values->front();
}

[[nodiscard]] std::string firstValueCaseInsensitive(const PropertyMap& properties, std::string_view name)
{
    const std::vector<std::string>* values = valuesForCaseInsensitive(properties, name);
    return values == nullptr || values->empty() ? std::string{} : values->front();
}

[[nodiscard]] std::optional<double> parseDouble(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    double result = 0.0;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const std::from_chars_result parsed = std::from_chars(begin, end, result);
    if (parsed.ec != std::errc{} || parsed.ptr != end || !std::isfinite(result)) {
        return std::nullopt;
    }
    return result;
}

[[nodiscard]] std::optional<int> parseInt(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    int result = 0;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const std::from_chars_result parsed = std::from_chars(begin, end, result);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return std::nullopt;
    }
    return result;
}

[[nodiscard]] std::optional<int> parseNonNegativeInt(std::string_view value)
{
    const auto parsed = parseInt(value);
    if (!parsed.has_value() || *parsed < 0) {
        return std::nullopt;
    }
    return parsed;
}

[[nodiscard]] std::optional<int> parsePositiveInt(std::string_view value)
{
    const auto parsed = parseInt(value);
    if (!parsed.has_value() || *parsed <= 0) {
        return std::nullopt;
    }
    return parsed;
}

[[nodiscard]] std::optional<Color> parsePlayerToMove(std::string_view value)
{
    if (value == "B" || value == "b") {
        return Color::Black;
    }
    if (value == "W" || value == "w") {
        return Color::White;
    }
    return std::nullopt;
}

[[nodiscard]] std::string playerToMoveValue(Color color)
{
    return color == Color::Black ? "B" : "W";
}

[[nodiscard]] BoardSize parseBoardSize(const PropertyMap& properties)
{
    const std::string value = firstValueCaseInsensitive(properties, "SZ");
    if (value.empty()) {
        return BoardSize::square(19);
    }

    const std::size_t separator = value.find(':');
    if (separator == std::string::npos) {
        if (const auto size = parseInt(value)) {
            return BoardSize::square(*size);
        }
        throw SgfError("invalid SZ property");
    }

    const auto width = parseInt(std::string_view(value).substr(0, separator));
    const auto height = parseInt(std::string_view(value).substr(separator + 1));
    if (!width.has_value() || !height.has_value()) {
        throw SgfError("invalid rectangular SZ property");
    }
    return BoardSize{*width, *height};
}

void appendPointOrRange(std::vector<Point>* points, std::string_view value, BoardSize boardSize)
{
    const std::size_t separator = value.find(':');
    if (separator == std::string_view::npos) {
        if (const auto point = pointFromSgf(value, boardSize)) {
            points->push_back(*point);
        }
        return;
    }

    const auto first = pointFromSgf(value.substr(0, separator), boardSize);
    const auto second = pointFromSgf(value.substr(separator + 1), boardSize);
    if (!first.has_value() || !second.has_value()) {
        return;
    }

    const int minX = std::min(first->x, second->x);
    const int maxX = std::max(first->x, second->x);
    const int minY = std::min(first->y, second->y);
    const int maxY = std::max(first->y, second->y);
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            points->push_back(Point{x, y});
        }
    }
}

void fillSetupStones(GameNode* node, const PropertyMap& properties, BoardSize boardSize)
{
    if (const std::vector<std::string>* values = valuesForCaseInsensitive(properties, "AB")) {
        for (const std::string& value : *values) {
            appendPointOrRange(&node->setupStones.black, value, boardSize);
        }
    }
    if (const std::vector<std::string>* values = valuesForCaseInsensitive(properties, "AW")) {
        for (const std::string& value : *values) {
            appendPointOrRange(&node->setupStones.white, value, boardSize);
        }
    }
    if (const std::vector<std::string>* values = valuesForCaseInsensitive(properties, "AE")) {
        for (const std::string& value : *values) {
            appendPointOrRange(&node->setupStones.empty, value, boardSize);
        }
    }
}

void fillMove(GameNode* node, const PropertyMap& properties, BoardSize boardSize)
{
    if (const std::vector<std::string>* values = valuesForCaseInsensitive(properties, "B");
        values != nullptr && !values->empty()) {
        node->move = moveFromSgf(Color::Black, values->front(), boardSize);
        return;
    }
    if (const std::vector<std::string>* values = valuesForCaseInsensitive(properties, "W");
        values != nullptr && !values->empty()) {
        node->move = moveFromSgf(Color::White, values->front(), boardSize);
    }
}

void fillPlayerToMove(GameNode* node, const PropertyMap& properties)
{
    if (const std::string value = firstValueCaseInsensitive(properties, "PL"); !value.empty()) {
        node->playerToMove = parsePlayerToMove(value);
    }
}

void appendMarks(
    std::vector<BoardMark>* marks,
    const PropertyMap& properties,
    const std::string& name,
    BoardMarkShape shape,
    BoardSize boardSize)
{
    const std::vector<std::string>* values = valuesForCaseInsensitive(properties, name);
    if (values == nullptr) {
        return;
    }
    std::vector<Point> points;
    for (const std::string& value : *values) {
        appendPointOrRange(&points, value, boardSize);
    }
    for (Point point : points) {
        marks->push_back(BoardMark{point, shape});
    }
}

void fillLabels(GameNode* node, const PropertyMap& properties, BoardSize boardSize)
{
    const std::vector<std::string>* values = valuesForCaseInsensitive(properties, "LB");
    if (values == nullptr) {
        return;
    }
    for (const std::string& value : *values) {
        const std::size_t separator = value.find(':');
        if (separator == std::string::npos) {
            continue;
        }
        const auto point = pointFromSgf(std::string_view(value).substr(0, separator), boardSize);
        if (!point.has_value()) {
            continue;
        }
        node->labels.push_back(BoardLabel{*point, value.substr(separator + 1)});
    }
}

void fillMarkup(GameNode* node, const PropertyMap& properties, BoardSize boardSize)
{
    fillLabels(node, properties, boardSize);
    appendMarks(&node->marks, properties, "CR", BoardMarkShape::Circle, boardSize);
    appendMarks(&node->marks, properties, "SQ", BoardMarkShape::Square, boardSize);
    appendMarks(&node->marks, properties, "TR", BoardMarkShape::Triangle, boardSize);
    appendMarks(&node->marks, properties, "MA", BoardMarkShape::Cross, boardSize);
    if (const std::string value = firstValueCaseInsensitive(properties, "MN"); !value.empty()) {
        node->moveNumber = parseNonNegativeInt(value);
    }
}

void fillAnalysisError(GameNode* node, const PropertyMap& properties)
{
    node->analysisError = firstValueCaseInsensitive(properties, "LZYERR");
}

void fillGameInfo(GameInfo* info, const PropertyMap& properties)
{
    info->blackPlayer = firstValueCaseInsensitive(properties, "PB");
    info->whitePlayer = firstValueCaseInsensitive(properties, "PW");
    info->blackRank = firstValueCaseInsensitive(properties, "BR");
    info->whiteRank = firstValueCaseInsensitive(properties, "WR");
    info->result = firstValueCaseInsensitive(properties, "RE");
    info->date = firstValueCaseInsensitive(properties, "DT");
    info->source = firstValueCaseInsensitive(properties, "SO");
    info->rules = firstValueCaseInsensitive(properties, "RU");
    if (const std::string value = firstValueCaseInsensitive(properties, "KM"); !value.empty()) {
        info->komi = parseDouble(value);
    }
    if (const std::string value = firstValueCaseInsensitive(properties, "HA"); !value.empty()) {
        info->handicap = parsePositiveInt(value);
    }
}

void applyRawNode(GameModel* model, NodeId id, const RawNode& raw, bool isRoot)
{
    GameNode& node = model->node(id);
    node.properties = raw.properties;
    node.comment = firstValueCaseInsensitive(raw.properties, "C");
    fillSetupStones(&node, raw.properties, model->boardSize());
    node.setupStones = normalizedSetupStones(node.setupStones);
    fillMove(&node, raw.properties, model->boardSize());
    fillPlayerToMove(&node, raw.properties);
    fillMarkup(&node, raw.properties, model->boardSize());
    fillAnalysisError(&node, raw.properties);
    if (isRoot) {
        fillGameInfo(&model->gameInfo(), raw.properties);
    }

    for (const std::unique_ptr<RawNode>& child : raw.children) {
        const NodeId childId = model->addChild(id);
        applyRawNode(model, childId, *child, false);
    }
}

[[nodiscard]] GameModel buildGameModel(const RawNode& raw)
{
    BoardSize boardSize = parseBoardSize(raw.properties);
    if (!boardSize.isValid()) {
        throw SgfError("invalid board size");
    }

    GameModel model(boardSize);
    applyRawNode(&model, model.rootId(), raw, true);
    return model;
}

void setSingle(PropertyMap* properties, const std::string& name, std::string value)
{
    erasePropertyCaseInsensitive(properties, name);
    if (value.empty()) {
        return;
    }
    (*properties)[name] = {std::move(value)};
}

void setOptionalDouble(PropertyMap* properties, const std::string& name, std::optional<double> value)
{
    erasePropertyCaseInsensitive(properties, name);
    if (!value.has_value() || !std::isfinite(*value)) {
        return;
    }
    std::ostringstream stream;
    stream << *value;
    (*properties)[name] = {stream.str()};
}

void setOptionalNonNegativeInt(PropertyMap* properties, const std::string& name, std::optional<int> value)
{
    erasePropertyCaseInsensitive(properties, name);
    if (!value.has_value() || *value < 0) {
        return;
    }
    (*properties)[name] = {std::to_string(*value)};
}

void setOptionalPositiveInt(PropertyMap* properties, const std::string& name, std::optional<int> value)
{
    erasePropertyCaseInsensitive(properties, name);
    if (!value.has_value() || *value <= 0) {
        return;
    }
    (*properties)[name] = {std::to_string(*value)};
}

[[nodiscard]] std::vector<std::string> pointsToValues(const std::vector<Point>& points)
{
    std::vector<std::string> values;
    values.reserve(points.size());
    for (Point point : points) {
        values.push_back(pointToSgf(point));
    }
    return values;
}

[[nodiscard]] std::vector<std::string> labelsToValues(const std::vector<BoardLabel>& labels)
{
    std::vector<std::string> values;
    values.reserve(labels.size());
    for (const BoardLabel& label : labels) {
        values.push_back(pointToSgf(label.point) + ":" + label.text);
    }
    return values;
}

[[nodiscard]] std::vector<BoardLabel> normalizedLabels(const std::vector<BoardLabel>& labels)
{
    std::vector<BoardLabel> normalized;
    normalized.reserve(labels.size());
    for (const BoardLabel& label : labels) {
        normalized.erase(
            std::remove_if(
                normalized.begin(),
                normalized.end(),
                [point = label.point](const BoardLabel& existing) {
                    return existing.point == point;
                }),
            normalized.end());
        normalized.push_back(label);
    }
    return normalized;
}

[[nodiscard]] std::vector<BoardMark> normalizedMarks(const std::vector<BoardMark>& marks)
{
    std::vector<BoardMark> normalized;
    normalized.reserve(marks.size());
    for (const BoardMark& mark : marks) {
        normalized.erase(
            std::remove_if(
                normalized.begin(),
                normalized.end(),
                [point = mark.point](const BoardMark& existing) {
                    return existing.point == point;
                }),
            normalized.end());
        normalized.push_back(mark);
    }
    return normalized;
}

[[nodiscard]] std::vector<std::string> marksToValues(
    const std::vector<BoardMark>& marks,
    BoardMarkShape shape)
{
    std::vector<std::string> values;
    for (const BoardMark& mark : marks) {
        if (mark.shape == shape) {
            values.push_back(pointToSgf(mark.point));
        }
    }
    return values;
}

void setMulti(PropertyMap* properties, const std::string& name, std::vector<std::string> values)
{
    erasePropertyCaseInsensitive(properties, name);
    if (values.empty()) {
        return;
    }
    (*properties)[name] = std::move(values);
}

void setRequired(PropertyMap* properties, const std::string& name, std::string value)
{
    erasePropertyCaseInsensitive(properties, name);
    (*properties)[name] = {std::move(value)};
}

[[nodiscard]] std::string formatFixed(double value, int decimals)
{
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << value;
    return stream.str();
}

[[nodiscard]] double boundedRate(double value)
{
    if (!std::isfinite(value)) {
        return 0.0;
    }
    return std::clamp(value, 0.0, 1.0);
}

[[nodiscard]] std::string pointToGtp(Point point, BoardSize boardSize)
{
    static constexpr std::string_view columns = "ABCDEFGHJKLMNOPQRSTUVWXYZ";
    if (point.x < 0 || point.x >= boardSize.width || point.x >= static_cast<int>(columns.size()) || point.y < 0 ||
        point.y >= boardSize.height) {
        return {};
    }
    return std::string{columns[static_cast<std::size_t>(point.x)]} + std::to_string(boardSize.height - point.y);
}

[[nodiscard]] std::string moveToGtp(const Move& move, BoardSize boardSize)
{
    switch (move.type) {
    case MoveType::Play:
        return pointToGtp(move.point, boardSize);
    case MoveType::Pass:
        return "pass";
    case MoveType::Resign:
        return "resign";
    }
    return "pass";
}

[[nodiscard]] std::string moveToAnalysisText(const Move& move, BoardSize boardSize)
{
    if (move.type != MoveType::Play) {
        return moveToGtp(move, boardSize);
    }
    if (boardSize.width <= 25) {
        const std::string text = moveToGtp(move, boardSize);
        if (!text.empty()) {
            return text;
        }
    }
    return moveToSgf(move);
}

[[nodiscard]] std::string compactVisits(int visits)
{
    return std::to_string(std::max(0, visits));
}

[[nodiscard]] int legacyRate(double value)
{
    return static_cast<int>(std::llround(boundedRate(value) * 10000.0));
}

[[nodiscard]] bool hasCompleteOwnership(const std::vector<double>& ownership, BoardSize boardSize)
{
    return ownership.size() == static_cast<std::size_t>(boardSize.pointCount()) &&
        std::ranges::all_of(ownership, [](double value) { return std::isfinite(value); });
}

[[nodiscard]] std::string analysisLegacyPayload(const AnalysisSnapshot& analysis, BoardSize boardSize)
{
    const double rootWinrate = boundedRate(analysis.rootWinrate);
    const double headerWinrate = (1.0 - std::clamp(rootWinrate, 0.0, 1.0)) * 100.0;
    const double scoreStdev = analysis.candidates.empty() ? 0.0 : analysis.candidates.front().scoreStdev;
    const bool writeRootOwnership = hasCompleteOwnership(analysis.ownership, boardSize);

    std::ostringstream output;
    output << "LizzieYzyQt " << formatFixed(headerWinrate, 1) << ' ' << compactVisits(analysis.rootVisits) << ' '
           << formatFixed(analysis.rootScoreMean, 2) << ' ' << formatFixed(scoreStdev, 2);

    if (!analysis.candidates.empty() || writeRootOwnership) {
        output << '\n';
    }
    for (std::size_t index = 0; index < analysis.candidates.size(); ++index) {
        const MoveCandidate& candidate = analysis.candidates[index];
        const bool writeCandidateOwnership = hasCompleteOwnership(candidate.ownership, boardSize);
        if (index > 0) {
            output << " info ";
        }
        output << "move " << moveToAnalysisText(candidate.move, boardSize) << " visits " << compactVisits(candidate.visits)
               << " winrate " << legacyRate(candidate.winrate) << " prior " << legacyRate(candidate.policy)
               << " scoreMean " << formatFixed(candidate.scoreMean, 2) << " scoreStdev "
               << formatFixed(candidate.scoreStdev, 2) << " pv";
        for (const Move& move : candidate.pv) {
            output << ' ' << moveToAnalysisText(move, boardSize);
        }
        if (!candidate.pvVisits.empty()) {
            output << " pvVisits";
            for (int visits : candidate.pvVisits) {
                output << ' ' << compactVisits(visits);
            }
        }
        if (writeCandidateOwnership) {
            output << " movesOwnership";
            for (double value : candidate.ownership) {
                output << ' ' << formatFixed(value, 6);
            }
        }
    }
    if (writeRootOwnership) {
        output << " ownership";
        for (double value : analysis.ownership) {
            output << ' ' << formatFixed(value, 6);
        }
    }
    return output.str();
}

[[nodiscard]] std::size_t generatedCommentPosition(const std::string& comment, std::string_view marker)
{
    const std::string separated = "\n\n" + std::string(marker);
    if (const std::size_t position = comment.find(separated); position != std::string::npos) {
        return position;
    }
    if (comment.starts_with(marker)) {
        return 0;
    }
    return std::string::npos;
}

[[nodiscard]] std::string stripGeneratedAnalysisComment(std::string comment)
{
    const std::size_t analysisPosition = generatedCommentPosition(comment, "LizzieYzy analysis\n");
    const std::size_t errorPosition = generatedCommentPosition(comment, "LizzieYzy analysis error\n");
    const std::size_t position = std::min(analysisPosition, errorPosition);
    if (position == std::string::npos) {
        return comment;
    }
    if (position == 0) {
        return {};
    }
    comment.erase(position);
    return comment;
}

void appendCommentSection(std::string* comment, std::string_view section)
{
    if (section.empty()) {
        return;
    }
    if (!comment->empty()) {
        *comment += "\n\n";
    }
    comment->append(section);
}

[[nodiscard]] std::string analysisComment(const AnalysisSnapshot& analysis, BoardSize boardSize)
{
    std::ostringstream output;
    output << "LizzieYzy analysis\n";
    output << "WinratePerspective: " << playerToMoveValue(analysis.winratePerspective.value_or(Color::Black))
           << "\n";
    output << "Winrate: " << formatFixed(boundedRate(analysis.rootWinrate) * 100.0, 1) << "%\n";
    output << "ScoreMean: " << formatFixed(analysis.rootScoreMean, 2) << "\n";
    output << "Visits: " << compactVisits(analysis.rootVisits);
    if (hasCompleteOwnership(analysis.ownership, boardSize)) {
        output << "\nOwnership:";
        for (double value : analysis.ownership) {
            output << ' ' << formatFixed(value, 6);
        }
    }
    if (!analysis.candidates.empty()) {
        output << "\nCandidates:";
    }
    const std::size_t limit = std::min<std::size_t>(analysis.candidates.size(), 5);
    for (std::size_t index = 0; index < limit; ++index) {
        const MoveCandidate& candidate = analysis.candidates[index];
        output << "\n" << moveToAnalysisText(candidate.move, boardSize) << " visits "
               << compactVisits(candidate.visits) << " winrate " << formatFixed(boundedRate(candidate.winrate) * 100.0, 1)
               << "% score " << formatFixed(candidate.scoreMean, 2) << " stdev "
               << formatFixed(candidate.scoreStdev, 2) << " policy "
               << formatFixed(boundedRate(candidate.policy) * 100.0, 1) << "%";
        if (!candidate.pv.empty()) {
            output << " pv";
            for (const Move& move : candidate.pv) {
                output << ' ' << moveToAnalysisText(move, boardSize);
            }
        }
        if (!candidate.pvVisits.empty()) {
            output << " pvVisits";
            for (int visits : candidate.pvVisits) {
                output << ' ' << compactVisits(visits);
            }
        }
        if (hasCompleteOwnership(candidate.ownership, boardSize)) {
            output << " movesOwnership";
            for (double value : candidate.ownership) {
                output << ' ' << formatFixed(value, 6);
            }
        }
    }
    return output.str();
}

[[nodiscard]] std::string analysisErrorComment(std::string_view error)
{
    std::string comment = "LizzieYzy analysis error\n";
    comment.append(error);
    return comment;
}

void setAnalysisProperties(PropertyMap* properties, const GameNode& node, BoardSize boardSize, bool isRoot)
{
    std::string comment = stripGeneratedAnalysisComment(node.comment);
    if (!node.analysisError.empty()) {
        appendCommentSection(&comment, analysisErrorComment(node.analysisError));
        erasePropertyCaseInsensitive(properties, "LZ");
        erasePropertyCaseInsensitive(properties, "LZOP");
        erasePropertyCaseInsensitive(properties, "LZYERR");
        erasePropertyCaseInsensitive(properties, kWinratePerspectiveProperty);
        (*properties)["LZYERR"] = {node.analysisError};
    } else {
        erasePropertyCaseInsensitive(properties, "LZYERR");
        if (node.analysis.has_value()) {
            appendCommentSection(&comment, analysisComment(*node.analysis, boardSize));
            erasePropertyCaseInsensitive(properties, isRoot ? "LZ" : "LZOP");
            erasePropertyCaseInsensitive(properties, isRoot ? "LZOP" : "LZ");
            erasePropertyCaseInsensitive(properties, kWinratePerspectiveProperty);
            (*properties)[std::string(kWinratePerspectiveProperty)] = {
                playerToMoveValue(node.analysis->winratePerspective.value_or(Color::Black))};
            (*properties)[isRoot ? "LZOP" : "LZ"] = {analysisLegacyPayload(*node.analysis, boardSize)};
        }
    }

    if (comment.empty()) {
        erasePropertyCaseInsensitive(properties, "C");
    } else {
        erasePropertyCaseInsensitive(properties, "C");
        (*properties)["C"] = {std::move(comment)};
    }
}

[[nodiscard]] PropertyMap propertiesForWrite(const GameModel& model, const GameNode& node, bool isRoot)
{
    PropertyMap properties = node.properties;

    if (isRoot) {
        setRequired(&properties, "GM", "1");
        setRequired(&properties, "FF", "4");
        setRequired(
            &properties,
            "SZ",
            model.boardSize().isSquare()
                ? std::to_string(model.boardSize().width)
                : std::to_string(model.boardSize().width) + ":" + std::to_string(model.boardSize().height));

        const GameInfo& info = model.gameInfo();
        setSingle(&properties, "PB", info.blackPlayer);
        setSingle(&properties, "PW", info.whitePlayer);
        setSingle(&properties, "BR", info.blackRank);
        setSingle(&properties, "WR", info.whiteRank);
        setSingle(&properties, "RE", info.result);
        setSingle(&properties, "DT", info.date);
        setSingle(&properties, "SO", info.source);
        setSingle(&properties, "RU", info.rules);
        setOptionalDouble(&properties, "KM", info.komi);
        setOptionalPositiveInt(&properties, "HA", info.handicap);
    }

    if (node.move.has_value()) {
        erasePropertyCaseInsensitive(&properties, "B");
        erasePropertyCaseInsensitive(&properties, "W");
        properties[node.move->color == Color::Black ? "B" : "W"] = {moveToSgf(*node.move)};
    } else {
        erasePropertyCaseInsensitive(&properties, "B");
        erasePropertyCaseInsensitive(&properties, "W");
    }
    if (node.playerToMove.has_value()) {
        erasePropertyCaseInsensitive(&properties, "PL");
        properties["PL"] = {playerToMoveValue(*node.playerToMove)};
    } else {
        erasePropertyCaseInsensitive(&properties, "PL");
    }

    const SetupStones setup = normalizedSetupStones(node.setupStones);
    setMulti(&properties, "AB", pointsToValues(setup.black));
    setMulti(&properties, "AW", pointsToValues(setup.white));
    setMulti(&properties, "AE", pointsToValues(setup.empty));
    const std::vector<BoardLabel> labels = normalizedLabels(node.labels);
    const std::vector<BoardMark> marks = normalizedMarks(node.marks);
    setMulti(&properties, "LB", labelsToValues(labels));
    setMulti(&properties, "CR", marksToValues(marks, BoardMarkShape::Circle));
    setMulti(&properties, "SQ", marksToValues(marks, BoardMarkShape::Square));
    setMulti(&properties, "TR", marksToValues(marks, BoardMarkShape::Triangle));
    setMulti(&properties, "MA", marksToValues(marks, BoardMarkShape::Cross));
    setOptionalNonNegativeInt(&properties, "MN", node.moveNumber);

    setAnalysisProperties(&properties, node, model.boardSize(), isRoot);
    return properties;
}

[[nodiscard]] std::string escapeValue(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char c : value) {
        if (c == '\\' || c == ']') {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }
    return escaped;
}

void appendProperties(std::string* output, const PropertyMap& properties)
{
    for (const auto& [name, values] : properties) {
        if (!isWritablePropertyName(name)) {
            continue;
        }
        if (values.empty()) {
            *output += name;
            *output += "[]";
            continue;
        }
        *output += name;
        for (const std::string& value : values) {
            *output += '[';
            *output += escapeValue(value);
            *output += ']';
        }
    }
}

void appendSubtree(std::string* output, const GameModel& model, NodeId id)
{
    const GameNode& node = model.node(id);
    output->push_back(';');
    appendProperties(output, propertiesForWrite(model, node, id == model.rootId()));

    if (node.children.size() == 1) {
        appendSubtree(output, model, node.children.front());
        return;
    }

    for (NodeId childId : node.children) {
        output->push_back('(');
        appendSubtree(output, model, childId);
        output->push_back(')');
    }
}

}  // namespace

SgfError::SgfError(const std::string& message)
    : std::runtime_error(message)
{
}

GameModel SgfParser::parseGame(std::string_view input) const
{
    std::vector<GameModel> collection = parseCollection(input);
    if (collection.empty()) {
        throw SgfError("SGF collection is empty");
    }
    return std::move(collection.front());
}

std::vector<GameModel> SgfParser::parseCollection(std::string_view input) const
{
    Parser parser(input);
    std::vector<std::unique_ptr<RawNode>> rawGames = parser.parseCollection();
    std::vector<GameModel> games;
    games.reserve(rawGames.size());
    for (const std::unique_ptr<RawNode>& raw : rawGames) {
        games.push_back(buildGameModel(*raw));
    }
    return games;
}

std::string writeSgf(const GameModel& model)
{
    std::string output;
    output.push_back('(');
    appendSubtree(&output, model, model.rootId());
    output.push_back(')');
    output.push_back('\n');
    return output;
}

std::string writeSgfCollection(const std::vector<GameModel>& models)
{
    std::string output;
    for (const GameModel& model : models) {
        output += writeSgf(model);
    }
    return output;
}

}  // namespace lizzie::core
