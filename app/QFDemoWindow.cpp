#include "QFDemoWindow.hpp"
#include "ofdm.hpp"
#include "modulation.hpp"
#include "sliding_buffer.hpp"
#include "channel.hpp"

#include <complex>
#include <mutex>
#include <string>
#include <deque>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QGroupBox>
#include <QEvent>
#include <QSlider>
#include <QTimer>
#include <QTextEdit>
#include <QLineEdit>
#include <QPen>

constexpr int   CONST_HISTORY = 10;
constexpr float ALPHA_DECAY   = 0.80f;

std::mutex                                  mtx;
size_t                                      payloadPos = 0;
const std::string                           payloadConst =
    "Hello, world! "
    "I am a Software-Defined Radio Stack.          "
    "This string is a result of demultiplexing a 16-QAM "
    "multiplexed OFDM signal. Khartia time has come. Русні пизда!!!!!!!         ";

constexpr auto                              noiseLvl = 26.0d; // dB
utils::sliding_buffer<std::complex<float>>  txTimeBuf(512);
utils::sliding_buffer<std::complex<float>>  rxTimeBuf(512);
utils::sliding_buffer<std::complex<float>>  noiseBuf(512);
utils::sliding_buffer<uint8_t>              rxTextBuf(128);
channel::awgn                               noise{noiseLvl};

OFDMDemoWindow::OFDMDemoWindow()
{
    setWindowTitle("Software-Defined Radio Stack Demo (TX → Channel → RX)");

    auto* central = new QWidget;
    auto* mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(10);
    central->setLayout(mainLayout);
    setCentralWidget(central);

    QFont captionFont;
    captionFont.setPointSize(14);

    QFont textFont;
    textFont.setPointSize(20);

    // -------- fixed row heights (visual contract) --------
    constexpr int CONSTELLATION_H = 220;
    constexpr int TIME_H          = 220;

    // ================== TX | RX ==================
    auto* trxLayout = new QHBoxLayout;
    mainLayout->addLayout(trxLayout);

    // ================== TX ==================
    auto* txGroup = new QGroupBox;
    txGroup->setFont(captionFont);
    auto* txLayout = new QVBoxLayout(txGroup);
    txLayout->setContentsMargins(0, 0, 0, 0);

    // --- TX constellation ---
    txConstPlot = new QCustomPlot;
    txConstPlot->setMouseTracking(true);
    txConstPlot->setAttribute(Qt::WA_Hover, true);
    txConstPlot->installEventFilter(this);
    txConstPlot->addGraph();
    txConstPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
    txConstPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssStar, 6));
    txConstPlot->xAxis->setLabel("Re");
    txConstPlot->yAxis->setLabel("Im");
    txConstPlot->xAxis->setRange(-1.1, 1.1);
    txConstPlot->yAxis->setRange(-1.1, 1.1);

    auto* txConstRow = new QWidget;
    auto* txConstRowLayout = new QVBoxLayout(txConstRow);
    txConstRowLayout->setContentsMargins(0,0,0,0);
    txConstRowLayout->addWidget(txConstPlot);
    txConstRow->setFixedHeight(CONSTELLATION_H);

    // --- TX time ---
    txTimePlot = new QCustomPlot;
    txTimePlot->setMouseTracking(true);
    txTimePlot->setAttribute(Qt::WA_Hover, true);
    txTimePlot->installEventFilter(this);
    txTimePlot->legend->setVisible(true);
    txTimePlot->setAutoAddPlottableToLegend(true);
    txTimePlot->addGraph();
    txTimePlot->addGraph();
    txTimePlot->graph(0)->setName("Re{x[n]}");
    txTimePlot->graph(1)->setName("Im{x[n]}");
    txTimePlot->graph(0)->setPen(QPen(Qt::blue));
    txTimePlot->graph(1)->setPen(QPen(Qt::red));
    txTimePlot->xAxis->setLabel("n");
    txTimePlot->yAxis->setLabel("Amplitude");
    txTimePlot->yAxis->setRange(-1.1, 1.1);

    auto* txTimeRow = new QWidget;
    auto* txTimeRowLayout = new QVBoxLayout(txTimeRow);
    txTimeRowLayout->setContentsMargins(0,0,0,0);
    txTimeRowLayout->addWidget(txTimePlot);
    txTimeRow->setFixedHeight(TIME_H);

    // --- TX text ---
    txTextEdit = new QTextEdit;
    txTextEdit->setFont(textFont);
    txTextEdit->setPlaceholderText("Enter text to transmit...");
    txTextEdit->setText(QString::fromStdString(payloadConst));
    txTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* txTextRow = new QWidget;
    auto* txTextRowLayout = new QVBoxLayout(txTextRow);
    txTextRowLayout->setContentsMargins(0,0,0,0);
    txTextRowLayout->addWidget(txTextEdit);
    txTextRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // --- Add TX rows with stretch factors ---
    txLayout->addWidget(txConstRow, 3);
    txLayout->addWidget(txTimeRow, 3);
    txLayout->addWidget(txTextRow, 1);

    trxLayout->addWidget(txGroup, 1);

    // ================== RX ==================
    auto* rxGroup = new QGroupBox;
    rxGroup->setFont(captionFont);
    auto* rxLayout = new QVBoxLayout(rxGroup);
    rxLayout->setContentsMargins(0, 0, 0, 0);

    // --- RX constellation ---
    rxConstPlot = new QCustomPlot;
    rxConstPlot->setMouseTracking(true);
    rxConstPlot->setAttribute(Qt::WA_Hover, true);
    rxConstPlot->installEventFilter(this);
    rxConstPlot->addGraph();
    rxConstPlot->graph(0)->setLineStyle(QCPGraph::lsNone);
    rxConstPlot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssStar, 6));
    rxConstPlot->xAxis->setLabel("Re");
    rxConstPlot->yAxis->setLabel("Im");
    rxConstPlot->xAxis->setRange(-1.1, 1.1);
    rxConstPlot->yAxis->setRange(-1.1, 1.1);

    auto* rxConstRow = new QWidget;
    auto* rxConstRowLayout = new QVBoxLayout(rxConstRow);
    rxConstRowLayout->setContentsMargins(0,0,0,0);
    rxConstRowLayout->addWidget(rxConstPlot);
    rxConstRow->setFixedHeight(CONSTELLATION_H);

    // --- RX time ---
    rxTimePlot = new QCustomPlot;
    rxTimePlot->setMouseTracking(true);
    rxTimePlot->setAttribute(Qt::WA_Hover, true);
    rxTimePlot->installEventFilter(this);
    rxTimePlot->legend->setVisible(true);
    rxTimePlot->setAutoAddPlottableToLegend(true);
    rxTimePlot->addGraph();
    rxTimePlot->addGraph();
    rxTimePlot->graph(0)->setName("Re{x[n]}");
    rxTimePlot->graph(1)->setName("Im{x[n]}");
    rxTimePlot->graph(0)->setPen(QPen(Qt::blue));
    rxTimePlot->graph(1)->setPen(QPen(Qt::red));
    rxTimePlot->xAxis->setLabel("n");
    rxTimePlot->yAxis->setLabel("Amplitude");
    rxTimePlot->yAxis->setRange(-1.1, 1.1);

    auto* rxTimeRow = new QWidget;
    auto* rxTimeRowLayout = new QVBoxLayout(rxTimeRow);
    rxTimeRowLayout->setContentsMargins(0,0,0,0);
    rxTimeRowLayout->addWidget(rxTimePlot);
    rxTimeRow->setFixedHeight(TIME_H);

    // --- RX decoded text ---
    rxTextLabel = new QLabel;
    rxTextLabel->setFont(textFont);
    rxTextLabel->setWordWrap(true);
    rxTextLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    rxTextLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* rxScroll = new QScrollArea;
    rxScroll->setWidgetResizable(true);
    rxScroll->setFrameShape(QFrame::NoFrame);
    rxScroll->setWidget(rxTextLabel);
    rxScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* rxTextRow = new QWidget;
    auto* rxTextRowLayout = new QVBoxLayout(rxTextRow);
    rxTextRowLayout->setContentsMargins(0,0,0,0);
    rxTextRowLayout->addWidget(rxScroll);
    rxTextRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // --- Add RX rows with stretch factors ---
    rxLayout->addWidget(rxConstRow, 3);
    rxLayout->addWidget(rxTimeRow, 3);
    rxLayout->addWidget(rxTextRow, 1);

    trxLayout->addWidget(rxGroup, 1);

    // ================== CHANNEL ==================
    auto* channelGroup = new QGroupBox("Channel");
    channelGroup->setFont(captionFont);
    auto* channelLayout = new QHBoxLayout(channelGroup);

    noisePlot = new QCustomPlot;
    noisePlot->yAxis->setRange(-0.5, 0.5);
    noisePlot->setMouseTracking(true);
    noisePlot->setAttribute(Qt::WA_Hover, true);
    noisePlot->installEventFilter(this);
    noisePlot->legend->setVisible(true);
    noisePlot->setAutoAddPlottableToLegend(true);
    noisePlot->addGraph();
    noisePlot->addGraph();
    noisePlot->graph(0)->setName("Re{AWGN[n]}");
    noisePlot->graph(1)->setName("Im{AWGN[n]}");
    noisePlot->graph(0)->setPen(QPen(Qt::blue));
    noisePlot->graph(1)->setPen(QPen(Qt::red));
    noisePlot->xAxis->setLabel("n");
    noisePlot->yAxis->setLabel("Amplitude");

    noisePlot->setMinimumHeight(220);
    channelLayout->addWidget(noisePlot, 4);

    auto* noiseBox = new QGroupBox("SNR (dB)");
    auto* noiseLayout = new QVBoxLayout(noiseBox);
    noiseEdit = new QLineEdit(QString::number(noiseLvl, 'f', 1));
    noiseLayout->addWidget(noiseEdit);
    noiseBox->setMaximumWidth(140);

    connect(noiseEdit, &QLineEdit::editingFinished, this, [this] {
        noise.snr(noiseEdit->text().toFloat());
    });

    channelLayout->addWidget(noiseBox);

    auto* speedBox = new QGroupBox("(De)mux speed");
    auto* speedLayout = new QVBoxLayout(speedBox);
    speedBox->setMaximumWidth(220);

    speedLabel = new QLabel("Update interval: 50 ms");
    speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setRange(1, 200);
    speedSlider->setValue(50);
    speedSlider->setTickInterval(10);
    speedSlider->setTickPosition(QSlider::TicksBelow);

    speedSlider->setStyleSheet(R"(
        QGroupBox { margin-top: 0px; }
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
    channelLayout->addWidget(speedBox);

    mainLayout->addWidget(channelGroup);

    // ================== TIMER ==================
    timer = new QTimer(this);
    timer->setInterval(50);

    connect(timer, &QTimer::timeout, this, &OFDMDemoWindow::updateFrame);
    connect(speedSlider, &QSlider::valueChanged, this, [this](int v) {
        timer->setInterval(v);
        speedLabel->setText(QString("Update interval: %1 ms").arg(v));
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
        if (obj == txTimePlot)  hoverOn(txTimePlot);
        if (obj == txConstPlot) hoverOn(txConstPlot);
        if (obj == rxTimePlot)  hoverOn(rxTimePlot);
        if (obj == rxConstPlot) hoverOn(rxConstPlot);
        if (obj == noisePlot) hoverOn(noisePlot);
    }
    else if (event->type() == QEvent::HoverLeave)
    {
        if (obj == txTimePlot)  hoverOff(txTimePlot);
        if (obj == txConstPlot) hoverOff(txConstPlot);
        if (obj == rxTimePlot)  hoverOff(rxTimePlot);
        if (obj == rxConstPlot) hoverOff(rxConstPlot);
        if (obj == noisePlot) hoverOff(noisePlot);
    }

    return QMainWindow::eventFilter(obj, event);
}

void OFDMDemoWindow::updateFrame()
{
    static std::deque<QVector<QPointF>> txConstHist;
    static std::deque<QVector<QPointF>> rxConstHist;

    auto payload = txTextEdit->toPlainText().toStdString();
    std::vector<uint8_t> input;
    for (int i = 0; i < 4; ++i)
    {
        input.push_back(payload[payloadPos % payload.size()]);
        ++payloadPos;
    }

    auto tx_const_syms = modulation::to_constl<modulation::e16QAM>(input); // bits encoding
    if (tx_const_syms.empty()) return;
    // constellation
    {
        QVector<QPointF> pts;
        for (auto& s : tx_const_syms)
            pts.emplace_back(s.real(), s.imag());

        txConstHist.push_front(pts);
        if (txConstHist.size() > CONST_HISTORY)
            txConstHist.pop_back();

        txConstPlot->clearGraphs();

        for (size_t i = 0; i < txConstHist.size(); ++i)
        {
            auto* g = txConstPlot->addGraph();
            g->setLineStyle(QCPGraph::lsNone);

            QColor c(0, 0, 160);
            c.setAlphaF(std::pow(ALPHA_DECAY, i));

            g->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssStar, 6));
            g->setPen(QPen(c));
            g->setBrush(c);

            QVector<double> x, y;
            for (auto& p : txConstHist[i])
            {
                x.push_back(p.x());
                y.push_back(p.y());
            }
            g->setData(x, y);
        }

        txConstPlot->replot(QCustomPlot::rpQueuedReplot);
    }

    auto tx_exp = ofdm::tx(tx_const_syms, 8); // multiplexing
    if (!tx_exp) return;

    auto& tx = *tx_exp;
    // TX time domain
    {
        std::lock_guard lock(mtx);
        txTimeBuf.push_back(tx.begin(), tx.end());

        size_t N = txTimeBuf.size();
        QVector<double> x(N), re(N), im(N);

        for (size_t i = 0; i < N; ++i)
            txTimeBuf.at(i)
                .and_then([&](auto&& v) -> std::expected<std::complex<double>, std::string>
                {
                    x[i]  = i;
                    re[i] = v.real();
                    im[i] = v.imag();
                    return v;
                });

        txTimePlot->graph(0)->setData(x, re);
        txTimePlot->graph(1)->setData(x, im);
        txTimePlot->xAxis->setRange(N > 512 ? N - 512 : 0, N);
        txTimePlot->replot();
    }

    auto ns = noise.apply(tx, true);
    // Noise time domain
    {
        std::lock_guard lock(mtx);
        noiseBuf.push_back(ns.begin(), ns.end());

        size_t N = noiseBuf.size();
        QVector<double> x(N), re(N), im(N);

        for (size_t i = 0; i < N; ++i)
            noiseBuf.at(i)
                .and_then([&](auto&& v) -> std::expected<std::complex<double>, std::string>
                {
                    x[i]  = i;
                    re[i] = v.real();
                    im[i] = v.imag();
                    return v;
                });

        noisePlot->graph(0)->setData(x, re);
        noisePlot->graph(1)->setData(x, im);
        noisePlot->xAxis->setRange(N > 512 ? N - 512 : 0, N);
        noisePlot->replot();
    }

    // RX time domain. Channel noise is already in the tx-var
    {
        std::lock_guard lock(mtx);
        rxTimeBuf.push_back(tx.begin(), tx.end());

        size_t N = rxTimeBuf.size();
        QVector<double> x(N), re(N), im(N);

        for (size_t i = 0; i < N; ++i)
            rxTimeBuf.at(i)
                .and_then([&](auto&& v) -> std::expected<std::complex<double>, std::string>
                {
                    x[i]  = i;
                    re[i] = v.real();
                    im[i] = v.imag();
                    return v;
                });

        rxTimePlot->graph(0)->setData(x, re);
        rxTimePlot->graph(1)->setData(x, im);
        rxTimePlot->xAxis->setRange(N > 512 ? N - 512 : 0, N);
        rxTimePlot->replot();
    }

    auto rx_exp = ofdm::rx(tx, 8); // demultiplexing
    if (!rx_exp) return;

    auto& rx_const_syms = *rx_exp;
    auto bytes = modulation::from_constl<modulation::e16QAM>(rx_const_syms); // bits decoding
    rxTextBuf.push_back(bytes.begin(), bytes.end());

    // constellation
    {
        QVector<QPointF> pts;
        for (auto& s : rx_const_syms)
            pts.emplace_back(s.real(), s.imag());

        rxConstHist.push_front(pts);
        if (rxConstHist.size() > CONST_HISTORY)
            rxConstHist.pop_back();

        rxConstPlot->clearGraphs();

        for (size_t i = 0; i < rxConstHist.size(); ++i)
        {
            auto* g = rxConstPlot->addGraph();
            g->setLineStyle(QCPGraph::lsNone);

            QColor c(0, 0, 160);
            c.setAlphaF(std::pow(ALPHA_DECAY, i));

            g->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssStar, 6));
            g->setPen(QPen(c));
            g->setBrush(c);

            QVector<double> x, y;
            for (auto& p : rxConstHist[i])
            {
                x.push_back(p.x());
                y.push_back(p.y());
            }
            g->setData(x, y);
        }

        rxConstPlot->replot(QCustomPlot::rpQueuedReplot);
    }

    // running text
    std::string s(rxTextBuf.begin(), rxTextBuf.end());
    rxTextLabel->setText(QString::fromStdString(s));
}
