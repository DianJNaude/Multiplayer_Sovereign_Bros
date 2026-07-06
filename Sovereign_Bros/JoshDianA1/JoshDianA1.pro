QT += core gui widgets network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    networkmanager.cpp \
    soundmanager.cpp \
    Playerguy.cpp \
    enemies.cpp \
    gamewindow.cpp \
    levelmanager.cpp \
    main.cpp \
    multiplayermanager.cpp \
    obstacle.cpp \
    scoremanager.cpp

HEADERS += \
    networkmanager.h \
    soundmanager.h \
    Obstacle.h \
    Playerguy.h \
    enemies.h \
    gamewindow.h \
    levelmanager.h \
    multiplayermanager.h \
    scoremanager.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    sounds/coin.wav \
    sounds/grenade.wav \
    sounds/shoot.wav \
    sounds/jump.wav \
    sounds/level_complete.wav

RESOURCES += resources.qrc
