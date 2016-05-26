#include <QSettings>
#include <QTimer>
#include <poll.h>
#include <QSocketNotifier>
#include <QDebug>
#ifdef QT_GUI_LIB
   #include <QPushButton>
   #include <QPixmap>
   #include "ui_dialog.h"
   #include <QMessageBox>
#else
   #include <QObject>
   #include <iostream>
#endif
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "dialog.h"
#include "global.h"
#include "cpen_backend.h"

// requires: sudo apt-get install libusb-1.0-0-dev
// see file:///usr/share/doc/libusb-1.0-doc/html/modules.html
#include <libusb-1.0/libusb.h>
/* libusb_error
   LIBUSB_SUCCESS = 0,
   LIBUSB_ERROR_IO = -1,
   LIBUSB_ERROR_INVALID_PARAM = -2,
   LIBUSB_ERROR_ACCESS = -3,
   LIBUSB_ERROR_NO_DEVICE = -4,
   LIBUSB_ERROR_NOT_FOUND = -5,
   LIBUSB_ERROR_BUSY = -6,
   LIBUSB_ERROR_TIMEOUT = -7,
   LIBUSB_ERROR_OVERFLOW = -8,
   LIBUSB_ERROR_PIPE = -9,
   LIBUSB_ERROR_INTERRUPTED = -10,
   LIBUSB_ERROR_NO_MEM = -11,
   LIBUSB_ERROR_NOT_SUPPORTED = -12,
   LIBUSB_ERROR_OTHER = -99 */

int Dialog::signalPipe[2];

#ifdef QT_GUI_LIB
Dialog::Dialog(QCommandLineParser *cmdline, QWidget *parent) : QDialog(parent), ui(new Ui::Dialog), pauseUSB(true), currentImage(-1),
#else
Dialog::Dialog(QCommandLineParser* cmdline, QObject* parent) : QObject(parent),
#endif
               dev_handle(nullptr), transfer(nullptr),
               unixSignal(nullptr) {
   bool connectOk;
#ifdef QT_GUI_LIB
   ui->setupUi(this);
   ui->usbButtonbox->button(QDialogButtonBox::Yes)->setText(tr("continue"));
   ui->usbButtonbox->button(QDialogButtonBox::No)->setText(tr("pause"));
   ui->usbButtonbox->button(QDialogButtonBox::No)->setEnabled(false);
   ui->playerButtonbox->button(QDialogButtonBox::Yes)->setText(tr("next"));
   ui->playerButtonbox->button(QDialogButtonBox::Yes)->setEnabled(false);
   connectOk = connect(ui->playerButtonbox, &QDialogButtonBox::accepted, this, &Dialog::nextImage);
   Q_ASSERT(connectOk);
   ui->playerButtonbox->button(QDialogButtonBox::No)->setText(tr("previous"));
   ui->playerButtonbox->button(QDialogButtonBox::No)->setEnabled(false);
   connectOk = connect(ui->playerButtonbox, &QDialogButtonBox::rejected, this, &Dialog::previousImage);
   Q_ASSERT(connectOk);
   ui->mainButtonbox->button(QDialogButtonBox::Close)->setEnabled(false);
   connectOk = connect(this->ui->usbButtonbox, &QDialogButtonBox::clicked, this, &Dialog::pauseContinueUSB);
   Q_ASSERT(connectOk);
   foreach ( QObject* widget, ui->memoryoutputBox->children() ) if ( widget->isWidgetType() ) {
      QCheckBox* checkbox = qobject_cast< QCheckBox* >(widget) ;
      Q_ASSERT_X(checkbox, "not a checkbox", widget->objectName().toLatin1());
      connectOk = connect( checkbox, &QCheckBox::toggled, this, &Dialog::setMemoryOutput );
      Q_ASSERT(connectOk);}
   foreach ( QObject* widget, ui->fileoutputBox->children() ) if ( widget->isWidgetType() ) {
      QCheckBox* checkbox = qobject_cast< QCheckBox* >(widget) ;
      Q_ASSERT_X(checkbox, "not a checkbox", widget->objectName().toLatin1());
      connectOk = connect( checkbox, &QCheckBox::toggled, this, &Dialog::setFileOutput );
      Q_ASSERT(connectOk);}
   connectOk = connect(ui->l2r, &QRadioButton::clicked, this, &Dialog::setDirection_l2r);
   Q_ASSERT(connectOk);
   connectOk = connect(ui->r2l, &QRadioButton::clicked, this, &Dialog::setDirection_r2l);
   Q_ASSERT(connectOk);
   connectOk = connect(ui->dimension2, &QRadioButton::clicked, this, &Dialog::setDirection_2D);
   Q_ASSERT(connectOk);
   ui->directoryPath->setText(QLatin1String(cpen_getbitmapdirectory()));
#endif
   connectOk = connect(this, &Dialog::requestData, this, &Dialog::receiveData);
   Q_ASSERT(connectOk);
   // set default settings
   bool ok;
   logLevel = LogLevel(cmdline->value(QLatin1String("loglevel")).toInt(&ok));
   Q_ASSERT(ok);
   cpen_Scanmode scanmode(cpen_Scanmode(cmdline->value(QLatin1String("scanmode")).toInt(&ok)));
   Q_ASSERT(ok);
   cpen_set_scanmode( scanmode );
   int memoryoutput(cpen_Scanmode(cmdline->value(QLatin1String("memory")).toInt(&ok)));
   Q_ASSERT(ok);
   cpen_set_memoryoutput(memoryoutput);
   int fileoutput((cpen_Scanmode)cmdline->value(QLatin1String("memory")).toInt(&ok));
   Q_ASSERT(ok);
   cpen_set_fileoutput(fileoutput);
   int selectedSignal(cmdline->value(QLatin1String("signal")).toInt(&ok));
   Q_ASSERT(ok);
   if ( selectedSignal != 0 ) {
      cpen_set_signal(selectedSignal);
      if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalPipe))
            qFatal("Couldn't create socketpair to forward unix signal to Qt");
      unixSignal = new QSocketNotifier(signalPipe[1], QSocketNotifier::Read, this);
      Q_ASSERT(unixSignal);
      if ( !connect(unixSignal, &QSocketNotifier::activated, this, &Dialog::processCpenEvent) )
            qFatal("Couldn't connect function to Qt signal");
      struct sigaction signalaction;
      signalaction.sa_handler = &Dialog::signalHandler;
      sigemptyset(&signalaction.sa_mask);
      signalaction.sa_flags = 0;
   //   signalaction.sa_flags |= SA_RESTART;
      if (::sigaction(selectedSignal, &signalaction, NULL) != 0)
            qFatal( "Couldn't capture unix signal" ); }
   reportButtonStatus = ReportButtonStatus(cmdline->isSet(QLatin1String("button")));
#ifdef QT_GUI_LIB        // update dialog with currenr settings
   switch (scanmode) {
      case cpen_1D_l2r:
         ui->l2r->setChecked(true);
         break;
      case cpen_1D_r2l:
         ui->r2l->setChecked(true);
         break;
      case cpen_2D:
         ui->r2l->setChecked(true);
         break;}
   ui->memoryChar->setChecked(memoryoutput & cpen_char);
   ui->memoryWord->setChecked(memoryoutput & cpen_word);
   ui->memoryRaw->setChecked(memoryoutput & cpen_raw);
   ui->memoryResized->setChecked(memoryoutput & cpen_resized);
   ui->memoryLine->setChecked(memoryoutput & cpen_line);
   ui->memoryDisplacement->setChecked(memoryoutput & cpen_displacement);
   ui->fileChar->setChecked(fileoutput & cpen_char);
   ui->fileWord->setChecked(fileoutput & cpen_word);
   ui->fileRaw->setChecked(fileoutput & cpen_raw);
   ui->fileResized->setChecked(fileoutput & cpen_resized);
   ui->fileLine->setChecked(fileoutput & cpen_line);
   ui->fileDisplacement->setChecked(fileoutput & cpen_displacement);
#endif
   QTimer::singleShot(0, this, &Dialog::initialize);}

Dialog::~Dialog() {
   int result;
   libusb_set_pollfd_notifiers(NULL, NULL, NULL, nullptr);
   if (transfer) libusb_free_transfer(transfer);
   if (interface >= 0) {
      result = libusb_release_interface(dev_handle, interface);
      Q_ASSERT_X(result == LIBUSB_SUCCESS || result == LIBUSB_ERROR_NOT_FOUND, "libusb_release_interface", libusb_error_name(result));}
   libusb_close(dev_handle);
   libusb_exit(NULL);
#ifdef QT_GUI_LIB
   delete ui;
#endif
}

void Dialog::logMessage(Dialog::LogLevel level, QString message){
   if (level > logLevel) return;
#ifdef QT_GUI_LIB
   ui->logEdit->appendPlainText(message);
#else
   std::cerr << (int)level << " " << message.toLatin1().data() << std::endl;
#endif
}

void Dialog::processCpenStatusCode( int code ) {
   Q_ASSERT_X( code > 0, "code (that should come from libcpen_backend)", QString::number(code).toLatin1() ); // else no status code (or success) that should be handled differently
   if ( code >= 0x010000 ) {   // button status
#ifdef QT_GUI_LIB
      ui->checkTipButton->setChecked( code & 0x01 );
      ui->checkSideButton->setChecked( code & 0x02 );
#else
      if ( reportButtonStatus == ReportButtonStatus::Yes ) std::cerr << (code & 0x3) << std::endl;
#endif
      }
   else {                        // error code
#ifdef QT_GUI_LIB
      ui->lcdError->display( code );
#else
      logMessage( LogLevel::ERR, tr("libcpen_backend reports error code 0x%1").arg(QString::number(code, 16)));
#endif
      }}

void Dialog::initialize() {
   bool ok(true);
//   = readUsbSettings();
   /// \todo use commandline arguments
   vendor = 2707;
   product = 267;
   configuration = 1;
   interface = 0;
   altsetting = 0;
   bus = -1;
   device = -5;
   epIn.address =2;
   epOut.address = -1;
   if (ok) dev_handle = this->openUsbDevice();
   if (dev_handle) ok = this->prepareUsbDevice();
   else ok = false;
   if (ok) ok = this->prepareCallbacks();
   if (ok) emit requestData(); } // ui->getButton->setEnabled(true);}

#ifdef OBSOLETE
bool Dialog::readUsbSettings(){
   QSettings settings;
   if (settings.childGroups().contains(QLatin1String("usb"))) {
      bool ok;
      settings.beginGroup(QLatin1String("usb"));
      vendor = settings.value(vendorId).toInt(&ok);
      if (!ok) {
         logMessage( LogLevel::WARNING, tr("Vendor Id not a number: %1").arg(settings.value(vendorId).toString()));
         return false;}
      product = settings.value(productId).toInt(&ok);
      if (!ok) {
         logMessage( LogLevel::WARNING, tr("Product Id not a number: %1").arg(settings.value(productId).toString()));
         return false;}
      logMessage( LogLevel::WARNING, tr("Requested vendorId:ProductId: %1:%2").arg(vendor, 4, 16, QLatin1Char('0')).arg(product, 4, 16, QLatin1Char('0')));
      configuration = settings.value(configurationNumber).toInt(&ok);
      if (!ok) {
         logMessage( LogLevel::WARNING, tr("Configuration not a number: %1").arg(settings.value(configurationNumber).toString()));
         return false;}
      interface = settings.value(interfaceNumber).toInt(&ok);
      if (!ok) {
         logMessage( LogLevel::WARNING, tr("Interface not a number: %1").arg(settings.value(interfaceNumber).toString()));
         return false;}
      altsetting = settings.value(altsettingNumber).toInt(&ok);
      if (!ok) {
         logMessage( LogLevel::WARNING, tr("Alternatative setting not a number: %1").arg(settings.value(altsettingNumber).toString()));
         return false;}
      logMessage( LogLevel::WARNING, tr("Requested to us configuration:interface:altsetting %1:%2:%3").arg(configuration).arg(interface).arg(altsetting));
      bus = settings.value(busNumber).toInt(&ok);
      if (!ok || bus < 0) logMessage( LogLevel::WARNING, tr("Illegal stored bus number (ignored): %1").arg(settings.value(productId).toString()));
      device = settings.value(deviceNumber).toInt(&ok);
      if (!ok || device < 0) logMessage( LogLevel::WARNING, tr("Illegal stored device number not a number (ignored): %1").arg(settings.value(productId).toString()));
      epIn.address = settings.value(endpointIn).toInt(&ok);
      if (ok) logMessage( LogLevel::WARNING, tr("Endpoint IN (reading) set to %1").arg(epIn.address));
      else {
         epIn.address = -1;
         logMessage( LogLevel::WARNING, tr("Endpoint IN (reading) is invalid (unlikely to be correct)"));}
      epOut.address = settings.value(endpointOut).toInt(&ok);
      if (ok) logMessage( LogLevel::WARNING, tr("Endpoint OUT (writing) set to %1").arg(epOut.address));
      else {
         epOut.address = -1;
         logMessage( LogLevel::WARNING, tr("Endpoint OUT (writing) is invalid (could be correct)"));}
      settings.endGroup();}
   else {
      logMessage( LogLevel::WARNING, tr("Use hard coded usb settings as retrieved from 'lsusb -v'"));
      vendor = 2707;
      product = 267;
      configuration = 1;
      interface = 0;
      altsetting = 0;
      bus = -1;
      device = -5;
      epIn.address =2;
      epOut.address = -1;}
   return true;}
#endif

libusb_device_handle *Dialog::openUsbDevice(){
   int result = libusb_init(NULL);
   Q_ASSERT_X(result == 0, "libusb_init", QString::number(result).toLatin1());
   libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_WARNING);
   libusb_device **device_list = nullptr;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   logMessage( LogLevel::WARNING, tr("Found %L1 usb devices").arg(device_count));
   libusb_device *dev;
   QList<libusb_device*> dev_list;
   libusb_device_handle* dev_handle1 = nullptr;
   int i(0);
   while ((dev = device_list[i++]) != NULL) {
      libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      if (desc.idVendor == vendor && desc.idProduct == product) {
         logMessage( LogLevel::WARNING, tr("Device with Vendor Id %1 and Product Id %2 found").arg(desc.idVendor, 4, 16, QLatin1Char('0'))
                                                                                          .arg(desc.idProduct, 4, 16, QLatin1Char('0')));
         dev_list.append(dev);}}
   if (dev_list.isEmpty()) {
      logMessage( LogLevel::WARNING, tr("No device with vendorId %1 and productId %2 found")
                           .arg(vendor, 4, 16, QLatin1Char('0')).arg(product, 4, 16, QLatin1Char('0')));
      logMessage( LogLevel::WARNING, tr("Close application, plug-in device and set correct parameters by running PenCapture_config"));}
   else {
      dev = nullptr;
      int bus1;
      int device1;
      foreach (libusb_device* dev1, dev_list) {
         bus1 = libusb_get_bus_number(dev1);
         device1 = libusb_get_device_address(dev1);
         if (bus == bus1 && device == device1) dev = dev1;}
      if (dev == nullptr) {
         logMessage( LogLevel::WARNING, tr("Device not at usb bus %1:%2").arg(bus).arg(device));
         dev = dev_list.at(0);}
      bus = libusb_get_bus_number(dev);
      device = libusb_get_device_address(dev);
      logMessage( LogLevel::WARNING, tr("Using device at usb bus %1:%2").arg(bus).arg(device));
      libusb_device_descriptor device_desc;
      result = libusb_get_device_descriptor(dev, &device_desc);
      Q_ASSERT_X(result == LIBUSB_SUCCESS, "libusb_get_device_descriptor", libusb_error_name(result));
      libusb_config_descriptor* config_desc;
      result = libusb_get_config_descriptor_by_value(dev, configuration, &config_desc);
      if (result != LIBUSB_SUCCESS) logMessage( LogLevel::WARNING, tr("Cannot get config descriptor for configuration %1: %2").arg(configuration)
                                                                 .arg(QLatin1String(libusb_error_name(result))));
      else {
         for (uint i(0); i < config_desc->bNumInterfaces; ++i) {
            const libusb_interface* interfacePtr = &config_desc->interface[i];
            for (int j(0); j < interfacePtr->num_altsetting; ++j) {
               libusb_interface_descriptor interface_desc = interfacePtr->altsetting[j];
               if (interface_desc.bAlternateSetting == altsetting && interface_desc.bInterfaceNumber == interface) {
                  logMessage( LogLevel::WARNING, tr("Retrieve details for the %1 endpoints").arg(interface_desc.bNumEndpoints));
                  for (uint k(0); k < interface_desc.bNumEndpoints; ++k) {
                     const libusb_endpoint_descriptor* endpoint_desc = &interface_desc.endpoint[k];
                     if ((endpoint_desc->bEndpointAddress & 0x07) == epIn.address || (endpoint_desc->bEndpointAddress & 0x07) == epOut.address) {
                        endpoint* epPtr = (endpoint_desc->bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN ? &epIn : &epOut;
                        Q_ASSERT(endpoint_desc->bDescriptorType == LIBUSB_DT_ENDPOINT);
                        epPtr->maxPacketSize = endpoint_desc->wMaxPacketSize;
                        epPtr->transferType = static_cast<libusb_transfer_type>(endpoint_desc->bmAttributes & 0x03);
                        logMessage( LogLevel::WARNING, tr("Details of endpoint %1 (%2): type %3, max packet size %4")
                                                     .arg(epPtr->address).arg(QLatin1String(epPtr == &epIn ? "IN" : "OUT"))
                                                     .arg(transferTypeFromEnum(endpoint_desc->bmAttributes & 0x03)).arg(epPtr->maxPacketSize));}
                  }
               }
            }
         }
         libusb_free_config_descriptor(config_desc);
         result = libusb_open(dev, &dev_handle1);
         if (result != LIBUSB_SUCCESS) {
            logMessage( LogLevel::WARNING, tr("Failed to open USB device %1 / %2 at usb %3:%4")
                                 .arg(vendor, 4, 16, QLatin1Char('0')).arg(product, 4, 16, QLatin1Char('0')).arg(bus).arg(device));
            logMessage( LogLevel::WARNING, tr("You may need to run this application as administrator; or write udev rule for a permanent solution"));}
         else {
            Q_ASSERT(dev_handle1 != nullptr);
            logMessage( LogLevel::WARNING, tr("USB device opened (i.e., device handle obtained)"));
            unsigned char descstring[255];
            result = libusb_get_string_descriptor_ascii(dev_handle1, device_desc.iManufacturer, descstring, sizeof(descstring));
            if (result > 0) logMessage( LogLevel::WARNING, tr("Manufacturer: %1").arg(QLatin1String((char*)descstring)));
            result = libusb_get_string_descriptor_ascii(dev_handle1, device_desc.iProduct, descstring, sizeof(descstring));
            if (result > 0) logMessage( LogLevel::WARNING, tr("Product: %1").arg(QLatin1String((char*)descstring)));
            result = libusb_get_string_descriptor_ascii(dev_handle1, device_desc.iSerialNumber, descstring, sizeof(descstring));
            if (result > 0) logMessage( LogLevel::WARNING, tr("Serial Number: %1").arg(QLatin1String((char*)descstring)));}}}
   libusb_free_device_list(device_list, 1);   // free devices, keep only handle
   return dev_handle1;}

bool Dialog::prepareUsbDevice() {
   int returnvalue(true);
   if (returnvalue == true) { // always true; used for consistency
      int active_config;
      int result = libusb_get_configuration(dev_handle, &active_config);
      if (result != LIBUSB_SUCCESS) {
         logMessage( LogLevel::WARNING, tr("Cannot retrieve active configuration id: %1").arg(QLatin1String(libusb_error_name(result))));
         returnvalue = false;}
      else {
         if (active_config == configuration) logMessage( LogLevel::WARNING, tr("Configuration %1 already set").arg(configuration));
         else {
            /* somehow get a libusb_config_desc; possibly requires to get libusb_device (from device handle)
            for (uint8_t i(0); i < config_desc->bNumInterfaces; ++i)
                    libusb_detach_kernel_driver(dev_handle, config_desc->interface->altsetting[0].bInterfaceNumber)*/
            result = libusb_set_configuration(dev_handle, configuration);
            if (result != LIBUSB_SUCCESS) {
                logMessage( LogLevel::WARNING, tr("Cannot set configuration %1: %2").arg(configuration).arg(QLatin1String(libusb_error_name(result))));
                returnvalue = false;}
            else {
               logMessage( LogLevel::WARNING, tr("Configuration set"));
               libusb_config_descriptor config_desc;
               unsigned char descstring[255];
               result = libusb_get_string_descriptor_ascii(dev_handle, config_desc.iConfiguration, descstring, sizeof(descstring));
               if (result > 0) logMessage( LogLevel::WARNING, tr("Configuration: %1").arg(QLatin1String((char*)descstring)));}}}}
   if (returnvalue == true) {
      int result = libusb_kernel_driver_active(dev_handle, interface);
      if (result < 0) logMessage( LogLevel::WARNING, tr("Cannot determine whether kernel driver is attached: %1")
                                                   .arg(QLatin1String(libusb_error_name(result))));
      else if (result == 0) logMessage( LogLevel::WARNING, tr("Great, no kernel driver is attached"));
      else {
         Q_ASSERT(result == 1);
         logMessage( LogLevel::WARNING, tr("First nedd to detach kernel driver"));
         result = libusb_detach_kernel_driver(dev_handle, interface);
         if (result == LIBUSB_SUCCESS) logMessage( LogLevel::WARNING, tr("Kernel driver detached"));
         else logMessage( LogLevel::WARNING, tr("Failed to detach kernel driver").arg(QLatin1String(libusb_error_name(result))));}
      result = libusb_claim_interface(dev_handle, interface);
      if (result == LIBUSB_SUCCESS) logMessage( LogLevel::WARNING, tr("Interface %1 claimed").arg(interface));
      else {
         logMessage( LogLevel::WARNING, tr("Failed to claim interface %1: %2").arg(interface).arg(QLatin1String(libusb_error_name(result))));
         returnvalue = false;}}
   if (returnvalue == true) {
      int result = libusb_set_interface_alt_setting(dev_handle, interface, altsetting);
      if (result == LIBUSB_SUCCESS) {
         logMessage( LogLevel::WARNING, tr("Alternative setting %1 set").arg(altsetting));}
      else {
         logMessage( LogLevel::WARNING, tr("Failed to set alternative setting %1: %2").arg(altsetting).arg(QLatin1String(libusb_error_name(result))));
         returnvalue = false;}}
   if (returnvalue == true) {    // prepare libusb_transfer struct
      // LIBUSB_ENDPOINT_IN = In: device-to-host
      // LIBUSB_ENDPOINT_OUT = Out: host-to-device
      transfer = libusb_alloc_transfer(0);
      if (transfer == nullptr) {
         logMessage( LogLevel::WARNING, tr("Cannot allocate transfer struct"));
         returnvalue = false;}
      else {
         transfer->dev_handle = dev_handle;
         transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER; // not '| LIBUSB_TRANSFER_FREE_TRANSFER', to allow reuse
         transfer->endpoint = epIn.address | LIBUSB_ENDPOINT_IN;
         transfer->type = epIn.transferType;
         transfer->timeout = 1000; // ms
         transfer->length = 2048 / epIn.maxPacketSize * epIn.maxPacketSize; // a multiple of epIn.maxPacketSize to avoid oversize
         transfer->callback = &Dialog::staticTransferCbFn;
         transfer->user_data = this; }}
   return returnvalue;}

bool Dialog::prepareCallbacks() {
   int result = libusb_pollfds_handle_timeouts(NULL);
   if (result == 0) {
      logMessage( LogLevel::WARNING, tr("libusb_pollfds_handle_timeouts indicates that app would need to handle timeouts; not implemented; exiting"));
      return false;}
   logMessage( LogLevel::WARNING, tr("Great, kernel supports simplified polling"));
   const libusb_pollfd** pollfds = libusb_get_pollfds(NULL);
   Q_ASSERT (pollfds != NULL);
   for (uint i(0); pollfds[i] != NULL; ++i) {
      const libusb_pollfd* pollfd = pollfds[i];
      if (pollfd->events & POLLIN) {
         QSocketNotifier* s = new QSocketNotifier(pollfd->fd, QSocketNotifier::Read, this);
         connect(s, &QSocketNotifier::activated, this, &Dialog::handleLibusbEvent);
         s->setEnabled(true);
         socketNotifiers.append(s);}
      if (pollfd->events & POLLOUT) {
         QSocketNotifier* s = new QSocketNotifier(pollfd->fd, QSocketNotifier::Write, this);
         connect(s, &QSocketNotifier::activated, this, &Dialog::handleLibusbEvent);
         s->setEnabled(true);
         socketNotifiers.append(s);}}
   QString fdString;
   foreach (QSocketNotifier* s, socketNotifiers) fdString += QString(QLatin1String("%1 (%2), ")).arg(s->socket())
         .arg(s->type() == QSocketNotifier::Write ? tr("Write") : (s->type() == QSocketNotifier::Read ? tr("Read") : tr("Exception")));
   logMessage( LogLevel::WARNING, tr("Initial fd to poll: %1").arg(fdString.left(fdString.length() - 2)));
   libusb_set_pollfd_notifiers(NULL, &Dialog::pollfdAddedCb, &Dialog::pollfdRemovedCb, this);
   logMessage( LogLevel::WARNING, tr("Callbacks set to get updates on fd to poll"));
   return true;}

void Dialog::pollfdAddedCb(int fd, short events){
   logMessage( LogLevel::WARNING, tr("Callback request to add fd %1 for polling of events %2").arg(fd).arg(events));
   if (events & POLLIN) {
      QSocketNotifier* s = new QSocketNotifier(fd, QSocketNotifier::Read, this);
      connect(s, &QSocketNotifier::activated, this, &Dialog::handleLibusbEvent);
      s->setEnabled(true);
      socketNotifiers.append(s);}
   if (events & POLLOUT) {
      QSocketNotifier* s = new QSocketNotifier(fd, QSocketNotifier::Write, this);
      connect(s, &QSocketNotifier::activated, this, &Dialog::handleLibusbEvent);
      s->setEnabled(true);
      socketNotifiers.append(s);}}

void Dialog::pollfdRemovedCb(int fd){
   logMessage( LogLevel::WARNING, tr("Callback request to remove fd %1 from polling").arg(fd));
   foreach (QSocketNotifier* s, socketNotifiers) if (s->socket() == fd) {
      socketNotifiers.removeOne(s);
      delete s;}}

void Dialog::handleLibusbEvent(){
   // logMessage( LogLevel::WARNING, tr("handleLibusbEvent entered"));
   timeval tv_zero;
   tv_zero.tv_sec = 0;
   tv_zero.tv_usec = 0;
   int result = libusb_handle_events_timeout_completed(NULL, &tv_zero, NULL);
   // logMessage( LogLevel::WARNING, tr("libusb_handle_events_timeout_completed returned %1").arg(QLatin1String(libusb_error_name(result))));
   Q_ASSERT( result == LIBUSB_SUCCESS );}

bool Dialog::receiveData(){
   unsigned char* inputBuffer = new unsigned char[transfer->length];  // auto-freed by libusb
   if (!inputBuffer) {
      logMessage( LogLevel::WARNING, tr("Allocation of inputbuffer (%1 bytes) failed").arg(transfer->length));
      return false; }
   transfer->buffer = inputBuffer;
   logMessage( LogLevel::DEBUG, tr("Ready to submit data request to endpoint %1, type %2").arg(transfer->endpoint)
                                                                                          .arg(this->transferTypeFromEnum(transfer->type)));
   logMessage( LogLevel::DEBUG, tr("handle = %1").arg((unsigned long)transfer->dev_handle));
   logMessage( LogLevel::DEBUG, tr("flags = %1").arg(transfer->flags));
   logMessage( LogLevel::DEBUG, tr("endpoint = %1").arg(transfer->endpoint));
   logMessage( LogLevel::DEBUG, tr("type = %1").arg(transfer->type));
   logMessage( LogLevel::DEBUG, tr("status = %1").arg(transfer->status));
   logMessage( LogLevel::DEBUG, tr("length = %1").arg(transfer->length));
   logMessage( LogLevel::DEBUG, tr("actual_length = %1").arg(transfer->actual_length));
   logMessage( LogLevel::DEBUG, tr("callback = %1").arg((unsigned long)transfer->callback));
   logMessage( LogLevel::DEBUG, tr("num_iso_packets = %1").arg(transfer->num_iso_packets));
   logMessage( LogLevel::DEBUG, tr("iso_packet_desc = %1").arg((unsigned long)transfer->iso_packet_desc));
   int result = libusb_submit_transfer(transfer);
   if (result == LIBUSB_SUCCESS) {
      logMessage( LogLevel::INFO, tr("Request for data submitted"));
      return true;}
   else {
      logMessage( LogLevel::ERR, tr("Request for data failed: %1").arg(QLatin1String(libusb_error_name(result))));
      return false;}}

void Dialog::staticTransferCbFn(libusb_transfer *transfer) {
   ((Dialog*)transfer->user_data)->libusbTransferCbFn(transfer);}

void Dialog::libusbTransferCbFn(libusb_transfer *transfer) {
   /* possible outcomes:
    * The transfer completes (i.e. some data was transferred)
    * The transfer has a timeout and the timeout expires before all data is transferred
    * The transfer fails due to an error
    * The transfer is cancelled   */
   bool bulk_completed(true);
   /* logMessage( LogLevel::WARNING, tr("USB transfer: callback entered: %1")
                                .arg(QLatin1String((transfer->type & 0x03) != LIBUSB_TRANSFER_TYPE_BULK ? "not bulk" : "bulk transfer"))); */
   switch (transfer->status) {
      case LIBUSB_TRANSFER_COMPLETED: // Transfer completed without error. Note that this does not indicate that the entire amount of requested data was transferred.
         // logMessage( LogLevel::WARNING, tr("USB transfer without error; actual length is %1").arg(transfer->actual_length));
         if (transfer->actual_length > 0) {
            QByteArray newData((const char*)transfer->buffer, transfer->actual_length);
            logMessage( LogLevel::WARNING, tr("New data received; %1 bytes (total %2 bytes space)").arg(newData.length()).arg(transfer->length));
#ifdef QT_GUI_LIB
            ui->capturedText->append((QLatin1String(newData.toHex())));
#endif
            }
         // last part of bulk transfer is either incompletely filled or even empty (if the 'real' last transfer happened to exactly fill the buffer)
         if (transfer->actual_length == transfer->length) bulk_completed = false;
         else {
#ifdef QT_GUI_LIB
            ui->capturedText->setTextColor(ui->capturedText->textColor() == Qt::blue ? Qt::black : Qt::blue);
#endif
//qDebug() << transfer->buffer[0] << transfer->buffer[1] << transfer->buffer[2] << transfer->buffer[3] << transfer->buffer[4] << transfer->buffer[5];
            int result(0);
            result = cpen_scandata(transfer->buffer, transfer->actual_length);
//qDebug() << "scandata with" << transfer->actual_length << "returned" << result;
            if ( result < 0 ) {    // status code
               // logMessage( LogLevel::WARNING, tr("No output from cpenlib; return code %1").arg(result));
               result = -result;
               processCpenStatusCode( result );}
            logMessage( LogLevel::INFO, tr("Forwarded frame to libcpen_scandata; result = %1").arg(result));}
         break;
      case LIBUSB_TRANSFER_ERROR: // Transfer failed.
         logMessage( LogLevel::WARNING, tr("USB transfer error"));
         break;
      case LIBUSB_TRANSFER_TIMED_OUT: // Transfer timed out.
         logMessage( LogLevel::INFO, tr("USB transfer timed out"));
         break;
      case LIBUSB_TRANSFER_CANCELLED: // Transfer was cancelled.
         logMessage( LogLevel::WARNING, tr("USB transfer cancelled"));
         break;
      case LIBUSB_TRANSFER_STALL: // For bulk/interrupt endpoints: halt condition detected (endpoint stalled). For control endpoints: control request not supported.
         logMessage( LogLevel::ERR, tr("USB transfer stalled"));
         break;
      case LIBUSB_TRANSFER_NO_DEVICE: // Device was disconnected.
         logMessage( LogLevel::CRIT, tr("USB transfer : no device (disconnected?)"));
         break;
      case LIBUSB_TRANSFER_OVERFLOW: // Device sent more data than requested.
         logMessage( LogLevel::ERR, tr("USB transfer: Device sent more data than requested (actual length = %1)").arg(transfer->actual_length));
         break;
      default:
         Q_ASSERT_X(false, "unexpected transfer status received", QString::number(transfer->status).toLatin1());}
#ifdef QT_GUI_LIB
   if (pauseUSB) {
      ui->usbButtonbox->button(QDialogButtonBox::Yes)->setEnabled(true);
      ui->mainButtonbox->button(QDialogButtonBox::Close)->setEnabled(true);
   }
   else
#endif
      emit requestData(); }

QLatin1String Dialog::transferTypeFromEnum(quint8 transferType) const{
   QLatin1String result("");
   switch (transferType & 0x03) {
      case LIBUSB_TRANSFER_TYPE_CONTROL:
         result = QLatin1String("CONTROL");
         break;
      case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:
         result = QLatin1String("ISOCHRONOUS");
         break;
      case LIBUSB_TRANSFER_TYPE_BULK:
         result = QLatin1String("BULK");
         break;
      case LIBUSB_TRANSFER_TYPE_INTERRUPT:
         result = QLatin1String("INTERRUPT");
         break;
      default:
      Q_ASSERT(false);}
   return result;}

void Dialog::signalHandler(int signalnumber) {
   Q_UNUSED( signalnumber );
   char a = 1;
   ::write(Dialog::signalPipe[0], &a, sizeof(a)); }

void Dialog::processCpenEvent() {
   char a;
   ::read( Dialog::signalPipe[1], &a, sizeof(a) );
   static int eventcount(0);
   logMessage( LogLevel::INFO, tr("Event #%1 received from cpenlib through socket").arg(++eventcount));
   unsigned char* data;
   unsigned int size;
   int result = cpen_retrieve_output( &data, &size );
   if ( result == 0 )
      logMessage( LogLevel::WARNING, tr("No output from cpenlib; return code %1").arg(result));
   else if ( result < 0 ) { // status code
      result = -result;
      processCpenStatusCode( result );}
   else {                  // image received
      logMessage( LogLevel::INFO,  tr("%1 bytes of data received; type (flag): %2").arg(size).arg(result) );
#ifdef QT_GUI_LIB
      QPixmap pixmap;
      pixmap.loadFromData( data, size );
      imageList.append(pixmap);
      free ( data );
      if ( currentImage < 0 ) nextImage() ;
      else ui->playerButtonbox->button(QDialogButtonBox::Yes)->setEnabled( true );
#else
      std::cout.write( (char*)data, size );
#endif
      }}

#ifdef QT_GUI_LIB    // interaction with user through dialog
void Dialog::pauseContinueUSB(QAbstractButton* button){
   switch (this->ui->usbButtonbox->standardButton(button)) {
      case QDialogButtonBox::Yes:
         pauseUSB = false;
         ui->mainButtonbox->button(QDialogButtonBox::Close)->setEnabled(pauseUSB);
         ui->usbButtonbox->button(QDialogButtonBox::Yes)->setEnabled(pauseUSB);
         emit requestData();
         break;
      case QDialogButtonBox::No:
         pauseUSB = true;
         // only enable ui->usbButtonbox->button(QDialogButtonBox::Yes) once usb is actually paused
         break;
      default:
         Q_ASSERT_X(false, "ui->usbButtonbox has invalid button", QByteArray::number(this->ui->usbButtonbox->standardButton(button)));}
   ui->usbButtonbox->button(QDialogButtonBox::No)->setEnabled(!pauseUSB);}

void Dialog::closeEvent(QCloseEvent *event) {
   Q_UNUSED(event)
   //   cpen_free();
}

void Dialog::nextImage(){
   ++currentImage;
   Q_ASSERT( currentImage >= 0 );
   Q_ASSERT( currentImage < imageList.count() );
   ui->playerButtonbox->button(QDialogButtonBox::No)->setEnabled( currentImage != 0 );
   ui->playerButtonbox->button(QDialogButtonBox::Yes)->setEnabled( currentImage != imageList.count() - 1 );
   ui->imageDisplay->setPixmap(imageList.at(currentImage).scaled( ui->imageDisplay->size(), Qt::KeepAspectRatio )); }

void Dialog::previousImage(){
   --currentImage;
   Q_ASSERT( currentImage >= 0 );
   ui->playerButtonbox->button(QDialogButtonBox::No)->setEnabled( currentImage != 0 );
   ui->playerButtonbox->button(QDialogButtonBox::Yes)->setEnabled( currentImage != imageList.count() - 1 );
   ui->imageDisplay->setPixmap(imageList.at(currentImage).scaled( ui->imageDisplay->size(), Qt::KeepAspectRatio )); }

void Dialog::setMemoryOutput() {
   cpen_set_memoryoutput( sumOfFlags(ui->memoryoutputBox) ); }

void Dialog::setFileOutput() {
   cpen_set_fileoutput( sumOfFlags(ui->fileoutputBox) ); }

void Dialog::setDirection_r2l(){
   cpen_set_scanmode(cpen_1D_r2l);}

void Dialog::setDirection_l2r(){
   cpen_set_scanmode(cpen_1D_l2r);}

void Dialog::setDirection_2D(){
   cpen_set_scanmode(cpen_2D);}

int Dialog::sumOfFlags(QGroupBox *groupbox) const {
   int sum(0);
   foreach ( QObject* widget, groupbox->children() ) if ( widget->isWidgetType() ) {
      QCheckBox* checkbox = qobject_cast< QCheckBox* >(widget) ;
      Q_ASSERT_X(checkbox, "not a checkbox", widget->objectName().toLatin1());
      if ( checkbox->isChecked() ) {
         QString label(checkbox->text());
         if (label == QLatin1String("character")) sum += cpen_char;
         else if (label == QLatin1String("raw")) sum += cpen_raw;
         else if (label == QLatin1String("word")) sum += cpen_word;
         else if (label == QLatin1String("line")) sum += cpen_line;
         else if (label == QLatin1String("displacement")) sum += cpen_displacement;
         else if (label == QLatin1String("resized")) sum += cpen_resized;
         else Q_ASSERT_X(false, "unknown label text of checkbox", label.toLatin1());}}
   return sum;}
#endif
