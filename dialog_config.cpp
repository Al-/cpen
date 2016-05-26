#include "dialog_config.h"
#include "ui_dialog_config.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <qdebug.h>
#include "global.h"
// requires: sudo apt-get install libusb-1.0-0-dev
// with "LIBS += /usr/lib/i386-linux-gnu/libusb-1.0.so" in the .pro file
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


Dialog::Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog) {
   ui->setupUi(this);
   int result = libusb_init(NULL);
   Q_ASSERT_X(result == 0, "libusb_init", QString::number(result).toLatin1());
   libusb_set_debug(NULL, LIBUSB_LOG_LEVEL_WARNING);
   //void (QSpinBox:: *QSpinBox_valueChanged_int)(int) = &QSpinBox::valueChanged;
   //connect(ui->vendorSpinBox, QSpinBox_valueChanged_int, this, &Dialog::slotDeviceChanged);
   //connect(ui->deviceSpinBox, QSpinBox_valueChanged_int, this, &Dialog::slotDeviceChanged);
   updateVendorList();
   QMetaObject::Connection connection = connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &Dialog::processButton);
   Q_ASSERT(connection);
   interfaceClassMap.insert((int)LIBUSB_CLASS_PER_INTERFACE, QLatin1String("In the context of a device descriptor, this bDeviceClass value indicates that each interface specifies its own class information and all interfaces operate independently."));
   interfaceClassMap.insert((int)LIBUSB_CLASS_AUDIO, QLatin1String("Audio class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_COMM, QLatin1String("Communications class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_HID, QLatin1String("Human Interface Device class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_PHYSICAL, QLatin1String("Physical"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_PRINTER, QLatin1String("Printer class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_PTP, QLatin1String("Image class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_MASS_STORAGE, QLatin1String("Mass storage class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_HUB, QLatin1String("Hub class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_DATA, QLatin1String("Data class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_SMART_CARD, QLatin1String("Smart Card"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_CONTENT_SECURITY, QLatin1String("Content Security"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_VIDEO, QLatin1String("Video"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_PERSONAL_HEALTHCARE, QLatin1String("Personal Healthcare"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_DIAGNOSTIC_DEVICE, QLatin1String("Diagnostic Device"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_WIRELESS, QLatin1String("Wireless class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_APPLICATION, QLatin1String("Application class"));
   interfaceClassMap.insert((int)LIBUSB_CLASS_VENDOR_SPEC, QLatin1String("Class is vendor-specific"));
}

Dialog::~Dialog() {
   delete ui;
   libusb_exit(NULL);}

void Dialog::updateVendorList() {
   disconnect(vendorConnection);
   ui->vendorId->clear();
   libusb_device **device_list;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   qDebug() << "libusb_get_device_list: count" << device_count;
   Q_ASSERT_X(device_count >= 0, "libusb_get_device_list", QString::number(device_count).toLatin1());

   libusb_device *dev;
   int i(0);
   while ((dev = device_list[i++]) != NULL) {
      struct libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      QString vendor = QString(QLatin1String("%1")).arg(desc.idVendor, 4, 16, QLatin1Char('0'));
      qDebug() << vendor << QString(QLatin1String("%1")).arg(desc.idProduct, 4, 16, QLatin1Char('0')) << libusb_get_bus_number(dev) << libusb_get_device_address(dev);
      /*libusb_device_handle *dev_handle;
      int r = libusb_open(dev, &dev_handle);
      // Q_ASSERT_X(r == 0, "libusb_open", QString::number(r).toLatin1());
      if (r = LIBUSB_SUCCESS) {
         result = libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer, NULL, 0);
         qDebug() << "string length" << result;
         // then prepare array to get string; retrieve string; add to vendor Id
         libusb_close(dev_handle);} */
      if (ui->vendorId->findText(vendor) == -1) ui->vendorId->addItem(vendor);
      //qDebug() << << desc.idProduct << libusb_get_bus_number(dev) << libusb_get_device_address(dev);
   }
   libusb_free_device_list(device_list, 1);
   ui->vendorId->setEnabled(ui->vendorId->count() > 1);
   updateProductList();
   void (QComboBox:: *QComboBox_currentIndexChanged_int)(int) = &QComboBox::currentIndexChanged;
   vendorConnection = connect(ui->vendorId, QComboBox_currentIndexChanged_int, this, &Dialog::updateProductList);
   Q_ASSERT(vendorConnection);}

void Dialog::updateProductList() {
   disconnect(productConnection);
   ui->productId->clear();
   libusb_device **device_list;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   //qDebug() << "libusb_get_device_list: count" << device_count;
   Q_ASSERT_X(device_count >= 0, "libusb_get_device_list", QString::number(device_count).toLatin1());
   libusb_device *dev;
   int i(0);
   QString vendortext(ui->vendorId->currentText());
   qDebug() << "get product IDs for vendor" << vendortext;
   while ((dev = device_list[i++]) != NULL) {
      struct libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      QString vendor1 = QString(QLatin1String("%1")).arg(desc.idVendor, 4, 16, QLatin1Char('0'));
      if (vendortext == vendor1) {
         QString product = QString(QLatin1String("%1")).arg(desc.idProduct, 4, 16, QLatin1Char('0'));
         /*libusb_device_handle *dev_handle
         int r = libusb_open(dev, &dev_handle);
         // Q_ASSERT_X(r == 0, "libusb_open", QString::number(r).toLatin1());
         if (r = LIBUSB_SUCCESS) {
            result = libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct, NULL, 0);
            qDebug() << "string length" << result;
            // then prepare array to get string; retrieve string; add to product Id
            libusb_close(dev_handle);} */
         if (ui->productId->findText(product) == -1) ui->productId->addItem(product);}}
   libusb_free_device_list(device_list, 1);
   ui->productId->setEnabled(ui->productId->count() > 1);
   updateBusList();
   void (QComboBox:: *QComboBox_currentIndexChanged_int)(int) = &QComboBox::currentIndexChanged;
   productConnection = connect(ui->productId, QComboBox_currentIndexChanged_int, this, &Dialog::updateBusList);
   Q_ASSERT(productConnection);}

void Dialog::updateBusList() {
   disconnect(busConnection);
   ui->busNumber->clear();
   libusb_device **device_list;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   //qDebug() << "libusb_get_device_list: count" << device_count;
   Q_ASSERT_X(device_count >= 0, "libusb_get_device_list", QString::number(device_count).toLatin1());
   libusb_device *dev;
   int i(0);
   QString vendortext(ui->vendorId->currentText());
   QString producttext(ui->productId->currentText());
   qDebug() << "get bus and ports for vendor" << vendortext << "and product" << producttext;
   while ((dev = device_list[i++]) != NULL) {
      struct libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      QString vendor1 = QString(QLatin1String("%1")).arg(desc.idVendor, 4, 16, QLatin1Char('0'));
      QString product1 = QString(QLatin1String("%1")).arg(desc.idProduct, 4, 16, QLatin1Char('0'));
      if (vendortext == vendor1 && producttext == product1) {
         QString bus(QString::number(libusb_get_bus_number(dev)));
         if (ui->busNumber->findText(bus) == -1) ui->busNumber->addItem(bus);}}
   libusb_free_device_list(device_list, 1);
   ui->busNumber->setEnabled(ui->busNumber->count() > 1);
   updateDeviceList();
   void (QComboBox:: *QComboBox_currentIndexChanged_int)(int) = &QComboBox::currentIndexChanged;
   busConnection = connect(ui->busNumber, QComboBox_currentIndexChanged_int, this, &Dialog::updateDeviceList);
   Q_ASSERT(busConnection);}

void Dialog::updateDeviceList() {
   disconnect(deviceConnection);
   ui->deviceNumber->clear();
   libusb_device **device_list;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   //qDebug() << "libusb_get_device_list: count" << device_count;
   Q_ASSERT_X(device_count >= 0, "libusb_get_device_list", QString::number(device_count).toLatin1());
   libusb_device *dev;
   int i(0);
   QString vendortext(ui->vendorId->currentText());
   QString producttext(ui->productId->currentText());
   QString bustext(ui->busNumber->currentText());
   qDebug() << "get devices for vendor" << vendortext << "and product" << producttext << "and bus" << bustext;
   while ((dev = device_list[i++]) != NULL) {
      struct libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      QString vendor1 = QString(QLatin1String("%1")).arg(desc.idVendor, 4, 16, QLatin1Char('0'));
      QString product1 = QString(QLatin1String("%1")).arg(desc.idProduct, 4, 16, QLatin1Char('0'));
      QString bus1 = QString::number(libusb_get_bus_number(dev));
      if (vendortext == vendor1 && producttext == product1 && bustext == bus1) {
         QString device(QString::number(libusb_get_device_address(dev)));
         if (ui->deviceNumber->findText(device) == -1) ui->deviceNumber->addItem(device);}}
   libusb_free_device_list(device_list, 1);
   ui->deviceNumber->setEnabled(ui->deviceNumber->count() > 1);
   updateConfigurationList();
   void (QComboBox:: *QComboBox_currentIndexChanged_int)(int) = &QComboBox::currentIndexChanged;
   deviceConnection = connect(ui->deviceNumber, QComboBox_currentIndexChanged_int, this, &Dialog::updateConfigurationList);
   Q_ASSERT(deviceConnection);}

void Dialog::updateConfigurationList() {
   disconnect(configurationConnection);
   ui->configurationNumber->clear();
   libusb_device **device_list;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   Q_ASSERT_X(device_count >= 0, "libusb_get_device_list", QString::number(device_count).toLatin1());
   libusb_device *dev;
   int i(0);
   QString vendortext(ui->vendorId->currentText());
   QString producttext(ui->productId->currentText());
   QString bustext(ui->busNumber->currentText());
   QString devicetext(ui->deviceNumber->currentText());
   qDebug() << "get configurations for vendor" << vendortext << "and product" << producttext << "and bus" << bustext << "and device" << devicetext;
   while ((dev = device_list[i++]) != NULL) {
      struct libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      QString vendor1 = QString(QLatin1String("%1")).arg(desc.idVendor, 4, 16, QLatin1Char('0'));
      QString product1 = QString(QLatin1String("%1")).arg(desc.idProduct, 4, 16, QLatin1Char('0'));
      QString bus1 = QString::number(libusb_get_bus_number(dev));
      QString device1 = QString::number(libusb_get_device_address(dev));
      if (vendortext == vendor1 && producttext == product1 && bustext == bus1 && devicetext == device1) {
         Q_ASSERT(ui->configurationNumber->count() == 0); // as we found the first and only bus/address combination
         for (uint8_t n(0); n < desc.bNumConfigurations; ++n) {
            libusb_config_descriptor* config_descriptor;
            result = libusb_get_config_descriptor(dev, n, &config_descriptor);
            Q_ASSERT_X(result == LIBUSB_SUCCESS, "libusb_get_config_descriptor", QString::number(result).toLatin1());
            ui->configurationNumber->addItem(QString::number(config_descriptor->bConfigurationValue));
            libusb_free_config_descriptor(config_descriptor);}}}
   libusb_free_device_list(device_list, 1);
   ui->configurationNumber->setEnabled(ui->configurationNumber->count() > 1);
   updateInterfaceList();
   void (QComboBox:: *QComboBox_currentIndexChanged_int)(int) = &QComboBox::currentIndexChanged;
   configurationConnection = connect(ui->configurationNumber, QComboBox_currentIndexChanged_int, this, &Dialog::updateInterfaceList);
   Q_ASSERT(configurationConnection);}

void Dialog::updateInterfaceList() {
   disconnect(interfaceConnection);
   ui->interfaceNumber->clear();
   libusb_device **device_list;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   Q_ASSERT_X(device_count >= 0, "libusb_get_device_list", QString::number(device_count).toLatin1());
   libusb_device *dev;
   int i(0);
   QString vendortext(ui->vendorId->currentText());
   QString producttext(ui->productId->currentText());
   QString bustext(ui->busNumber->currentText());
   QString devicetext(ui->deviceNumber->currentText());
   qDebug() << "get interfaces for vendor" << vendortext << "and product" << producttext << "and bus" << bustext << "and device" << devicetext;
   while ((dev = device_list[i++]) != NULL) {
      struct libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      QString vendor1 = QString(QLatin1String("%1")).arg(desc.idVendor, 4, 16, QLatin1Char('0'));
      QString product1 = QString(QLatin1String("%1")).arg(desc.idProduct, 4, 16, QLatin1Char('0'));
      QString bus1 = QString::number(libusb_get_bus_number(dev));
      QString device1 = QString::number(libusb_get_device_address(dev));
      if (vendortext == vendor1 && producttext == product1 && bustext == bus1 && devicetext == device1) {
         Q_ASSERT(ui->interfaceNumber->count() == 0); // as we found the first and only bus/address combination
         libusb_config_descriptor* config_descriptor;
         result = libusb_get_config_descriptor_by_value(dev, ui->configurationNumber->currentText().toInt(), &config_descriptor);
         Q_ASSERT_X(result == LIBUSB_SUCCESS, "libusb_get_config_descriptor", QString::number(result).toLatin1());
         for (uint8_t n(0); n < config_descriptor->bNumInterfaces; ++n) {
            Q_ASSERT(config_descriptor->interface[n].num_altsetting > 0);
            ui->interfaceNumber->addItem(QString::number(config_descriptor->interface[n].altsetting[0].bInterfaceNumber));}
         libusb_free_config_descriptor(config_descriptor);}}
   libusb_free_device_list(device_list, 1);
   ui->interfaceNumber->setEnabled(ui->interfaceNumber->count() > 1);
   updateAltsettingList();
   void (QComboBox:: *QComboBox_currentIndexChanged_int)(int) = &QComboBox::currentIndexChanged;
   interfaceConnection = connect(ui->interfaceNumber, QComboBox_currentIndexChanged_int, this, &Dialog::updateAltsettingList);
   Q_ASSERT(interfaceConnection);}

void Dialog::updateAltsettingList(){
   disconnect(altsettingConnection);
   ui->altSetting->clear();
   libusb_device **device_list;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   Q_ASSERT_X(device_count >= 0, "libusb_get_device_list", QString::number(device_count).toLatin1());
   libusb_device *dev;
   int i(0);
   QString vendortext(ui->vendorId->currentText());
   QString producttext(ui->productId->currentText());
   QString bustext(ui->busNumber->currentText());
   QString devicetext(ui->deviceNumber->currentText());
   qDebug() << "get interfaces for vendor" << vendortext << "and product" << producttext << "and bus" << bustext << "and device" << devicetext;
   while ((dev = device_list[i++]) != NULL) {
      struct libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      QString vendor1 = QString(QLatin1String("%1")).arg(desc.idVendor, 4, 16, QLatin1Char('0'));
      QString product1 = QString(QLatin1String("%1")).arg(desc.idProduct, 4, 16, QLatin1Char('0'));
      QString bus1 = QString::number(libusb_get_bus_number(dev));
      QString device1 = QString::number(libusb_get_device_address(dev));
      if (vendortext == vendor1 && producttext == product1 && bustext == bus1 && devicetext == device1) {
         Q_ASSERT(ui->altSetting->count() == 0); // as we found the first and only bus/address combination
         libusb_config_descriptor* config_descriptor;
         result = libusb_get_config_descriptor_by_value(dev, ui->configurationNumber->currentText().toInt(), &config_descriptor);
         Q_ASSERT_X(result == LIBUSB_SUCCESS, "libusb_get_config_descriptor", QString::number(result).toLatin1());
         const libusb_interface* interface_array;
         int num_altsetting;
         for (uint8_t n(0); n < config_descriptor->bNumInterfaces; ++n) {
            num_altsetting = config_descriptor->interface[n].num_altsetting;
            Q_ASSERT(num_altsetting > 0);
            interface_array = &config_descriptor->interface[n];
            if (interface_array->altsetting[0].bInterfaceNumber == ui->interfaceNumber->currentText().toInt()) break;}
         Q_ASSERT (interface_array->altsetting[0].bInterfaceNumber == ui->interfaceNumber->currentText().toInt());
         for (uint8_t n(0); n < num_altsetting; ++n) {
            ui->altSetting->addItem(QString::number(interface_array->altsetting[0].bAlternateSetting));}
         libusb_free_config_descriptor(config_descriptor);}}
   libusb_free_device_list(device_list, 1);
   ui->altSetting->setEnabled(ui->altSetting->count() > 1);
   updateDescription();
   void (QComboBox:: *QComboBox_currentIndexChanged_int)(int) = &QComboBox::currentIndexChanged;
   altsettingConnection = connect(ui->altSetting, QComboBox_currentIndexChanged_int, this, &Dialog::updateDescription);
   Q_ASSERT(altsettingConnection);}

void Dialog::updateDescription(){
   ui->descriptionTree->clear();
   //qDebug() << "inLayout count" << ui->inLayout->children().count();
   //qDebug() << "inGroup count" << ui->inGroup->children().count();
   foreach (QObject* obj, ui->endpointIn->children()) {
      qDebug() << obj << qobject_cast<QRadioButton*>(obj);
      delete qobject_cast<QRadioButton*>(obj);} // ui->inLayout->removeWidget(qobject_cast<QRadioButton*>(obj));
   foreach (QObject* obj, ui->endpointOut->children()) {
      delete qobject_cast<QRadioButton*>(obj);}
   libusb_device **device_list;
   ssize_t device_count = libusb_get_device_list (NULL, &device_list);
   Q_ASSERT_X(device_count >= 0, "libusb_get_device_list", QString::number(device_count).toLatin1());
   libusb_device *dev;
   int i(0);
   QString vendortext(ui->vendorId->currentText());
   QString producttext(ui->productId->currentText());
   QString bustext(ui->busNumber->currentText());
   QString devicetext(ui->deviceNumber->currentText());
   qDebug() << "retrieve device for vendor" << vendortext << "and product" << producttext << "and bus" << bustext << "and device" << devicetext;
   while ((dev = device_list[i++]) != NULL) {
      struct libusb_device_descriptor desc;
      int result = libusb_get_device_descriptor(dev, &desc);
      Q_ASSERT_X(result == 0, "libusb_get_device_descriptor", QString::number(result).toLatin1());
      QString vendor1 = QString(QLatin1String("%1")).arg(desc.idVendor, 4, 16, QLatin1Char('0'));
      QString product1 = QString(QLatin1String("%1")).arg(desc.idProduct, 4, 16, QLatin1Char('0'));
      QString bus1 = QString::number(libusb_get_bus_number(dev));
      QString device1 = QString::number(libusb_get_device_address(dev));
      if (vendortext == vendor1 && producttext == product1 && bustext == bus1 && devicetext == device1) {
         Q_ASSERT(ui->descriptionTree->model()->hasChildren() == false); // as we found the first and only bus/address combination
         libusb_config_descriptor* config_descriptor;
         result = libusb_get_config_descriptor_by_value(dev, ui->configurationNumber->currentText().toInt(), &config_descriptor);
         Q_ASSERT_X(result == LIBUSB_SUCCESS, "libusb_get_config_descriptor", QString::number(result).toLatin1());
         QTreeWidgetItem* interfaceItem = new QTreeWidgetItem(ui->descriptionTree);
         interfaceItem->setText(0, tr("Interface (Class %1 ('%2'), Subclass %3, Protocol %4)")
                                .arg(config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->bInterfaceClass)
               .arg(interfaceClassMap[config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->bInterfaceClass])
               .arg(config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->bInterfaceSubClass)
               .arg(config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->bInterfaceProtocol));
         QTreeWidgetItem* endpointsitem = new QTreeWidgetItem(interfaceItem);
         endpointsitem->setText(0, tr("Endpoints"));
         for (uint8_t e(0); e < config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->bNumEndpoints; ++e) {
            QTreeWidgetItem* child = new QTreeWidgetItem(endpointsitem);
            uint8_t eAddress = config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->endpoint[e].bEndpointAddress;
            uint8_t bmAttributes = config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->endpoint[e].bmAttributes;
            // qDebug() << "Endpoint" << e << eAddress << LIBUSB_ENDPOINT_IN << LIBUSB_ENDPOINT_OUT;
            child->setText(0, tr("%1: %2 (%3); extra length = %4 bytes (%6)")
                           .arg(eAddress & 0x07).arg((eAddress & 0x80) == LIBUSB_ENDPOINT_IN ? tr("IN") : tr("OUT"))
                           .arg(QLatin1String((bmAttributes & 0x03) == LIBUSB_TRANSFER_TYPE_CONTROL ? "Control" :
                                              (bmAttributes & 0x03) == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS ? "Isochronous" :
                                              (bmAttributes & 0x03) == LIBUSB_TRANSFER_TYPE_BULK ? "Bulk" :
                                              (bmAttributes & 0x03) == LIBUSB_TRANSFER_TYPE_INTERRUPT ? "Interrupt" : "unknown/error"))
                           .arg(config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->endpoint[e].extra_length)
                           .arg(QLatin1String(QByteArray((const char*)config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->endpoint[e].extra,
                                           config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->endpoint[e].extra_length).toHex())));
            QLayout* layout = (eAddress & 0x80) == LIBUSB_ENDPOINT_IN ? ui->inLayout : ui->outLayout;
            QRadioButton* radio = new QRadioButton(QString::number(eAddress & 0x07));
            layout->addWidget(radio);
            radio->setChecked(true);}
         libusb_device_handle *dev_handle;
         result = libusb_open(dev, &dev_handle);
         if (result == LIBUSB_SUCCESS) {
            unsigned char descstring[255];
            result = libusb_get_string_descriptor_ascii(dev_handle, config_descriptor->interface[ui->interfaceNumber->currentText().toInt()].altsetting->iInterface, descstring, sizeof(descstring));
            if (result > 0) {
               QTreeWidgetItem* child = new QTreeWidgetItem(interfaceItem);
               child->setText(0, tr("Description: '%1'").arg(QLatin1String((char*)descstring)));}
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->descriptionTree);
            item->setText(0, QLatin1String("device details"));
            result = libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber, descstring, sizeof(descstring));
            qDebug() << "serial: string length" << result << QString(QLatin1String((char*) descstring));
            if (result > 0) {
               QTreeWidgetItem* child = new QTreeWidgetItem(item);
               child->setText(0, tr("Serial number: '%1'").arg(QLatin1String((char*)descstring)));}
            result = libusb_get_string_descriptor_ascii(dev_handle, desc.iManufacturer, descstring, sizeof(descstring));
            qDebug() << "manufacturer: string length" << result << QString(QLatin1String((char*) descstring));
            if (result > 0) {
               QTreeWidgetItem* child = new QTreeWidgetItem(item);
               child->setText(0, tr("Manufacturer: '%1'").arg(QLatin1String((char*)descstring)));}
            result = libusb_get_string_descriptor_ascii(dev_handle, desc.iProduct, descstring, sizeof(descstring));
            qDebug() << "product: string length" << result << QString(QLatin1String((char*) descstring));
            if (result > 0) {
               QTreeWidgetItem* child = new QTreeWidgetItem(item);
               child->setText(0, tr("Product: '%1'").arg(QLatin1String((char*)descstring)));}
            libusb_close(dev_handle);}
         else {
            QTreeWidgetItem* item = new QTreeWidgetItem(ui->descriptionTree);
            item->setText(0, tr("Cannot get device handle; likely a permission issue"));}
         libusb_free_config_descriptor(config_descriptor);
         }}
   libusb_free_device_list(device_list, 1);
   ui->descriptionTree->expandAll();}

void Dialog::processButton(QAbstractButton *button) {
   if (button == (QAbstractButton*) ui->buttonBox->button(QDialogButtonBox::Apply)) {
      qDebug() << "Save settings to QSettings facility";
      bool ok;
      QSettings settings;
      settings.beginGroup(QLatin1String("usb"));
      settings.setValue(vendorId, ui->vendorId->currentText().toInt(&ok, 16));
      Q_ASSERT_X(ok, "not a number", ui->vendorId->currentText().toLatin1());
      settings.setValue(productId, ui->productId->currentText().toInt(&ok, 16));
      Q_ASSERT_X(ok, "not a number", ui->productId->currentText().toLatin1());
      settings.setValue(busNumber, ui->busNumber->currentText().toInt(&ok));
      Q_ASSERT_X(ok, "not a number", ui->busNumber->currentText().toLatin1());
      settings.setValue(deviceNumber, ui->deviceNumber->currentText().toInt(&ok));
      Q_ASSERT_X(ok, "not a number", ui->deviceNumber->currentText().toLatin1());
      settings.setValue(interfaceNumber, ui->interfaceNumber->currentText().toInt(&ok));
      Q_ASSERT_X(ok, "not a number", ui->interfaceNumber->currentText().toLatin1());
      settings.setValue(configurationNumber, ui->configurationNumber->currentText().toInt(&ok));
      Q_ASSERT_X(ok, "not a number", ui->configurationNumber->currentText().toLatin1());
      settings.setValue(altsettingNumber, ui->altSetting->currentText().toInt(&ok));
      Q_ASSERT_X(ok, "not a number", ui->altSetting->currentText().toLatin1());
      settings.setValue(endpointIn, getEndpointAddress(ui->endpointIn));
      settings.setValue(endpointOut, getEndpointAddress(ui->endpointOut));
      settings.endGroup();
   }
   else if (button == (QAbstractButton*) ui->buttonBox->button(QDialogButtonBox::Reset)) {
      updateVendorList();}}

QVariant Dialog::getEndpointAddress(QGroupBox *box) {
   QVariant address;
   foreach (QObject* obj, box->children()) {
      QRadioButton* radio = qobject_cast<QRadioButton*>(obj);
      if (radio && radio->isChecked()) address = radio->text().toInt();}
   return address;}

