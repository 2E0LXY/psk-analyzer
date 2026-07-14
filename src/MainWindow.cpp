#include "MainWindow.h"

#include "MacroEngine.h"
#include "SettingsDialog.h"
#include "WaterfallWidget.h"

#include <QAction>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QList>
#include <QMenuBar>
#include <QPair>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QToolBar>
#include <QVariant>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("PSK Analyzer Pro - v1.0.0 beta");
    resize(1360, 820);

    auto *settingsAction = new QAction("Setup", this);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    menuBar()->addMenu("File")->addAction(settingsAction);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->addWidget(buildTopBar());

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    m_waterfall = new WaterfallWidget(this);
    connect(m_waterfall, &WaterfallWidget::frequencyClicked, this, &MainWindow::handleWaterfallClick);
    splitter->addWidget(m_waterfall);

    m_decodeTable = new QTableWidget(0, 7, this);
    m_decodeTable->setHorizontalHeaderLabels({"Ch", "Call", "Audio", "Mode", "SNR", "Quality", "Decoded text"});
    m_decodeTable->horizontalHeader()->setStretchLastSection(true);
    m_decodeTable->verticalHeader()->setVisible(false);
    m_decodeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_decodeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(m_decodeTable, &QTableWidget::cellClicked, this, &MainWindow::handleDecodeClick);
    splitter->addWidget(m_decodeTable);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    mainLayout->addWidget(buildBottomBar());
    setCentralWidget(central);

    setStyleSheet(R"(
        QMainWindow, QWidget { background: #0b1017; color: #d8f7ff; }
        QLabel#vfo { color: #6ee9ff; font-size: 42px; font-weight: 600; }
        QTableWidget { background: #070b10; gridline-color: #1e3d49; color: #74e6f6; selection-background-color: #16495c; }
        QHeaderView::section { background: #16212b; color: #d8f7ff; padding: 4px; border: 1px solid #263642; }
        QPlainTextEdit { background: #05080c; color: #d8f7ff; border: 1px solid #2e5360; font-family: Consolas, monospace; }
        QPushButton { background: #1b2a35; color: #d8f7ff; border: 1px solid #355465; padding: 6px 10px; }
        QPushButton:hover { background: #244052; }
    )");

    connect(&m_decoder, &MockDecoder::decoded, this, &MainWindow::addDecodedLine);
    m_decoder.start();
    updateStatusLabels();
}

QWidget *MainWindow::buildTopBar()
{
    auto *bar = new QWidget(this);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(4, 0, 4, 0);

    auto *setup = new QPushButton("Setup", this);
    connect(setup, &QPushButton::clicked, this, &MainWindow::openSettings);

    m_catLabel = new QLabel("CAT Offline", this);
    m_vfoLabel = new QLabel("14.070.000 MHz", this);
    m_vfoLabel->setObjectName("vfo");
    auto *mode = new QLabel("Mode: BPSK31   BW: 60 Hz   RX", this);

    layout->addWidget(setup);
    layout->addWidget(m_catLabel);
    layout->addStretch();
    layout->addWidget(m_vfoLabel);
    layout->addStretch();
    layout->addWidget(mode);
    return bar;
}

QWidget *MainWindow::buildBottomBar()
{
    auto *bar = new QWidget(this);
    auto *layout = new QVBoxLayout(bar);

    auto *macroRow = new QHBoxLayout();
    const QList<QPair<QString, QString>> macros = {
        {"CQ", "CQ CQ CQ DE {MYCALL} {MYCALL} K"},
        {"Answer", "{THEIRCALL} DE {MYCALL} "},
        {"Report", "{THEIRCALL} DE {MYCALL} UR {RST} SNR {SNR} AUDIO {AUDIO_FREQ}HZ IMD {IMD}"},
        {"Name/QTH", "NAME {NAME} QTH {QTH} LOC {LOCATOR}"},
        {"Rig", "RIG {RADIO_MODEL} PWR {PSK_POWER}W ANT {ANTENNA_NAME}"},
        {"73", "{THEIRCALL} DE {MYCALL} 73 SK"}
    };

    for (const auto &macro : macros) {
        auto *button = new QPushButton(macro.first, this);
        connect(button, &QPushButton::clicked, this, [this, macro]() { insertMacro(macro.second); });
        macroRow->addWidget(button);
    }

    auto *send = new QPushButton("Send", this);
    auto *abort = new QPushButton("Abort", this);
    auto *clear = new QPushButton("Clear", this);
    connect(clear, &QPushButton::clicked, this, [this]() { m_txText->clear(); });
    macroRow->addStretch();
    macroRow->addWidget(send);
    macroRow->addWidget(abort);
    macroRow->addWidget(clear);

    m_targetLabel = new QLabel("Reply target: none", this);
    m_txText = new QPlainTextEdit(this);
    m_txText->setPlaceholderText("Click a decoded line to insert THEIRCALL DE MYCALL, then type the reply here.");
    m_txText->setMaximumHeight(120);

    layout->addWidget(m_targetLabel);
    layout->addLayout(macroRow);
    layout->addWidget(m_txText);
    return bar;
}

void MainWindow::addDecodedLine(const DecodeLine &line)
{
    const int row = m_decodeTable->rowCount();
    m_decodeTable->insertRow(row);
    m_decodeTable->setItem(row, 0, new QTableWidgetItem(QString::number(line.channel)));
    m_decodeTable->setItem(row, 1, new QTableWidgetItem(line.callsign));
    m_decodeTable->setItem(row, 2, new QTableWidgetItem(QString::number(line.metrics.audioFrequencyHz, 'f', 0)));
    m_decodeTable->setItem(row, 3, new QTableWidgetItem(line.mode));
    m_decodeTable->setItem(row, 4, new QTableWidgetItem(QString::number(line.metrics.snrDb, 'f', 1)));
    m_decodeTable->setItem(row, 5, new QTableWidgetItem(QString::number(line.metrics.qualityPercent) + "%"));
    m_decodeTable->setItem(row, 6, new QTableWidgetItem(line.text));

    for (int col = 0; col < m_decodeTable->columnCount(); ++col) {
        m_decodeTable->item(row, col)->setData(Qt::UserRole, QVariant::fromValue(row));
    }

    if (m_decodeTable->rowCount() > 80) {
        m_decodeTable->removeRow(0);
    }
    m_decodeTable->scrollToBottom();
}

void MainWindow::handleDecodeClick(int row, int)
{
    DecodeLine line;
    line.channel = m_decodeTable->item(row, 0)->text().toInt();
    line.callsign = m_decodeTable->item(row, 1)->text();
    line.metrics.audioFrequencyHz = m_decodeTable->item(row, 2)->text().toDouble();
    line.mode = m_decodeTable->item(row, 3)->text();
    line.metrics.snrDb = m_decodeTable->item(row, 4)->text().toDouble();
    line.metrics.qualityPercent = m_decodeTable->item(row, 5)->text().remove('%').toInt();
    line.text = m_decodeTable->item(row, 6)->text();
    line.callsign = extractCallsign(line.text, line.callsign);
    prepareReply(line);
}

void MainWindow::handleWaterfallClick(double audioHz)
{
    statusBar()->showMessage(QString("Selected audio frequency %1 Hz").arg(audioHz, 0, 'f', 0), 3000);
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(m_config, this);
    if (dialog.exec() == QDialog::Accepted) {
        m_config = dialog.config();
        updateStatusLabels();
    }
}

void MainWindow::insertMacro(const QString &macroText)
{
    const QString expanded = MacroEngine::expand(macroText, m_config, m_selectedLine);
    m_txText->insertPlainText(expanded);
}

QString MainWindow::extractCallsign(const QString &text, const QString &fallback) const
{
    QRegularExpression dePattern("\\bDE\\s+([A-Z0-9]{1,3}[0-9][A-Z0-9]{1,4})\\b", QRegularExpression::CaseInsensitiveOption);
    auto match = dePattern.match(text);
    if (match.hasMatch()) {
        return match.captured(1).toUpper();
    }

    QRegularExpression callPattern("\\b([A-Z0-9]{1,3}[0-9][A-Z0-9]{1,4})\\b", QRegularExpression::CaseInsensitiveOption);
    match = callPattern.match(text);
    if (match.hasMatch()) {
        return match.captured(1).toUpper();
    }
    return fallback.toUpper();
}

void MainWindow::prepareReply(const DecodeLine &line)
{
    m_selectedLine = line;
    m_targetLabel->setText(QString("Reply target: %1   Mode: %2   SNR: %3 dB   Audio: %4 Hz")
                               .arg(line.callsign, line.mode)
                               .arg(line.metrics.snrDb, 0, 'f', 1)
                               .arg(line.metrics.audioFrequencyHz, 0, 'f', 0));
    m_txText->setPlainText(QString("%1 DE %2 ").arg(line.callsign, m_config.station.callsign));
    m_txText->setFocus();
    QTextCursor cursor = m_txText->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_txText->setTextCursor(cursor);
}

void MainWindow::updateStatusLabels()
{
    m_catLabel->setText(QString("CAT: %1").arg(m_config.cat.backend));
}
