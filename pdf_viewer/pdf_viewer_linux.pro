
TEMPLATE = app
TARGET = pdf_viewer_project
INCLUDEPATH += .

# You can make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# Please consult the documentation of the deprecated API in order to know
# how to port your code away from it.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QT += core sql opengl gui widgets quickwidgets 3dcore 3danimation 3dextras 3dinput 3dlogic 3drender openglextensions
CONFIG += debug console c++17
DEFINES += _CONSOLE QT_3DCORE_LIB QT_3DANIMATION_LIB QT_3DEXTRAS_LIB QT_3DINPUT_LIB QT_3DLOGIC_LIB QT_3DRENDER_LIB QT_OPENGL_LIB QT_OPENGLEXTENSIONS_LIB QT_QUICKWIDGETS_LIB QT_SQL_LIB QT_WIDGETS_LIB

# Input
HEADERS += book.h \
           config.h \
           database.h \
           document.h \
           document_view.h \
           fts_fuzzy_match.h \
           input.h \
           main_widget.h \
           pdf_renderer.h \
           pdf_view_opengl_widget.h \
           resource.h \
           resource1.h \
           resource2.h \
           sqlite3.h \
           sqlite3ext.h \
           ui.h \
           utf8.h \
           utils.h \
           utf8/checked.h \
           utf8/core.h \
           utf8/unchecked.h
SOURCES += book.cpp \
           config.cpp \
           database.cpp \
           document.cpp \
           document_view.cpp \
           input.cpp \
           main.cpp \
           main_widget.cpp \
           pdf_renderer.cpp \
           pdf_view_opengl_widget.cpp \
           sqlite3.c \
           ui.cpp \
           utils.cpp

LIBS += -ldl -lmupdf -lz -lfreetype -lmujs -lgif -ljbig2dec -lopenjp2 -ljpeg -lharfbuzz
QMAKE_CXXFLAGS += -std=c++17