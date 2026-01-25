#pragma once

#include <QMainWindow>
#include "qcustomplot.h"

class QCustomPlot;
class QLabel;
class QTextEdit;
class QLineEdit;
class QSlider;
class QTimer;

class OFDMDemoWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OFDMDemoWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void updateFrame();

private:
    // ================== TX ==================
    QCustomPlot* txConstPlot  = nullptr;
    QCustomPlot* txTimePlot   = nullptr;
    QTextEdit*   txTextEdit   = nullptr;

    // ================== RX ==================
    QCustomPlot* rxConstPlot  = nullptr;
    QCustomPlot* rxTimePlot   = nullptr;
    QLabel*      rxTextLabel  = nullptr;

    // ================== Channel ==================
    QCustomPlot* noisePlot    = nullptr;
    QLineEdit*   noiseEdit    = nullptr;

    QSlider*     speedSlider  = nullptr;
    QLabel*      speedLabel   = nullptr;
    QTimer*      timer        = nullptr;
};
