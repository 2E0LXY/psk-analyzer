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
        QColor fillColor = m_fraction < 0.75 ? QColor(90, 200, 255) : QColor(140, 230, 150);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fillColor);
        painter.drawRoundedRect(fillRect, 2, 2);
    }
}
