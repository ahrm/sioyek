TEMPLATE = app
TARGET = sioyek
INCLUDEPATH += ./pdf_viewer\
              mupdf/include \
              zlib
          

QT += core sql opengl gui widgets quickwidgets 3dcore 3danimation 3dextras 3dinput 3dlogic 3drender openglextensions
CONFIG += c++17
DEFINES += QT_3DCORE_LIB QT_3DANIMATION_LIB QT_3DEXTRAS_LIB QT_3DINPUT_LIB QT_3DLOGIC_LIB QT_3DRENDER_LIB QT_OPENGL_LIB QT_OPENGLEXTENSIONS_LIB QT_QUICKWIDGETS_LIB QT_SQL_LIB QT_WIDGETS_LIB


CONFIG(non_portable){
    DEFINES += NON_PORTABLE
}

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
           pdf_viewer/checksum.h \
           pdf_viewer/sqlite3.h \
           pdf_viewer/sqlite3ext.h \
           pdf_viewer/ui.h \
           pdf_viewer/path.h \
           pdf_viewer/utf8.h \
           pdf_viewer/utils.h \
           pdf_viewer/utf8/checked.h \
           pdf_viewer/utf8/core.h \
           pdf_viewer/utf8/unchecked.h \
           pdf_viewer/synctex/synctex_parser.h \
           pdf_viewer/synctex/synctex_parser_utils.h \
           pdf_viewer/RunGuard.h \
           pdf_viewer/OpenWithApplication.h

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
           pdf_viewer/checksum.cpp \
           pdf_viewer/sqlite3.c \
           pdf_viewer/ui.cpp \
           pdf_viewer/path.cpp \
           pdf_viewer/utils.cpp \
           pdf_viewer/synctex/synctex_parser.c \
           pdf_viewer/synctex/synctex_parser_utils.c \
           pdf_viewer/RunGuard.cpp \
           pdf_viewer/OpenWithApplication.cpp


win32{
    DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
    RC_ICONS = pdf_viewer\icon2.ico
    LIBS += -Lmupdf\platform\win32\x64\Release -llibmupdf -Lzlib -lzlib
    #LIBS += -Llibs -llibmupdf
    LIBS += opengl32.lib
}

unix:!mac {

    QMAKE_CXXFLAGS += -std=c++17
    LIBS += -ldl -Lmupdf/build/release -lmupdf -lmupdf-third -lmupdf-threads -lharfbuzz -lz
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

mac {
    QMAKE_CXXFLAGS += -std=c++17
    LIBS += -ldl -Lmupdf/build/release -lmupdf -lmupdf-third -lmupdf-threads -lz
    CONFIG+=sdk_no_version_check
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15
    ICON = pdf_viewer\icon2.ico
    QMAKE_INFO_PLIST = resources/Info.plist
}

