#pragma once

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStringList>
#include <QWidget>

namespace lizzie::ui {

class GtpConsoleWidget : public QWidget {
    Q_OBJECT

public:
    explicit GtpConsoleWidget(QWidget* parent = nullptr);
    void setChineseText(bool enabled);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

public slots:
    void appendProtocolLine(const QString& line);
    void appendStderrLine(const QString& line, const QString& sourcePrefix = {});
    void appendSystemLine(const QString& line, const QString& sourcePrefix = {});

signals:
    void commandSubmitted(const QString& command);

private:
    void retranslate();
    [[nodiscard]] QString systemPrefix() const;
    [[nodiscard]] QString stderrPrefix() const;

    QPlainTextEdit* output_ = nullptr;
    QLineEdit* input_ = nullptr;
    QPushButton* clearButton_ = nullptr;
    QStringList commandHistory_;
    int historyIndex_ = 0;
    bool chineseText_ = false;
};

}  // namespace lizzie::ui
