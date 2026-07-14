#include "MacroEngine.h"

#include <QDateTime>

QString MacroEngine::expand(QString text,
                            const AppConfig &config,
                            const DecodeLine &selectedLine,
                            const QMap<QString, QString> &extra)
{
    QMap<QString, QString> values = {
        {"MYCALL", config.station.callsign},
        {"THEIRCALL", selectedLine.callsign},
        {"NAME", config.station.name},
        {"QTH", config.station.qth},
        {"LOCATOR", config.station.locator},
        {"COUNTRY", config.station.country},
        {"RST", config.station.defaultReport},
        {"RADIO_MAKE", config.equipment.radioMake},
        {"RADIO_MODEL", config.equipment.radioModel},
        {"PSK_POWER", QString::number(config.equipment.pskPower)},
        {"ANTENNA_NAME", config.antenna.name},
        {"ANTENNA_TYPE", config.antenna.type},
        {"FREQUENCY", QString::number(selectedLine.metrics.rfFrequencyMhz, 'f', 6)},
        {"RF_FREQ", QString::number(selectedLine.metrics.rfFrequencyMhz, 'f', 6)},
        {"AUDIO_FREQ", QString::number(selectedLine.metrics.audioFrequencyHz, 'f', 0)},
        {"MODE", selectedLine.mode},
        {"SNR", QString("%1 dB").arg(selectedLine.metrics.snrDb, 0, 'f', 1)},
        {"SIGNAL_LEVEL", QString("%1 dB").arg(selectedLine.metrics.signalLevelDb, 0, 'f', 1)},
        {"NOISE_FLOOR", QString("%1 dB").arg(selectedLine.metrics.noiseFloorDb, 0, 'f', 1)},
        {"SIGNAL_BANDWIDTH", QString("%1 Hz").arg(selectedLine.metrics.bandwidthHz, 0, 'f', 0)},
        {"DRIFT", QString("%1 Hz/min").arg(selectedLine.metrics.driftHzPerMinute, 0, 'f', 1)},
        {"QUALITY", QString("%1%").arg(selectedLine.metrics.qualityPercent)},
        {"LOCK_QUALITY", selectedLine.metrics.lockQuality},
        {"IMD", selectedLine.metrics.imdDb + " dB"},
        {"TIME_UTC", QDateTime::currentDateTimeUtc().toString("HHmm")},
        {"DATE_UTC", QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd")}
    };

    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it) {
        values[it.key()] = it.value();
    }

    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        text.replace("{" + it.key() + "}", it.value());
    }
    return text;
}
