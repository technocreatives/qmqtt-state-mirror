#-------------------------------------------------
#
# Project created by QtCreator 2015-06-23T09:11:43
#
#-------------------------------------------------

QT       += network
QT       += qml
QT       -= gui

include(local_settings.pri)

TARGET = qmqtt-state-mirror
TEMPLATE = lib

CONFIG += c++11

DEFINES += QMQTTSTATEMIRROR_LIBRARY

SOURCES += \
    statemanager.cpp

HEADERS +=\
        mqttstatemirror_global.h \
    statemanager.h

LIBS += -lqmqtt

unix {
    target.path = /usr/lib
    INSTALLS += target
}
