#pragma once

#include "AppConfig.h"
#include "SignalTypes.h"

#include <QMap>
#include <QString>

class MacroEngine {
public:
    static QString expand(QString text,
                          const AppConfig &config,
                          const DecodeLine &selectedLine,
                          const QMap<QString, QString> &extra = {});
};
