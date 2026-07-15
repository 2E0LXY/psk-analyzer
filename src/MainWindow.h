#pragma once

#include "AppConfig.h"
#include "DecoderTableModel.h"
#include "MockDecoder.h"
#include "SignalTypes.h"

#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableView>

class WaterfallWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addDecodedLine(const DecodeLine &line);
    void handleActiveDecodeClick(const QModelIndex &index);
    void handleSweeperClick(const QModelIndex &index);
    void handleWaterfallClick(double audioHz);
    void openSettings();
    void insertMacro(const QString &macroText);
    void updateTxSafety();

private:
    QString extractCallsign(const QString &text, const QString &fallback) const;
    void prepareReply(const DecodeLine &line);
    void loadSettings();
    void saveSettings() const;
    void updateStatusLabels();
    QWidget *buildTopBar();
    QWidget *buildRightPanel();
    QWidget *buildWorkflowPanel();
    QWidget *buildSelectedQsoPanel();
    QWidget *buildTxPanel();
    void configureTable(QTableView *view);

    AppConfig m_config;
    DecodeLine m_selectedLine;
    DecoderTableModel *m_activeModel = nullptr;
    DecoderTableModel *m_sweeperModel = nullptr;
    WaterfallWidget *m_waterfall = nullptr;
    QTableView *m_activeView = nullptr;
    QTableView *m_sweeperView = nullptr;
    QPlainTextEdit *m_txText = nullptr;
    QLabel *m_vfoLabel = nullptr;
    QLabel *m_targetLabel = nullptr;
    QLabel *m_catLabel = nullptr;
    QLabel *m_qsoCallLabel = nullptr;
    QLabel *m_qsoSignalLabel = nullptr;
    QLabel *m_qsoFreqLabel = nullptr;
    QLabel *m_txSafetyLabel = nullptr;
    QPushButton *m_sendButton = nullptr;
    MockDecoder m_decoder;
};
