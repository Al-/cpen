#ifndef DIALOG_CONFIG_H
#define DIALOG_CONFIG_H

#include <QDialog>
#include <libusb-1.0/libusb.h>
#include <QMap>

namespace Ui {
class Dialog;
}

class QAbstractButton;
class QGroupBox;

class Dialog : public QDialog {
   Q_OBJECT
public:
   explicit Dialog(QWidget *parent = 0);
   ~Dialog();
   void slotDeviceChanged();
private:
   Ui::Dialog *ui;
   QMap<int, QString> interfaceClassMap;
   QMetaObject::Connection vendorConnection;
   QMetaObject::Connection productConnection;
   QMetaObject::Connection busConnection;
   QMetaObject::Connection deviceConnection;
   QMetaObject::Connection configurationConnection;
   QMetaObject::Connection interfaceConnection;
   QMetaObject::Connection altsettingConnection;
   void updateVendorList();
   void updateProductList();
   void updateBusList();
   void updateDeviceList();
   void updateConfigurationList();
   void updateInterfaceList();
   void updateAltsettingList();
   void updateDescription();
   void processButton(QAbstractButton* button);
   QVariant getEndpointAddress(QGroupBox* box);
};

#endif // DIALOG_CONFIG_H
