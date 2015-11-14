#-------------------------------------------------
#
# Project created by QtCreator 2015-11-09T21:15:34
#
#-------------------------------------------------

QT       -= core gui

TARGET = chat
TEMPLATE = lib
CONFIG += staticlib

LIBS += -lexplain

SOURCES += libchat.c \
    log.c \
    error.c \
    non_block_io.c \
    fd_recv.c \
    fd_send.c \
    sig_handler.c \
    queue.c

HEADERS += \
    types.h \
    libchat.h \
    log.h \
    error.h \
    non_block_io.h \
    ancillary.h \
    sig_handler.h \
    queue.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
