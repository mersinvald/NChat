TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt


QMAKE_CFLAGS += -Wall -Wextra

SOURCES += \
    server.c \
    message_relay.c

HEADERS += \
    server.h \
    message_relay.h

LIBS += -lpthread

unix:!macx: LIBS += -L$$OUT_PWD/../libchat/ -lchat

INCLUDEPATH += $$PWD/../libchat
DEPENDPATH += $$PWD/../libchat

unix:!macx: PRE_TARGETDEPS += $$OUT_PWD/../libchat/libchat.a

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libexplain
