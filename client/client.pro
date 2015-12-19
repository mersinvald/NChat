TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += "../"

SOURCES += \
    client.c \
    interface.c \
    window.c

LIBS += -lexplain -lpthread -lncurses

unix:!macx: LIBS += -L$$OUT_PWD/../libchat/ -lchat

INCLUDEPATH += $$PWD/../libchat
DEPENDPATH += $$PWD/../libchat

HEADERS += \
    interface.h \
    client.h \
    window.h

unix:!macx: LIBS += -L$$OUT_PWD/../libchat/ -lchat

INCLUDEPATH += $$PWD/../libchat
DEPENDPATH += $$PWD/../libchat

unix:!macx: PRE_TARGETDEPS += $$OUT_PWD/../libchat/libchat.a

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libexplain
