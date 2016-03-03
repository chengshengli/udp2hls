#-------------------------------------------------
#
# Project created by QtCreator 2016-03-02T19:39:13
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = udp2hls
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    udpsocket.cpp \
    tsparser.cpp

HEADERS += \
    udpsocket.h \
    tsparser.h \
    global.h
