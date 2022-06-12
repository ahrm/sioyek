TEMPLATE = app
TARGET = sioyek
VERSION = 1.3.0
INCLUDEPATH += ./pdf_viewer

QT += core opengl gui widgets network 3dinput openglextensions
CONFIG += c++17
DEFINES += QT_3DINPUT_LIB QT_OPENGL_LIB QT_OPENGLEXTENSIONS_LIB QT_WIDGETS_LIB

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
           pdf_viewer/new_file_checker.h \
           pdf_viewer/coordinates.h \
           pdf_viewer/ui.h \
           pdf_viewer/path.h \
           pdf_viewer/utils.h \
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
           pdf_viewer/new_file_checker.cpp \
           pdf_viewer/coordinates.cpp \
           pdf_viewer/ui.cpp \
           pdf_viewer/path.cpp \
           pdf_viewer/utils.cpp \
           pdf_viewer/RunGuard.cpp \
           pdf_viewer/OpenWithApplication.cpp


win32{
    DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
    RC_ICONS = pdf_viewer\icon2.ico
    LIBS += -llibmupdf -llibsqlite3 -llibsynctex -Lzlib -lzlib
    #LIBS += -Llibs -llibmupdf
    LIBS += opengl32.lib
}

unix:!mac {

    QMAKE_CXXFLAGS += -std=c++17

    CONFIG(linux_app_image){
        LIBS += -ldl -lmupdf -lmupdf-third -lsynctex -lsqlite3 -lmupdf-threads -lharfbuzz -lz
    } else {
        DEFINES += NON_PORTABLE
        DEFINES += LINUX_STANDARD_PATHS
        LIBS += -ldl -lmupdf -lmupdf-third -lgumbo -lsynctex -lsqlite3 -lharfbuzz -lfreetype -ljbig2dec -ljpeg -lmujs -lopenjp2 -lz
    }

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
    LIBS += -ldl -lmupdf -lmupdf-third -lmupdf-threads -lsynctex -lsqlite3 -lz 
    CONFIG+=sdk_no_version_check
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 11
    ICON = pdf_viewer\icon2.ico
    QMAKE_INFO_PLIST = resources/Info.plist
}

