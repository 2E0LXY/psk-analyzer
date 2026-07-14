#pragma once

#include "AppConfig.h"
#include "MockDecoder.h"
#include "SignalTypes.h"

#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTableWidget>

class WaterfallWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addDecodedLine(const DecodeLine &line);
    void handleDecodeClick(int row, int column);
    void handleWaterfallClick(double audioHz);
    void openSettings();
    void insertMacro(const QString &macroText);

private:
    QString extractCallsign(const QString &text, const QString &fallback) const;
    void prepareReply(const DecodeLine &line);
    void updateStatusLabels();
    QWidget *buildTopBar();
    QWidget *buildBottomBar();

    AppConfig m_config;
    DecodeLine m_selectedLine;
    WaterfallWidget *m_waterfall = nullptr;
    QTableWidget *m_decodeTable = nullptr;
    QPlainTextEdit *m_txText = nullptr;
    QLabel *m_vfoLabel = nullptr;
    QLabel *m_targetLabel = nullptr;
    QLabel *m_catLabel = nullptr;
    MockDecoder m_decoder;
};
