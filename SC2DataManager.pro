#-------------------------------------------------
#
# Project created by QtCreator 2011-08-04T00:26:10
#
#-------------------------------------------------

QT       += core gui

TARGET = SC2DataManager
TEMPLATE = app


SOURCES += LoadXML.cpp \
    main.cpp \
    mainwindow.cpp \
    MapManager.cpp \
    NodeMatch.cpp \
    pugixml.cpp

HEADERS  += LoadXML.h \
    mainwindow.h \
    MapManager.h \
    NodeMatch.h \
    pugiconfig.hpp \
    pugixml.hpp

FORMS    += mainwindow.ui

#---- Inclusion of external project dependencies ---

INCLUDEPATH += $$quote(C:/Program Files (x86)/boost/boost_1_44)
CONFIG(debug, debug|release){
LIBS        += -L$$quote(C:/Program Files (x86)/boost/boost_1_44/lib) \
                    -llibboost_regex-vc100-mt-gd-1_44 \
					-llibboost_filesystem-vc100-mt-gd-1_44
} else {
LIBS        += -L$$quote(C:/Program Files (x86)/boost/boost_1_44/lib) \
                    -llibboost_regex-vc100-mt-1_44 \
					-llibboost_filesystem-vc100-mt-1_44
}









