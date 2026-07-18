#include "LevelMeter.h"

#include <QPainter>
#include <algorithm>

LevelMeter::LevelMeter(const QString &label, QWidget *parent)
    : QWidget(parent)
    , m_label(label)
{
    setUnavailable();
}

void LevelMeter::setValue(double value, double minValue, double maxValue, const QString &valueText)
{
    m_available = true;
    m_valueText = valueText;
    const double span = maxValue - minValue;
    m_fraction = span > 0.0 ? std::clamp((value - minValue) / span, 0.0, 1.0) : 0.0;
    update();
}

void LevelMeter::setUnavailable()
{
    m_available = false;
    m_valueText = "--";
    m_fraction = 0.0;
    update();
}

QSize LevelMeter::minimumSizeHint() const
{
    return QSize(120, 34);
}

void LevelMeter::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRect r = rect();
    QFont labelFont = painter.font();
    labelFont.setPointSize(8);
    painter.setFont(labelFont);
    painter.setPen(QColor(140, 190, 210));
    painter.drawText(r.adjusted(2, 0, -2, -r.height() + 14), Qt::AlignLeft | Qt::AlignVCenter, m_label);
    painter.drawText(r.adjusted(2, 0, -2, -r.height() + 14), Qt::AlignRight | Qt::AlignVCenter, m_valueText);

    const QRect barRect(2, 16, r.width() - 4, r.height() - 18);
    painter.setPen(QColor(50, 70, 85));
    painter.setBrush(QColor(10, 15, 20));
    painter.drawRoundedRect(barRect, 3, 3);

    if (m_available && m_fraction > 0.0) {
        QRect fillRect = barRect.adjusted(1, 1, -1, -1);
        fillRect.setWidth(static_cast<int>(fillRect.width() * m_fraction));
        painter.setPen(Qt::NoPen);
        painter.setBrush(colorForFraction(m_fraction));
        painter.drawRoundedRect(fillRect, 2, 2);
    }
}

QColor LevelMeter::colorForFraction(double fraction) const
{
    // Poor-to-good colour scale: red at 0, yellow at 0.5, green at 1.0.
    // Two linear segments (red->yellow, yellow->green) rather than a
    // single interpolation, so 0.5 reads as a clean, recognisable amber.
    if (fraction < 0.5) {
        const double t = fraction / 0.5;
        return QColor(220, static_cast<int>(60 + t * (200 - 60)), 60);
    }
    const double t = (fraction - 0.5) / 0.5;
    return QColor(static_cast<int>(220 - t * (220 - 90)), 200, static_cast<int>(60 + t * (150 - 60)));
}
