#include "gui/MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("CUHK");
    QCoreApplication::setApplicationName("Person Search Annotation");

    MainWindow w;
    w.show();

    return a.exec();
}
