#pragma once

#include <QMetaType>
#include <QString>

struct StationDetails {
    QString callsign = "N0CALL";
    QString name = "Operator";
    QString qth = "QTH";
    QString locator = "IO00AA";
    QString country = "";
    QString defaultReport = "599";
};

struct CatRigSettings {
    bool enabled = false;
    QString backend = "Hamlib rigctld";
    int rigSlot = 1;
    QString radioModel = "Manual";
    QString port = "";
    int baudRate = 9600;
    QString host = "127.0.0.1";
    int tcpPort = 4532;
};

struct CatSettings {
    QString backend = "Hamlib rigctld";
    int activeRig = 1;
    CatRigSettings rig1 = [] {
        CatRigSettings rig;
        rig.enabled = false;
        rig.rigSlot = 1;
        rig.tcpPort = 4532;
        return rig;
    }();
    CatRigSettings rig2 = [] {
        CatRigSettings rig;
        rig.enabled = false;
        rig.rigSlot = 2;
        rig.tcpPort = 4533;
        return rig;
    }();
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

struct AudioSettings {
    QString rxInputDeviceId;
    QString txOutputDeviceId;
};

struct RemoteControlSettings {
    bool enabled = false;
    int port = 8765;
    // Plaintext token, no TLS on the server side - see
    // RemoteControlServer's header comment. Empty by default so the
    // server refuses ALL auth attempts (rather than accepting an empty
    // token) until the operator explicitly sets one.
    QString authToken;
};

struct AppConfig {
    StationDetails station;
    CatSettings cat;
    AudioSettings audio;
    EquipmentProfile equipment;
    AntennaProfile antenna;
    RemoteControlSettings remote;
};

Q_DECLARE_METATYPE(AppConfig)
