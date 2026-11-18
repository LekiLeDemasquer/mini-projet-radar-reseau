QT       += core gui widgets network

CONFIG   += c++17
TARGET    = Mini-Projet_Radar_ultrason
TEMPLATE  = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    udpthread.cpp

HEADERS += \
    mainwindow.h \
    udpthread.h

FORMS += \
    mainwindow.ui
