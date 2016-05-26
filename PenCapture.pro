QT +=       core gui
CONFIG +=   c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET =    PenCapture
TEMPLATE =  app
DEFINES +=  QT_NO_CAST_FROM_ASCII \
            QT_NO_CAST_TO_ASCII

SOURCES +=  main.cpp\
            dialog.cpp \
            global.cpp

HEADERS +=  dialog.h \
            global.h

FORMS +=    dialog.ui

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
