#include "BoardPosition.h"
#include "BoardQuickItem.h"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QEventLoop>
#include <QMouseEvent>
#include <QPainter>
#include <QTemporaryDir>
#include <QTimer>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int countPreviewWhitePixels(const QImage& image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.alpha() > 180 && color.red() > 235 && color.green() > 230 && color.blue() > 210) {
                ++count;
            }
        }
    }
    return count;
}

int countPixelsMatching(const QImage& image, bool (*predicate)(const QColor&))
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (predicate(image.pixelColor(x, y))) {
                ++count;
            }
        }
    }
    return count;
}

bool isCandidatePixel(const QColor& color);
bool isPrimaryCandidateHighlightPixel(const QColor& color);

QPoint pointPixelCenter(lizzie::core::Point point, lizzie::core::BoardSize boardSize, QSize imageSize)
{
    const QRectF area = QRectF(QPointF(0.0, 0.0), QSizeF(imageSize)).adjusted(18.0, 18.0, -18.0, -18.0);
    const int columns = std::max(1, boardSize.width - 1);
    const int rows = std::max(1, boardSize.height - 1);
    const qreal cell = std::min(area.width() / columns, area.height() / rows);
    const QSizeF gridSize(cell * columns, cell * rows);
    const QRectF grid(
        QPointF(area.center().x() - gridSize.width() / 2.0, area.center().y() - gridSize.height() / 2.0),
        gridSize);
    return QPoint(
        static_cast<int>(std::llround(grid.left() + point.x * cell)),
        static_cast<int>(std::llround(grid.top() + point.y * cell)));
}

int countCandidatePixelsNear(
    const QImage& image,
    lizzie::core::Point point,
    lizzie::core::BoardSize boardSize,
    int radius)
{
    const QPoint center = pointPixelCenter(point, boardSize, image.size());
    int count = 0;
    for (int y = std::max(0, center.y() - radius); y <= std::min(image.height() - 1, center.y() + radius); ++y) {
        for (int x = std::max(0, center.x() - radius); x <= std::min(image.width() - 1, center.x() + radius); ++x) {
            if (isCandidatePixel(image.pixelColor(x, y))) {
                ++count;
            }
        }
    }
    return count;
}

int countPrimaryCandidateHighlightPixelsNear(
    const QImage& image,
    lizzie::core::Point point,
    lizzie::core::BoardSize boardSize,
    int radius)
{
    const QPoint center = pointPixelCenter(point, boardSize, image.size());
    int count = 0;
    for (int y = std::max(0, center.y() - radius); y <= std::min(image.height() - 1, center.y() + radius); ++y) {
        for (int x = std::max(0, center.x() - radius); x <= std::min(image.width() - 1, center.x() + radius); ++x) {
            if (isPrimaryCandidateHighlightPixel(image.pixelColor(x, y))) {
                ++count;
            }
        }
    }
    return count;
}

bool isCandidatePixel(const QColor& color)
{
    return color.alpha() > 120 && color.green() > 150 && color.red() < 90 && color.blue() > 120;
}

bool isPrimaryCandidateHighlightPixel(const QColor& color)
{
    return color.alpha() > 180 &&
        color.red() > 185 &&
        color.green() > 160 &&
        color.blue() < 140 &&
        color.red() > color.blue() + 80;
}

bool isCandidateLabelPixel(const QColor& color)
{
    return color.alpha() > 150 &&
        color.red() < 35 &&
        color.green() > 25 && color.green() < 85 &&
        color.blue() > 25 && color.blue() < 85;
}

bool isOwnershipPixel(const QColor& color)
{
    return color.alpha() > 180 && color.blue() > color.red() + 20 && color.blue() > color.green() + 10;
}

bool isDarkTextOrLinePixel(const QColor& color)
{
    return color.alpha() > 180 && color.red() < 70 && color.green() < 70 && color.blue() < 70;
}

bool isBrightLabelPixel(const QColor& color)
{
    return color.alpha() > 150 && color.red() > 230 && color.green() > 230 && color.blue() > 230;
}

bool isRedTexturePixel(const QColor& color)
{
    return color.alpha() > 220 && color.red() > 200 && color.green() < 80 && color.blue() < 80;
}

bool isBlueTexturePixel(const QColor& color)
{
    return color.alpha() > 220 && color.blue() > 200 && color.red() < 80 && color.green() < 120;
}

QImage renderBoard(lizzie::ui::BoardQuickItem* board, QSize size, qreal devicePixelRatio = 1.0)
{
    board->setWidth(size.width());
    board->setHeight(size.height());
    const QSize pixelSize(
        std::max(1, static_cast<int>(std::ceil(size.width() * devicePixelRatio))),
        std::max(1, static_cast<int>(std::ceil(size.height() * devicePixelRatio))));
    QImage image(pixelSize, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatio);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    board->paint(&painter);
    painter.end();
    return image;
}

QRect boundsForPixels(const QImage& image, bool (*predicate)(const QColor&))
{
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (!predicate(image.pixelColor(x, y))) {
                continue;
            }
            const QRect pixelBounds(x, y, 1, 1);
            bounds = bounds.isNull() ? pixelBounds : bounds.united(pixelBounds);
        }
    }
    return bounds;
}

int countDifferingPixels(const QImage& first, const QImage& second)
{
    if (first.size() != second.size()) {
        return -1;
    }
    int count = 0;
    for (int y = 0; y < first.height(); ++y) {
        for (int x = 0; x < first.width(); ++x) {
            const QColor lhs = first.pixelColor(x, y);
            const QColor rhs = second.pixelColor(x, y);
            const int delta = std::abs(lhs.red() - rhs.red()) +
                std::abs(lhs.green() - rhs.green()) +
                std::abs(lhs.blue() - rhs.blue()) +
                std::abs(lhs.alpha() - rhs.alpha());
            if (delta > 12) {
                ++count;
            }
        }
    }
    return count;
}

void waitForBoardAnimationTick()
{
    QEventLoop loop;
    QTimer::singleShot(280, &loop, &QEventLoop::quit);
    loop.exec();
}

}  // namespace

int main(int argc, char* argv[])
{
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }

    QApplication app(argc, argv);

    try {
        lizzie::ui::BoardQuickItem board;
        board.setWidth(360);
        board.setHeight(360);
        board.setPosition(lizzie::core::BoardPosition(lizzie::core::BoardSize::square(9)));
        board.setPreviewMoves({
            lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 2}),
            lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{3, 2}),
            lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 3}),
        });
        require(board.previewMoveCount() == 3, "preview moves should be retained");

        QImage image = renderBoard(&board, QSize(360, 360));

        require(countPreviewWhitePixels(image) > 40, "PV preview should render white preview stones");

        int clickedX = -1;
        int clickedY = -1;
        QObject::connect(&board, &lizzie::ui::BoardQuickItem::pointClicked, [&](int x, int y) {
            clickedX = x;
            clickedY = y;
        });
        QMouseEvent click(
            QEvent::MouseButtonPress,
            QPointF(99.0, 99.0),
            QPointF(99.0, 99.0),
            QPointF(99.0, 99.0),
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier);
        QApplication::sendEvent(&board, &click);
        require(clickedX == 2 && clickedY == 2, "board click should map device-independent coordinates to points");

        board.clearOverlay();
        require(board.previewMoveCount() == 0, "clearOverlay should clear PV preview moves");
        board.setCandidates({
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{30, 30}), 999999},
        });
        const QImage invalidCandidateImage = renderBoard(&board, QSize(360, 360));
        require(
            countPixelsMatching(invalidCandidateImage, isCandidatePixel) == 0,
            "off-board candidates should not render or animate");
        board.setPosition(lizzie::core::BoardPosition(lizzie::core::BoardSize{52, 52}));
        const QImage resizedCandidateImage = renderBoard(&board, QSize(720, 720));
        waitForBoardAnimationTick();
        const QImage resizedAnimatedCandidateImage = renderBoard(&board, QSize(720, 720));
        require(
            countDifferingPixels(resizedCandidateImage, resizedAnimatedCandidateImage) > 10,
            "newly visible candidates should animate after a board resize");
        board.clearOverlay();
        const QImage resizedNoCandidateImage = renderBoard(&board, QSize(720, 720));
        require(
            countDifferingPixels(resizedCandidateImage, resizedNoCandidateImage) > 10,
            "candidates should be re-evaluated after a board resize makes them visible");
        board.setPosition(lizzie::core::BoardPosition(lizzie::core::BoardSize::square(9)));
        board.setCandidates({
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{4, 4}), 123456789},
        });
        const QImage compactCandidateImage = renderBoard(&board, QSize(360, 360));
        require(
            countPixelsMatching(compactCandidateImage, isCandidatePixel) > 40,
            "large visit-count candidates should still render the candidate marker");
        require(
            countPixelsMatching(compactCandidateImage, isCandidateLabelPixel) > 2,
            "large visit-count candidates should render a fitted compact label");
        board.setCandidates({
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 3}), 2048},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{5, 5}), 1024},
        });
        const QImage primaryCandidateImage = renderBoard(&board, QSize(360, 360));
        require(
            countPrimaryCandidateHighlightPixelsNear(
                primaryCandidateImage,
                lizzie::core::Point{3, 3},
                lizzie::core::BoardSize::square(9),
                18) > 20,
            "the first board candidate should render a distinct highlight ring");
        require(
            countPrimaryCandidateHighlightPixelsNear(
                primaryCandidateImage,
                lizzie::core::Point{5, 5},
                lizzie::core::BoardSize::square(9),
                18) < 5,
            "secondary board candidates should not render the primary highlight ring");
        board.setCandidates({
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{1, 1}), 110},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 1}), 109},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 1}), 108},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{4, 1}), 107},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{5, 1}), 106},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{6, 1}), 105},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{7, 1}), 104},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{1, 2}), 103},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{2, 2}), 102},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 2}), 101},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{4, 2}), 100},
        });
        const QImage cappedCandidateImage = renderBoard(&board, QSize(360, 360));
        require(
            countCandidatePixelsNear(
                cappedCandidateImage,
                lizzie::core::Point{3, 2},
                lizzie::core::BoardSize::square(9),
                14) > 20,
            "the tenth board candidate marker should render");
        require(
            countCandidatePixelsNear(
                cappedCandidateImage,
                lizzie::core::Point{4, 2},
                lizzie::core::BoardSize::square(9),
                14) == 0,
            "board candidate markers should be capped at the first ten candidates");
        waitForBoardAnimationTick();
        const QImage animatedCandidateImage = renderBoard(&board, QSize(360, 360));
        require(
            countDifferingPixels(compactCandidateImage, animatedCandidateImage) > 20,
            "candidate markers should animate between timer ticks");
        board.clearOverlay();

        QTemporaryDir textureDir;
        require(textureDir.isValid(), "temporary texture directory should be created");
        const QString blackTexturePath = textureDir.filePath("black.png");
        const QString whiteTexturePath = textureDir.filePath("white.png");
        QImage blackTexture(32, 32, QImage::Format_ARGB32_Premultiplied);
        blackTexture.fill(QColor(230, 20, 20));
        QImage whiteTexture(32, 32, QImage::Format_ARGB32_Premultiplied);
        whiteTexture.fill(QColor(20, 60, 230));
        require(blackTexture.save(blackTexturePath), "black stone texture should save");
        require(whiteTexture.save(whiteTexturePath), "white stone texture should save");

        lizzie::core::BoardPosition texturedPosition(lizzie::core::BoardSize::square(9));
        texturedPosition.placeSetupStone(lizzie::core::Color::Black, lizzie::core::Point{2, 2});
        texturedPosition.placeSetupStone(lizzie::core::Color::White, lizzie::core::Point{6, 6});
        board.setPosition(texturedPosition);
        board.setStoneTextures(blackTexturePath, whiteTexturePath);
        QImage texturedImage = renderBoard(&board, QSize(360, 360));
        require(
            isRedTexturePixel(texturedImage.pixelColor(99, 99)),
            "black stone should render from configured texture");
        require(
            isBlueTexturePixel(texturedImage.pixelColor(261, 261)),
            "white stone should render from configured texture");

        board.setStoneTextures(textureDir.filePath("missing-black.png"), textureDir.filePath("missing-white.png"));
        QImage fallbackImage = renderBoard(&board, QSize(360, 360));
        require(
            !isRedTexturePixel(fallbackImage.pixelColor(99, 99)) && !isBlueTexturePixel(fallbackImage.pixelColor(261, 261)),
            "missing texture paths should fall back to generated stones");

        lizzie::core::BoardPosition longLabelPosition(lizzie::core::BoardSize::square(9));
        longLabelPosition.placeSetupStone(lizzie::core::Color::Black, lizzie::core::Point{4, 4});
        board.setPosition(longLabelPosition);
        board.setLabels({lizzie::core::BoardLabel{lizzie::core::Point{4, 4}, "WIDE100"}});
        QImage longLabelImage = renderBoard(&board, QSize(360, 360));
        const QRect longLabelBounds = boundsForPixels(longLabelImage, isBrightLabelPixel);
        require(!longLabelBounds.isNull(), "long SGF labels should remain visible on black stones");
        require(
            longLabelBounds.width() <= 34 && longLabelBounds.height() <= 18,
            "long SGF labels should fit inside the stone marker; bounds were " +
                std::to_string(longLabelBounds.width()) + "x" + std::to_string(longLabelBounds.height()));
        board.setLabels({});

        lizzie::core::BoardPosition rectangularPosition(lizzie::core::BoardSize{9, 13});
        rectangularPosition.placeSetupStone(lizzie::core::Color::Black, lizzie::core::Point{0, 12});
        rectangularPosition.placeSetupStone(lizzie::core::Color::White, lizzie::core::Point{8, 0});
        board.setPosition(rectangularPosition);
        board.setShowOwnership(true);
        board.setOwnership(std::vector<double>(9 * 13, 1.0));
        board.setCandidates({
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{8, 12}), 2048},
        });
        board.setPreviewMoves({
            lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{0, 12}),
            lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{8, 0}),
        });
        require(board.boardWidth() == 9 && board.boardHeight() == 13, "rectangular board size should be exposed");
        const QImage rectangularImage = renderBoard(&board, QSize(360, 520));
        const int rectangularOwnershipPixels = countPixelsMatching(rectangularImage, isOwnershipPixel);
        const int rectangularCandidatePixels = countPixelsMatching(rectangularImage, isCandidatePixel);
        require(
            rectangularOwnershipPixels > 3000,
            "rectangular board should render ownership using width*height indexing; got " +
                std::to_string(rectangularOwnershipPixels));
        require(
            rectangularCandidatePixels > 30,
            "rectangular board should render candidates on the bottom edge; got " +
                std::to_string(rectangularCandidatePixels));
        clickedX = -1;
        clickedY = -1;
        QMouseEvent rectangularClick(
            QEvent::MouseButtonPress,
            QPointF(19.0, 502.0),
            QPointF(19.0, 502.0),
            QPointF(19.0, 502.0),
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier);
        QApplication::sendEvent(&board, &rectangularClick);
        require(clickedX == 0 && clickedY == 12, "rectangular board click should map the bottom row");
        board.clearOverlay();

        lizzie::core::BoardPosition largePosition(lizzie::core::BoardSize::square(19));
        largePosition.playMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{3, 3}));
        largePosition.playMove(lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{15, 15}));
        largePosition.playMove(lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{16, 3}));
        largePosition.playMove(lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{3, 16}));
        board.setPosition(largePosition);
        std::vector<double> ownership(19 * 19, 1.0);
        for (std::size_t index = 1; index < ownership.size(); index += 2) {
            ownership[index] = -1.0;
        }
        board.setOwnershipOpacity(0.8);
        board.setOwnership(std::move(ownership));
        require(!board.showOwnership(), "ownership data should not override the display visibility setting");
        board.setShowOwnership(false);
        const QImage hiddenOwnershipImage = renderBoard(&board, QSize(640, 640));
        require(
            countPixelsMatching(hiddenOwnershipImage, isOwnershipPixel) < 100,
            "ownership overlay should not render when display is disabled");
        board.setShowOwnership(true);
        board.setLabels({lizzie::core::BoardLabel{lizzie::core::Point{10, 10}, "A"}});
        board.setMarks({lizzie::core::BoardMark{lizzie::core::Point{9, 9}, lizzie::core::BoardMarkShape::Triangle}});
        board.setPreviewMoves({
            lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{10, 9}),
            lizzie::core::Move::play(lizzie::core::Color::White, lizzie::core::Point{10, 8}),
        });
        board.setCandidates({
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{4, 4}), 320},
            lizzie::core::MoveCandidate{lizzie::core::Move::play(lizzie::core::Color::Black, lizzie::core::Point{14, 14}), 128},
        });

        const QImage largeImage = renderBoard(&board, QSize(3840, 2160));
        const int darkPixels = countPixelsMatching(largeImage, isDarkTextOrLinePixel);
        const int ownershipPixels = countPixelsMatching(largeImage, isOwnershipPixel);
        const int candidatePixels = countPixelsMatching(largeImage, isCandidatePixel);
        require(darkPixels > 20000,
            "4K board should render crisp grid, stones, and text pixels; got " + std::to_string(darkPixels));
        require(ownershipPixels > 100000,
            "4K board should render ownership overlay pixels; got " + std::to_string(ownershipPixels));
        require(candidatePixels > 5000,
            "4K board should render candidate overlay pixels; got " + std::to_string(candidatePixels));

        const QImage highDpiImage = renderBoard(&board, QSize(900, 900), 2.0);
        const int highDpiDarkPixels = countPixelsMatching(highDpiImage, isDarkTextOrLinePixel);
        const int highDpiOwnershipPixels = countPixelsMatching(highDpiImage, isOwnershipPixel);
        const int highDpiCandidatePixels = countPixelsMatching(highDpiImage, isCandidatePixel);
        const int highDpiLabelPixels = countPixelsMatching(highDpiImage, isCandidateLabelPixel);
        require(highDpiDarkPixels > 12000,
            "2x high-DPI board should render grid, stones, and text in device-independent coordinates; got " +
                std::to_string(highDpiDarkPixels));
        require(highDpiOwnershipPixels > 80000,
            "2x high-DPI board should render ownership overlay pixels; got " +
                std::to_string(highDpiOwnershipPixels));
        require(highDpiCandidatePixels > 1800,
            "2x high-DPI board should render candidate overlay pixels; got " +
                std::to_string(highDpiCandidatePixels));
        require(highDpiLabelPixels > 12,
            "2x high-DPI board should keep fitted candidate labels visible; got " +
                std::to_string(highDpiLabelPixels));
    } catch (const std::exception& error) {
        std::cerr << "board_quick_item_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "board_quick_item_tests passed\n";
    return EXIT_SUCCESS;
}
