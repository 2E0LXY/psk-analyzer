#pragma once

#include <QColor>
#include <QImage>
#include <QTimer>
#include <QWidget>

class WaterfallWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaterfallWidget(QWidget *parent = nullptr);

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
    QImage m_image;
    QTimer m_timer;
    int m_tick = 0;
};
