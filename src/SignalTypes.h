#pragma once

#include <QString>

struct SignalMetrics {
    double snrDb = 12.0;
    double signalLevelDb = -64.0;
    double noiseFloorDb = -76.0;
    double audioFrequencyHz = 1420.0;
    double rfFrequencyMhz = 14.071420;
    double bandwidthHz = 60.0;
    double driftHzPerMinute = 1.5;
    int qualityPercent = 92;
    QString lockQuality = "Locked";
    QString imdDb = "-24";
};

struct DecodeLine {
    int channel = 1;
    QString callsign;
    QString mode = "BPSK31";
    QString text;
    SignalMetrics metrics;
};
