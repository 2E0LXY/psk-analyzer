#include "MainWindow.h"
#include "AppConfig.h"
#include "cat/CatController.h"

#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QMetaType>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QSplashScreen>
#include <QTimer>

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
            const int originalWidth = splashPixmap.width();

            // Scale relative to the actual screen size rather than a fixed
            // pixel width, so it looks appropriately large on a real
            // display without hardcoding another number that's either too
            // big for a small screen or too small for a large one. Capped
            // at the source image's own resolution - never upscale past
            // native and introduce blur.
            int targetWidth = originalWidth;
            if (const QScreen *screen = QGuiApplication::primaryScreen()) {
                targetWidth = qMin(originalWidth, static_cast<int>(screen->availableGeometry().width() * 0.6));
            }
            if (originalWidth > targetWidth) {
                splashPixmap = splashPixmap.scaledToWidth(targetWidth, Qt::SmoothTransformation);
            }

            splash = new QSplashScreen(splashPixmap);
            splash->show();
            app.processEvents();

            // The splash graphic has a decorative segmented bar built into
            // it (a progress/level-meter-style graphic, blue-to-green,
            // with a bright marker). Its position in the source image was
            // found by scanning for a thin band with high edge density
            // (many small colour transitions = many segments) surrounded
            // by clean background rows - not eyeballed, since this tool
            // chain can't reliably verify pixel content visually. Overlay
            // a real animated fill on top of it, scaled to match however
            // the splash image itself got scaled above, rather than
            // leaving it as inert decoration.
            const QRectF originalBarRect(393, 579, 704, 38);
            const double scale = static_cast<double>(splashPixmap.width()) / originalWidth;
            const QRectF barRect(originalBarRect.x() * scale, originalBarRect.y() * scale,
                                  originalBarRect.width() * scale, originalBarRect.height() * scale);

            const QPixmap basePixmap = splashPixmap;
            const int durationMs = 1400;
            QElapsedTimer elapsed;
            elapsed.start();
            QTimer frameTimer;
            frameTimer.setInterval(16); // ~60fps
            QEventLoop loop;
            QObject::connect(&frameTimer, &QTimer::timeout, [&]() {
                const double progress = qMin(1.0, elapsed.elapsed() / static_cast<double>(durationMs));
                QPixmap frame = basePixmap;
                QPainter painter(&frame);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(150, 230, 255, 150));
                const QRectF fillRect(barRect.x(), barRect.y(), barRect.width() * progress, barRect.height());
                painter.drawRoundedRect(fillRect, barRect.height() / 2.0, barRect.height() / 2.0);
                painter.end();
                splash->setPixmap(frame);
                if (progress >= 1.0) {
                    loop.quit();
                }
            });
            frameTimer.start();
            loop.exec();
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
