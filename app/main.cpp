#include "QFDemoWindow.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    OFDMDemoWindow window;
    window.resize(1200, 800);
    window.show();

    return app.exec();
}