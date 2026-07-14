#include "SettingsDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QTabWidget>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const AppConfig &config, QWidget *parent)
    : QDialog(parent), m_config(config)
{
    setWindowTitle("Setup");
    resize(640, 460);

    auto *tabs = new QTabWidget(this);

    auto *stationPage = new QWidget(this);
    auto *stationForm = new QFormLayout(stationPage);
    m_callsign = new QLineEdit(config.station.callsign);
    m_name = new QLineEdit(config.station.name);
    m_qth = new QLineEdit(config.station.qth);
    m_locator = new QLineEdit(config.station.locator);
    stationForm->addRow("Callsign", m_callsign);
    stationForm->addRow("Name", m_name);
    stationForm->addRow("QTH", m_qth);
    stationForm->addRow("Locator", m_locator);
    tabs->addTab(stationPage, "Station");

    auto *catPage = new QWidget(this);
    auto *catForm = new QFormLayout(catPage);
    m_catBackend = new QLineEdit(config.cat.backend);
    m_radioModel = new QLineEdit(config.cat.radioModel);
    m_catPort = new QLineEdit(config.cat.port);
    m_baud = new QSpinBox();
    m_baud->setRange(1200, 921600);
    m_baud->setValue(config.cat.baudRate);
    catForm->addRow("CAT backend", m_catBackend);
    catForm->addRow("Radio model", m_radioModel);
    catForm->addRow("Serial/USB port", m_catPort);
    catForm->addRow("Baud", m_baud);
    tabs->addTab(catPage, "CAT/PTT");

    auto *radioPage = new QWidget(this);
    auto *radioForm = new QFormLayout(radioPage);
    m_rigMake = new QLineEdit(config.equipment.radioMake);
    m_rigModel = new QLineEdit(config.equipment.radioModel);
    m_pskPower = new QSpinBox();
    m_pskPower->setRange(1, 1500);
    m_pskPower->setValue(config.equipment.pskPower);
    radioForm->addRow("Radio make", m_rigMake);
    radioForm->addRow("Radio model", m_rigModel);
    radioForm->addRow("Normal PSK power", m_pskPower);
    tabs->addTab(radioPage, "Radio");

    auto *antennaPage = new QWidget(this);
    auto *antennaForm = new QFormLayout(antennaPage);
    m_antennaName = new QLineEdit(config.antenna.name);
    m_antennaType = new QLineEdit(config.antenna.type);
    antennaForm->addRow("Antenna name", m_antennaName);
    antennaForm->addRow("Antenna type", m_antennaType);
    tabs->addTab(antennaPage, "Antenna");

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);
}

AppConfig SettingsDialog::config() const
{
    AppConfig updated = m_config;
    updated.station.callsign = m_callsign->text().trimmed().toUpper();
    updated.station.name = m_name->text().trimmed();
    updated.station.qth = m_qth->text().trimmed();
    updated.station.locator = m_locator->text().trimmed().toUpper();
    updated.cat.backend = m_catBackend->text().trimmed();
    updated.cat.radioModel = m_radioModel->text().trimmed();
    updated.cat.port = m_catPort->text().trimmed();
    updated.cat.baudRate = m_baud->value();
    updated.equipment.radioMake = m_rigMake->text().trimmed();
    updated.equipment.radioModel = m_rigModel->text().trimmed();
    updated.equipment.pskPower = m_pskPower->value();
    updated.antenna.name = m_antennaName->text().trimmed();
    updated.antenna.type = m_antennaType->text().trimmed();
    return updated;
}
