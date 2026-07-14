#pragma once

#include "AppConfig.h"

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(const AppConfig &config, QWidget *parent = nullptr);
    AppConfig config() const;

private:
    AppConfig m_config;
    QLineEdit *m_callsign = nullptr;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_qth = nullptr;
    QLineEdit *m_locator = nullptr;
    QLineEdit *m_catBackend = nullptr;
    QLineEdit *m_radioModel = nullptr;
    QLineEdit *m_catPort = nullptr;
    QSpinBox *m_baud = nullptr;
    QLineEdit *m_rigMake = nullptr;
    QLineEdit *m_rigModel = nullptr;
    QSpinBox *m_pskPower = nullptr;
    QLineEdit *m_antennaName = nullptr;
    QLineEdit *m_antennaType = nullptr;
};
