TEMPLATE = subdirs

SUBDIRS = libchat \
          server \
          client

unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libexplain
