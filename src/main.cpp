#include "MainWindow.h"

#include <QApplication>
#include <QMetaType>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<DecodeLine>("DecodeLine");
    MainWindow window;
    window.show();
    return app.exec();
}
