#include "MainWindow.h"
#include "AppConfig.h"
#include "cat/CatController.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMetaType>
#include <QPixmap>
#include <QScreen>
#include <QSplashScreen>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<DecodeLine>("DecodeLine");
    qRegisterMetaType<CatSnapshot>("CatSnapshot");
    qRegisterMetaType<AppConfig>("AppConfig");

    // Optional splash screen shown while the main window constructs. Skipped
    // silently (not an error) if assets/splash.png hasn't been added yet -
    // see assets/README.md for the expected image. The assets directory is
    // copied next to the executable at build time (see CMakeLists.txt) so
    // this path resolves the same way whether running from the build tree
    // or an installed copy.
    QSplashScreen *splash = nullptr;
    const QString splashPath = QDir(QCoreApplication::applicationDirPath()).filePath("assets/splash.png");
    if (QFile::exists(splashPath)) {
        QPixmap splashPixmap(splashPath);
        if (!splashPixmap.isNull()) {
            // Scale relative to the actual screen size rather than a fixed
            // pixel width, so it looks appropriately large on a real
            // display without hardcoding another number that's either too
            // big for a small screen or too small for a large one. Capped
            // at the source image's own resolution - never upscale past
            // native and introduce blur.
            int targetWidth = splashPixmap.width();
            if (const QScreen *screen = QGuiApplication::primaryScreen()) {
                targetWidth = qMin(splashPixmap.width(), static_cast<int>(screen->availableGeometry().width() * 0.6));
            }
            if (splashPixmap.width() > targetWidth) {
                splashPixmap = splashPixmap.scaledToWidth(targetWidth, Qt::SmoothTransformation);
            }
            splash = new QSplashScreen(splashPixmap);
            splash->show();
            app.processEvents();
        }
    }

    MainWindow window;
    window.show();

    if (splash) {
        splash->finish(&window);
        delete splash;
    }

    return app.exec();
}
