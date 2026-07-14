#include "GtpConsoleWidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QStringList>
#include <QVBoxLayout>

#include <algorithm>

namespace lizzie::ui {

namespace {

void appendPrefixedLines(QPlainTextEdit* output, const QString& prefix, const QString& sourcePrefix, QString text)
{
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    const QStringList lines = text.split('\n');
    for (const QString& line : lines) {
        output->appendPlainText(prefix + sourcePrefix + line);
    }
}

}  // namespace

GtpConsoleWidget::GtpConsoleWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    output_ = new QPlainTextEdit(this);
    output_->setReadOnly(true);
    output_->setMaximumBlockCount(5000);
    input_ = new QLineEdit(this);
    input_->installEventFilter(this);
    clearButton_ = new QPushButton(this);
    auto* inputRow = new QWidget(this);
    auto* inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->addWidget(input_, 1);
    inputLayout->addWidget(clearButton_);
    layout->addWidget(output_);
    layout->addWidget(inputRow);

    connect(input_, &QLineEdit::returnPressed, this, [this]() {
        const QString command = input_->text().trimmed();
        if (command.isEmpty()) {
            return;
        }
        if (commandHistory_.isEmpty() || commandHistory_.back() != command) {
            commandHistory_.push_back(command);
            constexpr qsizetype kMaxHistory = 200;
            while (commandHistory_.size() > kMaxHistory) {
                commandHistory_.removeFirst();
            }
        }
        historyIndex_ = commandHistory_.size();
        input_->clear();
        emit commandSubmitted(command);
    });
    connect(clearButton_, &QPushButton::clicked, output_, &QPlainTextEdit::clear);
    retranslate();
}

void GtpConsoleWidget::setChineseText(bool enabled)
{
    if (chineseText_ == enabled) {
        return;
    }
    chineseText_ = enabled;
    retranslate();
}

bool GtpConsoleWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == input_ && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up && !commandHistory_.isEmpty()) {
            historyIndex_ = std::max(0, historyIndex_ - 1);
            input_->setText(commandHistory_.at(historyIndex_));
            input_->setCursorPosition(input_->text().size());
            return true;
        }
        if (keyEvent->key() == Qt::Key_Down && !commandHistory_.isEmpty()) {
            if (historyIndex_ < commandHistory_.size() - 1) {
                ++historyIndex_;
                input_->setText(commandHistory_.at(historyIndex_));
            } else {
                historyIndex_ = commandHistory_.size();
                input_->clear();
            }
            input_->setCursorPosition(input_->text().size());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void GtpConsoleWidget::appendProtocolLine(const QString& line)
{
    output_->appendPlainText(line);
}

void GtpConsoleWidget::appendStderrLine(const QString& line, const QString& sourcePrefix)
{
    appendPrefixedLines(output_, stderrPrefix(), sourcePrefix, line);
}

void GtpConsoleWidget::appendSystemLine(const QString& line, const QString& sourcePrefix)
{
    appendPrefixedLines(output_, systemPrefix(), sourcePrefix, line);
}

void GtpConsoleWidget::retranslate()
{
    if (input_ != nullptr) {
        input_->setPlaceholderText(
            chineseText_ ? QString::fromUtf8("输入 GTP 命令后按回车") : QStringLiteral("Enter GTP command and press Return"));
    }
    if (clearButton_ != nullptr) {
        clearButton_->setText(chineseText_ ? QString::fromUtf8("清空") : QStringLiteral("Clear"));
        clearButton_->setToolTip(chineseText_ ? QString::fromUtf8("清空控制台输出") : QStringLiteral("Clear console output"));
    }
}

QString GtpConsoleWidget::systemPrefix() const
{
    return chineseText_ ? QString::fromUtf8("系统: ") : QStringLiteral("system: ");
}

QString GtpConsoleWidget::stderrPrefix() const
{
    return chineseText_ ? QString::fromUtf8("标准错误: ") : QStringLiteral("stderr: ");
}

}  // namespace lizzie::ui
