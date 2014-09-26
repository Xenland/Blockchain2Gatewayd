#-------------------------------------------------
#
# Project created by QtCreator 2014-09-16T16:23:55
#
#-------------------------------------------------

QT       += core network sql

QT       -= gui

TARGET = b2g
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    gatewayd_api.cpp \
    blockchain_api.cpp \
    sql.cpp

HEADERS += \
    gatewayd_api.h \
    blockchain_api.h \
    sql.h
