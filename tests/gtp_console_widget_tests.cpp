#include "GtpConsoleWidget.h"

#include <QApplication>
#include <QByteArray>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }

    QApplication app(argc, argv);

    try {
        lizzie::ui::GtpConsoleWidget console;
        console.appendProtocolLine("=1 KataGo Fake");
        console.appendStderrLine("CUDA backend ready");
        console.appendSystemLine("engine started");
        console.appendStderrLine("line one\r\nline two");
        console.appendSystemLine("startup failed\nCommand: katago gtp\nWorking directory: /tmp/lizzie");
        console.appendStderrLine("compare line one\r\ncompare line two", "[compare] ");
        console.appendSystemLine("compare failed\nCommand: compare katago", "[compare] ");

        QPlainTextEdit* output = console.findChild<QPlainTextEdit*>();
        require(output != nullptr, "console should contain output widget");
        const QString text = output->toPlainText();
        require(text.contains("=1 KataGo Fake"), "console should show protocol lines");
        require(text.contains("stderr: CUDA backend ready"), "console should prefix stderr lines");
        require(text.contains("system: engine started"), "console should prefix system lines");
        require(text.contains("stderr: line one\nstderr: line two"), "console should prefix each stderr line");
        require(
            text.contains("system: startup failed\nsystem: Command: katago gtp\nsystem: Working directory: /tmp/lizzie"),
            "console should prefix each system diagnostic line");
        require(
            text.contains("stderr: [compare] compare line one\nstderr: [compare] compare line two"),
            "console should preserve compare source prefix on each stderr line");
        require(
            text.contains("system: [compare] compare failed\nsystem: [compare] Command: compare katago"),
            "console should preserve compare source prefix on each system diagnostic line");

        QString submitted;
        QObject::connect(&console, &lizzie::ui::GtpConsoleWidget::commandSubmitted, [&](const QString& command) {
            submitted = command;
        });

        QLineEdit* input = console.findChild<QLineEdit*>();
        require(input != nullptr, "console should contain input widget");
        require(
            input->placeholderText() == "Enter GTP command and press Return",
            "console should describe raw GTP command input");
        input->setText("  kata-set-param maxVisits 128  ");
        emit input->returnPressed();
        require(submitted == "kata-set-param maxVisits 128", "console should submit trimmed non-empty commands");
        require(input->text().isEmpty(), "console should clear submitted input");

        input->setText("list_commands");
        emit input->returnPressed();
        require(submitted == "list_commands", "console should submit a second command");

        QKeyEvent up(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
        QApplication::sendEvent(input, &up);
        require(input->text() == "list_commands", "console history Up should recall latest command");
        QKeyEvent upAgain(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
        QApplication::sendEvent(input, &upAgain);
        require(
            input->text() == "kata-set-param maxVisits 128",
            "console history Up should move to older commands");
        QKeyEvent upAtStart(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
        QApplication::sendEvent(input, &upAtStart);
        require(
            input->text() == "kata-set-param maxVisits 128",
            "console history should stay at the oldest command");
        QKeyEvent down(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
        QApplication::sendEvent(input, &down);
        require(input->text() == "list_commands", "console history Down should move to newer commands");
        QKeyEvent downToEnd(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
        QApplication::sendEvent(input, &downToEnd);
        require(input->text().isEmpty(), "console history Down at the end should clear the input");

        submitted.clear();
        input->setText("   ");
        emit input->returnPressed();
        require(submitted.isEmpty(), "console should ignore empty commands");

        QPushButton* clearButton = console.findChild<QPushButton*>();
        require(clearButton != nullptr, "console should expose a clear button");
        require(clearButton->text() == "Clear", "console clear button should default to English");
        clearButton->click();
        require(output->toPlainText().isEmpty(), "console clear button should clear output");

        console.setChineseText(true);
        require(input->placeholderText() == QString::fromUtf8("输入 GTP 命令后按回车"), "console input should translate");
        require(clearButton->text() == QString::fromUtf8("清空"), "console clear button should translate");
        console.appendSystemLine(QString::fromUtf8("引擎已启动"));
        console.appendStderrLine(QString::fromUtf8("CUDA 后端已就绪\nOpenCL 后端已就绪"));
        console.appendStderrLine(QString::fromUtf8("对比 stderr"), QString::fromUtf8("对比："));
        require(output->toPlainText().contains(QString::fromUtf8("系统: 引擎已启动")), "console system prefix should translate");
        require(
            output->toPlainText().contains(QString::fromUtf8("标准错误: CUDA 后端已就绪\n标准错误: OpenCL 后端已就绪")),
            "console stderr prefix should translate on each line");
        require(
            output->toPlainText().contains(QString::fromUtf8("标准错误: 对比：对比 stderr")),
            "console stderr prefix should preserve translated compare source prefix");

        console.setChineseText(false);
        console.appendSystemLine("engine restarted");
        console.appendStderrLine("backend restarted");
        require(output->toPlainText().contains("system: engine restarted"), "console system prefix should return to English");
        require(output->toPlainText().contains("stderr: backend restarted"), "console stderr prefix should return to English");
    } catch (const std::exception& error) {
        std::cerr << "gtp_console_widget_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "gtp_console_widget_tests passed\n";
    return EXIT_SUCCESS;
}
