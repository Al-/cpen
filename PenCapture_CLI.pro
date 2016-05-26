CONFIG += console
CONFIG -= app_bundle
QT +=       core
QT -= gui
CONFIG +=   c++11

TARGET =    PenCapture_CLI
TEMPLATE =  app
DEFINES +=  QT_NO_CAST_FROM_ASCII \
            QT_NO_CAST_TO_ASCII

SOURCES +=  main.cpp\
            dialog.cpp \
            global.cpp

HEADERS +=  dialog.h \
            global.h

LIBS +=     ~/lib/libcpen_backend.so

exists( /usr/lib/i386-linux-gnu/libusb-1.0.so ) {         # intel 32 bit architecture
   librarypath = /usr/lib/i386-linux-gnu
}
exists( /usr/lib/x86_64-linux-gnu/libusb-1.0.so ) {       # intel 64 bit architecture
   librarypath = /usr/lib/x86_64-linux-gnu
}

LIBS += $${librarypath}/libusb-1.0.so
LIBS += \
        $${librarypath}/libopencv_imgproc.so \
        $${librarypath}/libopencv_core.so \
        $${librarypath}/libopencv_highgui.so \
        $${librarypath}/libopencv_features2d.so \
        $${librarypath}/libopencv_flann.so \
        $${librarypath}/libopencv_calib3d.so \
        $${librarypath}/libopencv_video.so \
        $${librarypath}/libopencv_stitching.so
