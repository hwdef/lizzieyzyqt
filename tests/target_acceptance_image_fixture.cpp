#include <QCoreApplication>
#include <QImage>
#include <QStringList>

#include <iostream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments();
    if (args.size() != 2) {
        std::cerr << "usage: " << argv[0] << " <output-image>\n";
        return 1;
    }

    QImage image(3840, 2160, QImage::Format_RGB32);
    image.fill(0xffffffff);
    image.setPixel(0, 0, 0xff000000);
    image.setPixel(image.width() - 1, image.height() - 1, 0xff336699);

    if (!image.save(args.at(1), "PNG")) {
        std::cerr << "failed to write " << args.at(1).toStdString() << '\n';
        return 1;
    }

    return 0;
}
