#pragma once

#include <QColor>
#include <QImage>
#include <QTimer>
#include <QWidget>

class WaterfallWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaterfallWidget(QWidget *parent = nullptr);
    void setRxAudioHz(double audioHz);
    void setTxAudioHz(double audioHz);
    void setTxLockedToRx(bool locked);

signals:
    void frequencyClicked(double audioHz);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void advanceFrame();

private:
    QColor colorForLevel(double value) const;
    int xForAudio(double audioHz) const;
    double audioForX(double x) const;
    QImage m_image;
    QTimer m_timer;
    double m_rxAudioHz = 1420.0;
    double m_txAudioHz = 1420.0;
    bool m_txLockedToRx = true;
    int m_tick = 0;
};
