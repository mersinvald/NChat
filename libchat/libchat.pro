#-------------------------------------------------
#
# Project created by QtCreator 2015-11-09T21:15:34
#
#-------------------------------------------------

QT       -= core gui

TARGET = chat
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    log.c \
    error.c \
    non_block_io.c \
    queue.c

HEADERS += \
    types.h \
    log.h \
    error.h \
    non_block_io.h \
    queue.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libexplain
