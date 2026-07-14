#include "WaterfallWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

WaterfallWidget::WaterfallWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(520, 320);
    m_timer.setInterval(45);
    connect(&m_timer, &QTimer::timeout, this, &WaterfallWidget::advanceFrame);
    m_timer.start();
}

void WaterfallWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(4, 8, 18));
    painter.drawImage(rect(), m_image);

    painter.setPen(QColor(90, 120, 145, 160));
    for (int i = 0; i <= 8; ++i) {
        const int x = i * width() / 8;
        painter.drawLine(x, 0, x, height());
        painter.drawText(x + 4, 16, QString::number(1000 + i * 250) + " Hz");
    }

    painter.setPen(QColor(0, 220, 255));
    for (int lane = 1; lane <= 16; ++lane) {
        const int x = lane * width() / 17;
        painter.drawLine(x, 24, x, height());
    }
}

void WaterfallWidget::resizeEvent(QResizeEvent *)
{
    if (width() <= 0 || height() <= 0) {
        return;
    }
    QImage newImage(size(), QImage::Format_RGB32);
    newImage.fill(QColor(4, 8, 18));
    QPainter painter(&newImage);
    painter.drawImage(0, 0, m_image.scaled(size()));
    m_image = newImage;
}

void WaterfallWidget::mousePressEvent(QMouseEvent *event)
{
    const double audioHz = 300.0 + (event->position().x() / qMax(1, width())) * 2700.0;
    emit frequencyClicked(audioHz);
}

void WaterfallWidget::advanceFrame()
{
    if (m_image.size() != size()) {
        m_image = QImage(size(), QImage::Format_RGB32);
        m_image.fill(QColor(4, 8, 18));
    }

    QPainter painter(&m_image);
    painter.drawImage(QPoint(-2, 0), m_image);

    for (int y = 0; y < height(); ++y) {
        const double yf = static_cast<double>(y) / qMax(1, height());
        double level = 0.08 + 0.09 * qSin((m_tick + y) * 0.03);

        const double traces[] = {0.18, 0.32, 0.46, 0.63, 0.78};
        for (double trace : traces) {
            const double distance = qAbs(yf - trace);
            level += qExp(-distance * distance * 900.0) * (0.55 + 0.35 * qSin(m_tick * 0.08 + trace * 20.0));
        }

        if ((m_tick / 12 + y) % 97 < 3) {
            level += 0.75;
        }

        painter.setPen(colorForLevel(qBound(0.0, level, 1.0)));
        painter.drawPoint(width() - 2, y);
        painter.drawPoint(width() - 1, y);
    }

    ++m_tick;
    update();
}

QColor WaterfallWidget::colorForLevel(double value) const
{
    if (value < 0.20) return QColor(5, 10, 45 + int(value * 120));
    if (value < 0.45) return QColor(0, int(90 + value * 220), int(160 + value * 100));
    if (value < 0.70) return QColor(int(value * 180), 220, 40);
    if (value < 0.90) return QColor(255, int(190 - value * 70), 20);
    return QColor(255, 245, 210);
}
