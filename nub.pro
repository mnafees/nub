#-------------------------------------------------
#
# Project created by QtCreator 2013-05-06T18:20:26
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = nub
TEMPLATE = app

include(o2/o2.pri)

SOURCES += main.cpp \
    nub.cpp \
    qt-json/json.cpp \
    ChunkUploader.cpp

HEADERS  += \
    nub.h \
    qt-json/json.h \
    ChunkUploader.h

FORMS    += \
    nub.ui

win32|linux {
QT += webkit
}

macx {
LIBS += -framework Cocoa -framework WebKit
OBJECTIVE_SOURCES += mac/MacWebView.mm
HEADERS  += mac/MacWebView.h
}
