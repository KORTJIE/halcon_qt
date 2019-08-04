
CONFIG		+= qt debug
QT              += core gui  widgets


  #includes
  INCLUDEPATH   += "$$(HALCONROOT)/include"
  INCLUDEPATH   += "$$(HALCONROOT)/include/halconcpp"

  #libs
  win32:LIBS    += "$$(HALCONROOT)/lib/$$(HALCONARCH)/halconcpp.lib" \
                   "$$(HALCONROOT)/lib/$$(HALCONARCH)/halcon.lib"


#sources
HEADERS	    += qhalconwindow.h
HEADERS	    += matching.h
SOURCES	    += qhalconwindow.cpp
SOURCES	    += matching.cpp
SOURCES	    += main.cpp

DISTFILES += \
    README.md
