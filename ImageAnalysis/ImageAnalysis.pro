#-------------------------------------------------
#
# Project created by QtCreator 2016-10-17T18:21:40
#
#-------------------------------------------------

QT	   += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 4): CONFIG += c++11

TARGET = ImageAnalysis
TEMPLATE = app


SOURCES += \
                Convolution.cpp \
                main.cpp \
                MainWindow.cpp \
    FilterBox.cpp \
    TresholdBox.cpp

HEADERS += \
                Convolution.h \
                Convolution.inl \
                MainWindow.h \
    FilterBox.h \
    TresholdBox.h

FORMS   += \
                MainWindow.ui \
    FilterBox.ui \
    TresholdBox.ui
