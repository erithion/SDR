#pragma once

#include <QMainWindow>
#include <QPen>
#include <QTimer>
#include <QLabel>
#include "qcustomplot.h"

class QCustomPlot;
class QLabel;

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
    QCustomPlot* timePlot = nullptr;
    QCustomPlot* constPlot = nullptr;
    QLabel*      textLabel = nullptr;
};
