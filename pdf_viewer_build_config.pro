
TEMPLATE = app
TARGET = sioyek
INCLUDEPATH += ./pdf_viewer\
              mupdf/include
          
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
HEADERS += pdf_viewer/book.h \
           pdf_viewer/config.h \
           pdf_viewer/database.h \
           pdf_viewer/document.h \
           pdf_viewer/document_view.h \
           pdf_viewer/fts_fuzzy_match.h \
           pdf_viewer/input.h \
           pdf_viewer/main_widget.h \
           pdf_viewer/pdf_renderer.h \
           pdf_viewer/pdf_view_opengl_widget.h \
           pdf_viewer/resource.h \
           pdf_viewer/resource1.h \
           pdf_viewer/resource2.h \
           pdf_viewer/sqlite3.h \
           pdf_viewer/sqlite3ext.h \
           pdf_viewer/ui.h \
           pdf_viewer/utf8.h \
           pdf_viewer/utils.h \
           pdf_viewer/utf8/checked.h \
           pdf_viewer/utf8/core.h \
           pdf_viewer/utf8/unchecked.h

SOURCES += pdf_viewer/book.cpp \
           pdf_viewer/config.cpp \
           pdf_viewer/database.cpp \
           pdf_viewer/document.cpp \
           pdf_viewer/document_view.cpp \
           pdf_viewer/input.cpp \
           pdf_viewer/main.cpp \
           pdf_viewer/main_widget.cpp \
           pdf_viewer/pdf_renderer.cpp \
           pdf_viewer/pdf_view_opengl_widget.cpp \
           pdf_viewer/sqlite3.c \
           pdf_viewer/ui.cpp \
           pdf_viewer/utils.cpp


win32{
    LIBS += -Lmupdf\platform\win32\x64\Release -llibmupdf
    LIBS += opengl32.lib
}

unix{
    QMAKE_CXXFLAGS += -std=c++17
    LIBS += -ldl -Lmupdf/build/release -lmupdf -lmupdf-third -lmupdf-threads -lharfbuzz
    isEmpty(PREFIX){
        PREFIX = /usr
    }
    target.path = $$PREFIX/bin
    shortcutfiles.files = resources/sioyek.desktop
    shortcutfiles.path = $$PREFIX/share/applications/
    data.files = resources/sioyek-icon-linux.png
    data.path = $$PREFIX/share/pixmaps/
    INSTALLS += shortcutfiles
    INSTALLS += data
    INSTALLS += target
    DISTFILES += resources/sioyek.desktop\
        resources/sioyek-icon-linux.png
}

