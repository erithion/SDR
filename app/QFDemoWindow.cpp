// QFDemoWindow.cpp
#include "QFDemoWindow.hpp"
#include "ofdm.hpp"
#include "sliding_buffer.hpp"

#include <deque>
#include <complex>
#include <mutex>
#include <string>
#include <vector>

#include <QVBoxLayout>
#include <QLabel>
#include <QFont>
#include <QGroupBox>
#include <QEvent>

std::mutex                                  mtx;
utils::sliding_buffer<std::complex<float>>  g_time_plot(512);
utils::sliding_buffer<uint8_t>              g_decoded_text(50);
size_t                                      payload_pos = 0;
const std::string                           payload =
    "Hello, world! I am a Software-Defined Radio Stack.          ";

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

    // OFDM signal
    auto* timeGroup = new QGroupBox("OFDM Signal");
    timeGroup->setFont(captionFont);
    auto* timeLayout = new QVBoxLayout(timeGroup);

    timePlot = new QCustomPlot;
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

    constPlot = new QCustomPlot;
    constPlot->installEventFilter(this);

    constPlot->addGraph();
    constPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
    constPlot->graph(0)->setScatterStyle(
        QCPScatterStyle(QCPScatterStyle::ssCircle, 6));

    constPlot->xAxis->setLabel("Re");
    constPlot->yAxis->setLabel("Im");
    constPlot->xAxis->setRange(-4.3, 4.3);
    constPlot->yAxis->setRange(-4.3, 4.3);

    constLayout->addWidget(constPlot);
    layout->addWidget(constGroup);

    // Decoded data
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

    // Timer
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &OFDMDemoWindow::updateFrame);
    timer->start(50);
}

bool OFDMDemoWindow::eventFilter(QObject* obj, QEvent* event)
{
    auto applyStyle = [](QCustomPlot* plot, bool hover)
    {
        if (hover)
        {
            plot->setBackground(QColor(245, 248, 255));
            plot->xAxis->setBasePen(QPen(Qt::darkBlue, 2));
            plot->yAxis->setBasePen(QPen(Qt::darkBlue, 2));
        }
        else
        {
            plot->setBackground(Qt::white);
            plot->xAxis->setBasePen(QPen(Qt::black, 1));
            plot->yAxis->setBasePen(QPen(Qt::black, 1));
        }
        plot->replot(QCustomPlot::rpQueuedReplot);
    };

    if (event->type() == QEvent::Enter)
    {
        if (obj == timePlot)  applyStyle(timePlot, true);
        if (obj == constPlot) applyStyle(constPlot, true);
    }
    else if (event->type() == QEvent::Leave)
    {
        if (obj == timePlot)  applyStyle(timePlot, false);
        if (obj == constPlot) applyStyle(constPlot, false);
    }

    return QMainWindow::eventFilter(obj, event);
}

void OFDMDemoWindow::updateFrame()
{
    std::vector<uint8_t> input;
    for (int i = 0; i < 4; ++i)
    {
        input.push_back(payload[payload_pos % payload.size()]);
        ++payload_pos;
    }

    auto const_syms = ofdm::to_constellations<ofdm::e16QAM>(input); // encoding bits
    if (const_syms.empty()) return;

    auto tx_exp = ofdm::tx(const_syms, 8); // multiplexing
    if (!tx_exp) return;

    auto& tx = *tx_exp;

    ofdm::rx(tx, 8) // demultiplexing
        .transform([](auto&& rx) -> std::expected<void, std::string>
        {
            auto bytes = ofdm::from_constellations<ofdm::e16QAM>(rx);
            g_decoded_text.push_back(bytes.begin(), bytes.end());
            return {};
        });

    // ----------- Time-domain plot -----------
    {
        std::lock_guard lock(mtx);
        g_time_plot.push_back(tx.begin(), tx.end());

        size_t N = g_time_plot.size();
        QVector<double> x(N), re(N), im(N);

        for (size_t i = 0; i < N; ++i)
        {
            auto v = g_time_plot.at(i);
            if (!v) continue;
            x[i]  = i;
            re[i] = v->real();
            im[i] = v->imag();
        }

        timePlot->graph(0)->setData(x, re);
        timePlot->graph(1)->setData(x, im);
        timePlot->xAxis->setRange(N > 512 ? N - 512 : 0, N);
        timePlot->replot();
    }

    // ----------- Constellation plot -----------
    {
        QVector<double> x(const_syms.size()), y(const_syms.size());
        for (size_t i = 0; i < const_syms.size(); ++i)
        {
            x[i] = const_syms[i].real();
            y[i] = const_syms[i].imag();
        }
        constPlot->graph(0)->setData(x, y);
        constPlot->replot();
    }

    // ----------- Running text (WORKING) -----------
    std::string s(g_decoded_text.begin(), g_decoded_text.end());
    textLabel->setText(QString::fromStdString(s));
}
