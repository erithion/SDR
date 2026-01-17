#include "QFDemoWindow.hpp"
#include "ofdm.hpp"
#include "modulation.hpp"
#include "sliding_buffer.hpp"

#include <complex>
#include <mutex>
#include <string>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QGroupBox>
#include <QEvent>
#include <QSlider>
#include <QTimer>

std::mutex                                  mtx;
utils::sliding_buffer<std::complex<float>>  slidingPlot(512);
utils::sliding_buffer<uint8_t>              slidingText(50);
size_t                                      payloadPos = 0;
const std::string                           payload =
    "Hello, world! "
    "I am a Software-Defined Radio Stack.          "
    "This string is a result of demultiplexing a 16-QAM "
    "multiplexed OFDM signal.          ";

OFDMDemoWindow::OFDMDemoWindow()
{
    setWindowTitle("Software-Defined Radio Stack Demo");

    auto* central = new QWidget;
    auto* layout  = new QVBoxLayout;
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);
    central->setLayout(layout);
    setCentralWidget(central);

    QFont captionFont;
    captionFont.setPointSize(14);

    // OFDM time signal
    auto* timeGroup = new QGroupBox("OFDM Signal");
    timeGroup->setFont(captionFont);
    auto* timeLayout = new QVBoxLayout(timeGroup);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(0);

    timePlot = new QCustomPlot;
    timePlot->setMouseTracking(true);
    timePlot->setAttribute(Qt::WA_Hover, true);
    timePlot->installEventFilter(this);

    timePlot->legend->setVisible(true);
    timePlot->setAutoAddPlottableToLegend(true);

    timePlot->addGraph(); // Re
    timePlot->addGraph(); // Im

    timePlot->graph(0)->setName("Re{x[n]}");
    timePlot->graph(1)->setName("Im{x[n]}");

    timePlot->graph(0)->setPen(QPen(Qt::blue));
    timePlot->graph(1)->setPen(QPen(Qt::red));

    timePlot->xAxis->setLabel("n");
    timePlot->yAxis->setLabel("Amplitude");
    timePlot->yAxis->setRange(-4, 4);

    timeLayout->addWidget(timePlot);
    layout->addWidget(timeGroup);

    // Constellation
    auto* constGroup = new QGroupBox("Constellation");
    constGroup->setFont(captionFont);
    auto* constLayout = new QVBoxLayout(constGroup);
    constLayout->setContentsMargins(0, 0, 0, 0);
    constLayout->setSpacing(0);

    constPlot = new QCustomPlot;
    constPlot->setMouseTracking(true);
    constPlot->setAttribute(Qt::WA_Hover, true);
    constPlot->installEventFilter(this);

    constPlot->addGraph();
    constPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
    constPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 6));

    constPlot->xAxis->setLabel("Re");
    constPlot->yAxis->setLabel("Im");
    constPlot->xAxis->setRange(-4.3, 4.3);
    constPlot->yAxis->setRange(-4.3, 4.3);

    constLayout->addWidget(constPlot);
    layout->addWidget(constGroup);

    // Decoded text
    auto* textGroup = new QGroupBox("Decoded Data");
    textGroup->setFont(captionFont);
    auto* textLayout = new QVBoxLayout(textGroup);

    textLabel = new QLabel;
    textLabel->setWordWrap(true);

    QFont textFont = textLabel->font();
    textFont.setPointSize(24);
    textLabel->setFont(textFont);

    textLayout->addWidget(textLabel);
    layout->addWidget(textGroup);

    // Speed control
    auto* speedGroup  = new QGroupBox("(De)multiplexing speed");
    speedGroup->setFont(captionFont);
    auto* speedLayout = new QHBoxLayout(speedGroup);

    auto* speedLabel  = new QLabel("Update interval: 50 ms");
    auto* speedSlider = new QSlider(Qt::Horizontal);

    speedSlider->setRange(1, 200);
    speedSlider->setValue(50);
    speedSlider->setTickInterval(10);
    speedSlider->setTickPosition(QSlider::TicksBelow);

    // Slider hover styling (THIS WORKS)
    speedSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            height: 6px;
            background: #d0d0d0;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            width: 18px;
            margin: -6px 0;
            background: #5a8dee;
            border-radius: 9px;
        }
        QSlider::handle:horizontal:hover {
            background: #2f6fed;
        }
        QSlider::groove:horizontal:hover {
            background: #b0c8ff;
        }
    )");

    speedLayout->addWidget(speedLabel);
    speedLayout->addWidget(speedSlider);
    layout->addWidget(speedGroup);

    // Timer
    auto* timer = new QTimer(this);
    timer->setInterval(50);

    connect(timer, &QTimer::timeout, this, &OFDMDemoWindow::updateFrame);
    connect(speedSlider, &QSlider::valueChanged, this, [timer, speedLabel](int value)
    {
        timer->setInterval(value);
        speedLabel->setText(QString("Update interval: %1 ms").arg(value));
    });

    timer->start();
}

bool OFDMDemoWindow::eventFilter(QObject* obj, QEvent* event)
{
    auto hoverOn = [](QCustomPlot* p)
    {
        p->setBackground(QColor(245, 248, 255));
        p->xAxis->setBasePen(QPen(Qt::darkBlue, 2));
        p->yAxis->setBasePen(QPen(Qt::darkBlue, 2));
        p->xAxis->setTickPen(QPen(Qt::darkBlue, 2));
        p->yAxis->setTickPen(QPen(Qt::darkBlue, 2));
        p->replot(QCustomPlot::rpQueuedReplot);
    };

    auto hoverOff = [](QCustomPlot* p)
    {
        p->setBackground(Qt::white);
        p->xAxis->setBasePen(QPen(Qt::black, 1));
        p->yAxis->setBasePen(QPen(Qt::black, 1));
        p->xAxis->setTickPen(QPen(Qt::black, 1));
        p->yAxis->setTickPen(QPen(Qt::black, 1));
        p->replot(QCustomPlot::rpQueuedReplot);
    };

    if (event->type() == QEvent::HoverEnter)
    {
        if (obj == timePlot)  hoverOn(timePlot);
        if (obj == constPlot) hoverOn(constPlot);
    }
    else if (event->type() == QEvent::HoverLeave)
    {
        if (obj == timePlot)  hoverOff(timePlot);
        if (obj == constPlot) hoverOff(constPlot);
    }

    return QMainWindow::eventFilter(obj, event);
}

void OFDMDemoWindow::updateFrame()
{
    std::vector<uint8_t> input;
    for (int i = 0; i < 4; ++i)
    {
        input.push_back(payload[payloadPos % payload.size()]);
        ++payloadPos;
    }

    auto const_syms = modulation::to_constl<modulation::e16QAM>(input); // bits encoding
    if (const_syms.empty()) return;

    auto tx_exp = ofdm::tx(const_syms, 8); // multiplexing
    if (!tx_exp) return;

    auto& tx = *tx_exp;

    ofdm::rx(tx, 8) // demultiplexing
        .transform([](auto&& rx)
        {
            auto bytes = modulation::from_constl<modulation::e16QAM>(rx); // bits decoding
            slidingText.push_back(bytes.begin(), bytes.end());
            return std::expected<void, std::string>{};
        });

    // time domain
    {
        std::lock_guard lock(mtx);
        slidingPlot.push_back(tx.begin(), tx.end());

        size_t N = slidingPlot.size();
        QVector<double> x(N), re(N), im(N);

        for (size_t i = 0; i < N; ++i)
            slidingPlot.at(i)
                .and_then([&](auto&& v) -> std::expected<std::complex<double>, std::string>
                {
                    x[i]  = i;
                    re[i] = v.real();
                    im[i] = v.imag();
                    return v;
                });

        timePlot->graph(0)->setData(x, re);
        timePlot->graph(1)->setData(x, im);
        timePlot->xAxis->setRange(N > 512 ? N - 512 : 0, N);
        timePlot->replot();
    }

    // constellation
    {
        QVector<double> x(const_syms.size()),
                        y(const_syms.size());
        for (size_t i = 0; i < const_syms.size(); ++i)
        {
            x[i] = const_syms[i].real();
            y[i] = const_syms[i].imag();
        }
        constPlot->graph(0)->setData(x, y);
        constPlot->replot();
    }

    // running text
    std::string s(slidingText.begin(), slidingText.end());
    textLabel->setText(QString::fromStdString(s));
}
