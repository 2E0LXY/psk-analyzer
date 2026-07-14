#pragma once

#include <QString>

struct StationDetails {
    QString callsign = "N0CALL";
    QString name = "Operator";
    QString qth = "QTH";
    QString locator = "IO00AA";
    QString country = "";
    QString defaultReport = "599";
};

struct CatSettings {
    QString backend = "Hamlib native";
    QString radioModel = "Manual";
    QString port = "";
    int baudRate = 9600;
    QString host = "127.0.0.1";
    int tcpPort = 4532;
    QString pttMethod = "CAT";
    int pollMs = 500;
    bool autoConnect = false;
};

struct EquipmentProfile {
    QString radioMake = "";
    QString radioModel = "";
    QString interfaceType = "";
    int maxPower = 100;
    int pskPower = 25;
    QString dataMode = "USB-D";
};

struct AntennaProfile {
    QString name = "Main antenna";
    QString type = "Dipole";
    QString bands = "20m";
    QString height = "";
    QString direction = "";
};

struct AppConfig {
    StationDetails station;
    CatSettings cat;
    EquipmentProfile equipment;
    AntennaProfile antenna;
};
