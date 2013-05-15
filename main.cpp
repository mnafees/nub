// nub
#include "nub.h"

// Qt
#include <QApplication>

int main( int argc, char *argv[] )
{
    QApplication app( argc, argv );
    nub mainWindow;
    mainWindow.show();
    return app.exec();
}
