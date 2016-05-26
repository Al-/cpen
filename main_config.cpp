#include "dialog_config.h"
#include "global.h"
#include <QApplication>

int main(int argc, char *argv[])
{
   QApplication a(argc, argv);
   QCoreApplication::setOrganizationDomain(domain);
   QCoreApplication::setOrganizationName(organization);
   QCoreApplication::setApplicationName(application);
   QCoreApplication::setApplicationVersion(QLatin1String("0.1"));
   Dialog w;
   w.show();

   return a.exec();
}
