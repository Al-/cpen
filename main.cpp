#include "dialog.h"
#ifdef QT_GUI_LIB
   #include <QApplication>
#else
   #include <QCoreApplication>
#endif
#include <QCommandLineParser>
#include "global.h"

int main(int argc, char *argv[]) {
#ifdef QT_GUI_LIB
   QApplication a(argc, argv);
#else
   QCoreApplication a(argc, argv);
#endif
   QCoreApplication::setApplicationName(application);
   QCoreApplication::setApplicationVersion(QLatin1String("1.0"));
   // QCoreApplication::setOrganizationDomain(domain);
   QCoreApplication::setOrganizationName(organization);
   QCommandLineParser parser;
   parser.setApplicationDescription(QCoreApplication::translate("commandline parser",
      "Example implementation of an application that uses libcpen_backend to support the hand-held line scanners "
      "C-Pen on linux. The USB options have the appropriate default values These options should only be used if "
      "the C-Pen manufacturer changes some settings (however, unluckily, in that case it is unlikely that the "
      "library remains functional).\n\n"
      "Depending on defines set at compilation, this app works as Qt GUI application, best suited to explore the "
      "options offered by libcpen_backend; or as commandline interface to retrieve individual images.\n\n"
      "See documentation of libcpen_backend for more information."));
   parser.addHelpOption();
   parser.addVersionOption();
   bool ok;
   QCommandLineOption loglevelOption(QLatin1String("loglevel"),
               QCoreApplication::translate("commandline parser", "Specifies the required logging level (0 - 7). Default is 4 (Warning)."),
               QCoreApplication::translate("commandline parser", "loglevel"), QLatin1String("4"));
   ok = parser.addOption(loglevelOption);
   Q_ASSERT(ok);
   // usb options
   QCommandLineOption vendorOption(QLatin1String("vendor"),
               QCoreApplication::translate("commandline parser", "Vendor Id of the USB device."),
               QCoreApplication::translate("commandline parser", "vendorId"), QLatin1String("2707"));
   ok = parser.addOption(vendorOption);
   Q_ASSERT(ok);
   QCommandLineOption productOption(QLatin1String("product"),
               QCoreApplication::translate("commandline parser", "Product Id of the USB device."),
               QCoreApplication::translate("commandline parser", "productId"), QLatin1String("267"));
   ok = parser.addOption(productOption);
   Q_ASSERT(ok);
   QCommandLineOption configurationOption(QLatin1String("configuration"),
               QCoreApplication::translate("commandline parser", "Configuration number of the USB device."),
               QCoreApplication::translate("commandline parser", "configuration"), QLatin1String("1"));
   ok = parser.addOption(configurationOption);
   Q_ASSERT(ok);
   QCommandLineOption interfaceOption(QLatin1String("interface"),
               QCoreApplication::translate("commandline parser", "Interface number of the USB device."),
               QCoreApplication::translate("commandline parser", "interface"), QLatin1String("0"));
   ok = parser.addOption(interfaceOption);
   Q_ASSERT(ok);
   QCommandLineOption alternativeOption(QLatin1String("alternative"),
               QCoreApplication::translate("commandline parser", "Alternative setting of the USB device."),
               QCoreApplication::translate("commandline parser", "alternative"), QLatin1String("0"));
   ok = parser.addOption(alternativeOption);
   Q_ASSERT(ok);
   QCommandLineOption busOption(QLatin1String("bus"),
               QCoreApplication::translate("commandline parser", "Bus address of the USB device."),
               QCoreApplication::translate("commandline parser", "bus"), QLatin1String("1"));
   ok = parser.addOption(busOption);
   Q_ASSERT(ok);
   QCommandLineOption deviceOption(QLatin1String("device"),
               QCoreApplication::translate("commandline parser", "Device address of the USB device."),
               QCoreApplication::translate("commandline parser", "device"), QLatin1String("4"));
   ok = parser.addOption(deviceOption);
   Q_ASSERT(ok);
   QCommandLineOption endpointOption(QLatin1String("endpoint"),
               QCoreApplication::translate("commandline parser", "Endpoint used by the USB device."),
               QCoreApplication::translate("commandline parser", "endpoint"), QLatin1String("2"));
   ok = parser.addOption(endpointOption);
   Q_ASSERT(ok);
   // libcpen_backend options
   QCommandLineOption fileoutputOption(QLatin1String("file"),
               QCoreApplication::translate("commandline parser", "Specifies which images to write to file (or'ed; see enum cpen_Output_flag in libcpen_backend. Default is 0 (no output)."),
               QCoreApplication::translate("commandline parser", "fileoutput"), QLatin1String("0"));
   ok = parser.addOption(fileoutputOption);
   Q_ASSERT(ok);
   QCommandLineOption memoryoutputOption(QLatin1String("memory"),
               QCoreApplication::translate("commandline parser", "Specifies which images to write to stdout (or'ed; see enum cpen_Output_flag in libcpen_backend. Default is 32 (only final image)."),
               QCoreApplication::translate("commandline parser", "memoryoutput"), QLatin1String("32"));
   ok = parser.addOption(memoryoutputOption);
   Q_ASSERT(ok);
   QCommandLineOption signalOption(QLatin1String("signal"),
               QCoreApplication::translate("commandline parser", "Unix signal number that library should use to alert caller (0 = no signal). Default is 10 (= SIGUSR1)."),
               QCoreApplication::translate("commandline parser", "signal"), QLatin1String("10"));
   ok = parser.addOption(signalOption);
   Q_ASSERT(ok);
   QCommandLineOption scanmodeOption(QLatin1String("scanmode"),
               QCoreApplication::translate("commandline parser", "Selected numeric scan mode; see enum cpen_Scanmode in libcpen_backend. Default is 1 (left to right)."),
               QCoreApplication::translate("commandline parser", "scanmode"), QLatin1String("1"));
   ok = parser.addOption(scanmodeOption);
   Q_ASSERT(ok);
   QCommandLineOption reportbuttonstatusOption(QLatin1String("button"),
               QCoreApplication::translate("commandline parser", "Report button status changes on stderr as numeric code."));
   ok = parser.addOption(reportbuttonstatusOption);
   Q_ASSERT(ok);
   parser.process(a);
   Dialog w(&parser);
#ifdef QT_GUI_LIB
   w.show();
#endif
   return a.exec();}
