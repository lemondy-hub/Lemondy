TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.c

#INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/local/Cellar/ffmpeg/4.2.3/include
LIBS += -L/usr/local/Cellar/ffmpeg/4.2.3/lib -lavcodec -lavdevice -lavfilter -lavformat -lavresample -lavutil -lpostproc -lswresample -lswscale
#LIBS += -L/usr/local/Cellar/sdl2/2.0.12_1/lib -lSDL2_test -lSDL2 -lSDL2main
