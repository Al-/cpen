#-------------------------------------------------
#
# Project created by QtCreator 2016-05-22T23:15:39
#
#-------------------------------------------------

QT       += core gui
CONFIG += c++11
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = PenCapture_config
TEMPLATE = app
DEFINES += QT_NO_CAST_FROM_ASCII \
           QT_NO_CAST_TO_ASCII

SOURCES += main_config.cpp\
        dialog_config.cpp \
    global.cpp

HEADERS  += dialog_config.h \
    global.h
LIBS += /usr/lib/x86_64-linux-gnu/libusb-1.0.so
FORMS    += dialog_config.ui

DISTFILES += \
    ../../.config/Christ/PenCapture.conf \
    readme \
    capture_EZ.sh
