#pragma once

#include <QLabel>
#include <QWidget>

// A compact horizontal level meter: a label, a bar, and a numeric
// readout. Used for real-time signal/audio metrics (SNR, decode quality,
// RX/TX audio level) - every value shown here comes from an actual
// measurement upstream (AudioEngine/Bpsk31Codec), never a placeholder.
// When there is nothing to show (e.g. TX level while not transmitting,
// or no CAT/signal lock yet), call setUnavailable() rather than leaving
// a stale or fabricated value on screen.
class LevelMeter : public QWidget {
    Q_OBJECT

public:
    explicit LevelMeter(const QString &label, QWidget *parent = nullptr);

    // value is clamped to [minValue, maxValue] and the bar fill is drawn
    // proportionally; valueText is what's shown numerically (caller
    // formats units/precision, e.g. "12.4 dB" or "86%").
    void setValue(double value, double minValue, double maxValue, const QString &valueText);
    void setUnavailable();

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize minimumSizeHint() const override;

private:
    QString m_label;
    QString m_valueText;
    double m_fraction = 0.0;
    bool m_available = false;
};
