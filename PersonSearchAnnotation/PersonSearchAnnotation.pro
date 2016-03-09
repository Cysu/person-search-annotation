QT += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_MAC_SDK = macosx10.11

TARGET = PersonSearchAnnotation
TEMPLATE = app

CONFIG += c++11

SOURCES += \
  main.cpp \
  gui/MainWindow.cpp \
  gui/PreferencesDialog.cpp \
  gui/GalleryNavigator.cpp \
  gui/ImageArea.cpp \
  utils/PreferencesManager.cpp \
  utils/util_functions.cpp \
  db/DatabaseHelper.cpp

HEADERS += \
  gui/MainWindow.h \
  gui/PreferencesDialog.h \
  gui/GalleryNavigator.h \
  gui/ImageArea.h \
  utils/PreferencesManager.h \
  utils/util_functions.h \
  db/DatabaseHelper.h \
  common/PersonBBox.hpp \
  common/ImageFile.hpp

RESOURCES += \
  resources.qrc
