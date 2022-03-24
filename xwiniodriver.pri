INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

HEADERS += \
    $$PWD/xwiniodriver.h

SOURCES += \
    $$PWD/xwiniodriver.cpp

!contains(XCONFIG, xprocess) {
    XCONFIG += xprocess
    include(../XProcess/xprocess.pri)
}

win32-msvc* {
    LIBS += ntdll.lib
}
