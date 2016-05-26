#ifndef DIALOG_H
#define DIALOG_H
#include <libusb-1.0/libusb.h>
#include <QList>
#include <QCommandLineParser>

#ifdef QT_GUI_LIB
   #include <QDialog>
   class QAbstractButton;
   class QGroupBox;
   namespace Ui {
   class Dialog;
   }
#else
   #include <QObject>
#endif
class QSocketNotifier;

#ifdef QT_GUI_LIB
class Dialog : public QDialog {
   Q_OBJECT
public:
   explicit Dialog(QCommandLineParser* cmdline, QWidget *parent = 0);
private:
   Ui::Dialog *ui;
   virtual void closeEvent(QCloseEvent *event);
   void pauseContinueUSB(QAbstractButton *button);
   int sumOfFlags(QGroupBox* groupbox ) const;
#else
class Dialog : public QObject {
   Q_OBJECT
public:
   explicit Dialog(QCommandLineParser* cmdline, QObject *parent = 0);
#endif
public:
   ~Dialog();
private:
   void initialize();
   enum class LogLevel {EMERG = 0, //        System is unusable
                        ALERT = 1, //        Action must be taken immediately
                        CRIT =  2, //        Critical conditions
                        ERR =   3, //        Error conditions
                        WARNING=4, //        Warning conditions
                        NOTICE =5, //        Normal but significant condition
                        INFO =  6, //        Informational
                        DEBUG = 7};//        Debug-level messages

   void logMessage(LogLevel level, QString message);
   LogLevel logLevel;
   void processCpenStatusCode( int code );
   // USB handling
   uint16_t vendor;
   uint16_t product;
   int bus;
   int device;
   int configuration;
   int interface;
   int altsetting;
   struct endpoint {
      unsigned char address;
      libusb_transfer_type transferType;
      uint16_t maxPacketSize;
   };
   endpoint epIn;
   endpoint epOut;
   libusb_device_handle *dev_handle;
   libusb_transfer* transfer;
//   bool readUsbSettings();
   libusb_device_handle* openUsbDevice();
   bool prepareUsbDevice();
   bool prepareCallbacks();
   //QList<int> pollfdIn;
   //QList<int> pollfdOut;
   QList<QSocketNotifier*> socketNotifiers;
   static void pollfdAddedCb(int fd, short events, void *user_data) {((Dialog*)user_data)->pollfdAddedCb(fd, events);}
   static void pollfdRemovedCb(int fd, void *user_data) {((Dialog*)user_data)->pollfdRemovedCb(fd);}
   void pollfdAddedCb(int fd, short events);
   void pollfdRemovedCb(int fd);
   void handleLibusbEvent();
   bool receiveData();
   static void staticTransferCbFn(struct libusb_transfer *transfer);
   void libusbTransferCbFn(struct libusb_transfer *transfer); //callback function for result of libusb_submit_transfer()
   QLatin1String transferTypeFromEnum(quint8 transferType) const;
   //  cpenlib_backend handling
   enum class ReportButtonStatus : bool { Yes = true, No = false } reportButtonStatus;
   static void signalHandler(int signalnumber);   // Unix signal handler
   static int signalPipe[2];                // pipe connecting unix and Qt (socket)
   QSocketNotifier *unixSignal;
   void processCpenEvent();                 // Qt signal handlers
#ifdef QT_GUI_LIB
   bool pauseUSB;
   QList<QPixmap> imageList;
   int currentImage;
   void nextImage();
   void previousImage();
   void setMemoryOutput();
   void setFileOutput();
   void setDirection_r2l();
   void setDirection_l2r();
   void setDirection_2D();
#endif
signals:
   void requestData();
};

#endif // DIALOG_H
