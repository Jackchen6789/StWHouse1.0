#include "mainwindow.h"
#include <QApplication>
#include <QNetworkProxyFactory>

int main(int argc, char *argv[])
{
    // ??????????? "proxy type is invalid" ??
    QNetworkProxyFactory::setUseSystemConfiguration(false);

    QApplication a(argc, argv);

    MainWindow w;

    if (!w.showLoginDialog().isEmpty()) {
        w.show();
        return QApplication::exec();
    }

    return 0;
}
