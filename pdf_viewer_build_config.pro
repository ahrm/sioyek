TEMPLATE = app
TARGET = sioyek
VERSION = 2.0.0
INCLUDEPATH += ./pdf_viewer\
              mupdf/include \
              zlib
          

QT += core opengl gui widgets network 3dinput 

greaterThan(QT_MAJOR_VERSION, 5){
	QT += openglwidgets
	DEFINES += SIOYEK_QT6
}
else{
	QT += openglextensions
}

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
           pdf_viewer/rapidfuzz_amalgamated.hpp \
           pdf_viewer/input.h \
           pdf_viewer/main_widget.h \
           pdf_viewer/pdf_renderer.h \
           pdf_viewer/pdf_view_opengl_widget.h \
           pdf_viewer/checksum.h \
           pdf_viewer/new_file_checker.h \
           pdf_viewer/coordinates.h \
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
           pdf_viewer/new_file_checker.cpp \
           pdf_viewer/coordinates.cpp \
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

    CONFIG(linux_app_image){
        LIBS += -ldl -Lmupdf/build/release -lmupdf -lmupdf-third -lmupdf-threads -lharfbuzz -lz
    } else {
        DEFINES += NON_PORTABLE
        DEFINES += LINUX_STANDARD_PATHS
        LIBS += -ldl -lmupdf -lmupdf-third -lgumbo -lharfbuzz -lfreetype -ljbig2dec -ljpeg -lmujs -lopenjp2 -lz
    }

    isEmpty(PREFIX){
        PREFIX = /usr
    }
    target.path = $$PREFIX/bin
    shortcutfiles.files = resources/sioyek.desktop
    shortcutfiles.path = $$PREFIX/share/applications/
    icon.files = resources/sioyek-icon-linux.png
    icon.path = $$PREFIX/share/pixmaps/
    shaders.files = pdf_viewer/shaders/
    shaders.path = $$PREFIX/share/sioyek/
    tutorial.files = tutorial.pdf
    tutorial.path = $$PREFIX/share/sioyek/
    keys.files = pdf_viewer/keys.config
    keys.path = $$PREFIX/etc/sioyek
    prefs.files = pdf_viewer/prefs.config
    prefs.path = $$PREFIX/etc/sioyek
    INSTALLS += target
    INSTALLS += shortcutfiles
    INSTALLS += icon
    INSTALLS += shaders
    INSTALLS += tutorial
    INSTALLS += keys
    INSTALLS += prefs	
    DISTFILES += resources/sioyek.desktop\
        resources/sioyek-icon-linux.png
}

mac {
    QMAKE_CXXFLAGS += -std=c++17
    LIBS += -ldl -Lmupdf/build/release -lmupdf -lmupdf-third -lmupdf-threads -lz
    CONFIG+=sdk_no_version_check
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 11
    ICON = pdf_viewer\icon2.ico
    QMAKE_INFO_PLIST = resources/Info.plist
}

