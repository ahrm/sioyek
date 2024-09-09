TEMPLATE = app
TARGET = sioyek
VERSION = 2.0.0

INCLUDEPATH += ./pdf_viewer \
               mupdf/include

!android{
    INCLUDEPATH += zlib
}
          

QT += core opengl gui widgets network 3dinput quickwidgets svg texttospeech

greaterThan(QT_MAJOR_VERSION, 5){
	QT += openglwidgets
	DEFINES += SIOYEK_QT6
}
else{
	QT += openglextensions
}


CONFIG += c++17
DEFINES += QT_3DINPUT_LIB QT_OPENGL_LIB QT_OPENGLEXTENSIONS_LIB QT_WIDGETS_LIB

RESOURCES += resources.qrc

SOURCES += \
        pdf_viewer/touchui/TouchSlider.cpp \
        pdf_viewer/touchui/TouchCheckbox.cpp \
        pdf_viewer/touchui/TouchListView.cpp \
        pdf_viewer/touchui/TouchCopyOptions.cpp \
        pdf_viewer/touchui/TouchRectangleSelectUI.cpp \
        pdf_viewer/touchui/TouchRangeSelectUI.cpp \
        pdf_viewer/touchui/TouchPageSelector.cpp \
        pdf_viewer/touchui/TouchConfigMenu.cpp \
        pdf_viewer/touchui/TouchTextEdit.cpp \
        pdf_viewer/touchui/TouchSearchButtons.cpp \
        pdf_viewer/touchui/TouchDeleteButton.cpp \
        pdf_viewer/touchui/TouchHighlightButtons.cpp \
        pdf_viewer/touchui/TouchSettings.cpp \
        pdf_viewer/touchui/TouchAudioButtons.cpp \
        pdf_viewer/touchui/TouchMarkSelector.cpp \
        pdf_viewer/touchui/TouchDrawControls.cpp \
        pdf_viewer/touchui/TouchMacroEditor.cpp \
        pdf_viewer/touchui/TouchGenericButtons.cpp \
        pdf_viewer/touchui/TouchMainMenu.cpp

HEADERS += \
    pdf_viewer/touchui/TouchSlider.h \
    pdf_viewer/touchui/TouchCheckbox.h \
    pdf_viewer/touchui/TouchListView.h \
    pdf_viewer/touchui/TouchCopyOptions.h \
    pdf_viewer/touchui/TouchRectangleSelectUI.h \
    pdf_viewer/touchui/TouchRangeSelectUI.h \
    pdf_viewer/touchui/TouchPageSelector.h \
    pdf_viewer/touchui/TouchConfigMenu.h \
    pdf_viewer/touchui/TouchTextEdit.h \
    pdf_viewer/touchui/TouchSearchButtons.h \
    pdf_viewer/touchui/TouchDeleteButton.h \
    pdf_viewer/touchui/TouchHighlightButtons.h \
    pdf_viewer/touchui/TouchSettings.h \
    pdf_viewer/touchui/TouchAudioButtons.h \
    pdf_viewer/touchui/TouchMarkSelector.h \
    pdf_viewer/touchui/TouchDrawControls.h \
    pdf_viewer/touchui/TouchMacroEditor.h \
    pdf_viewer/touchui/TouchGenericButtons.h \
    pdf_viewer/touchui/TouchMainMenu.h

android{
    DEFINES += SIOYEK_ANDROID
    QT += core-private
}

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
           pdf_viewer/mysortfilterproxymodel.h \
           pdf_viewer/utf8/checked.h \
           pdf_viewer/utf8/core.h \
           pdf_viewer/utf8/unchecked.h \
           pdf_viewer/RunGuard.h \
           pdf_viewer/OpenWithApplication.h \
           fzf/fzf.h


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
           pdf_viewer/mysortfilterproxymodel.cpp \
           pdf_viewer/RunGuard.cpp \
           pdf_viewer/OpenWithApplication.cpp \
           fzf/fzf.c

!android{
           HEADERS += pdf_viewer/synctex/synctex_parser.h \
           pdf_viewer/synctex/synctex_parser_utils.h
           
           SOURCES += pdf_viewer/synctex/synctex_parser.c \
           pdf_viewer/synctex/synctex_parser_utils.c
}


win32{

    CONFIG(Debug){
        CONFIG += console
        QMAKE_LFLAGS += /SUBSYSTEM:CONSOLE
    }
    CONFIG(Release){
    }
        
    # set /bigobj for MSVC, otherwise it will fail
    QMAKE_CXXFLAGS += /bigobj

    # enable multiprocessor compilation
    QMAKE_CXXFLAGS += /MP

    DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE NOMINMAX
    RC_ICONS = pdf_viewer\icon2.ico

    LIBS += -Lmupdf\platform\win32\x64\Release -llibmupdf -Lzlib -lzlib

    # CONFIG(debug){
        # LIBS += -Lmupdf\platform\win32\x64\Debug -llibmupdf -Lzlib -lzlib
    # }
    # CONFIG(release){
        # LIBS += -Lmupdf\platform\win32\x64\Release -llibmupdf -Lzlib -lzlib
    # }

    # LIBS += -llibmupdf -Lzlib -lzlib
    #LIBS += -Llibs -llibmupdf
    LIBS += opengl32.lib
}

unix:!mac:!android {

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
    LIBS += -ldl -L$$PWD/mupdf/build/release -lmupdf -lmupdf-third -lmupdf-threads -lz
    CONFIG+=sdk_no_version_check
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 11
    ICON = pdf_viewer\icon2.ico
    QMAKE_INFO_PLIST = resources/Info.plist
    LIBS += -framework AppKit
    OBJECTIVE_SOURCES += pdf_viewer/macos_specific.mm
}

android{
    !isEmpty(target.path): INSTALLS += target
    LIBS += -L$$PWD/libs/ -lmupdf_java -lcrypto_3 -lssl_3
    # ssl libraries downloaded from https://github.com/KDAB/android_openssl/tree/master/ssl_3/arm64-v8a
    ANDROID_EXTRA_LIBS += $$PWD/libs/libmupdf_java.so $$PWD/libs/libcrypto_3.so $$PWD/libs/libssl_3.so

    DISTFILES += \
        android/build.gradle \
        android/gradle.properties \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew \
        android/gradlew.bat \
        android/res/values/libs.xml\
        android/AndroidManifest.xml

}


contains(ANDROID_TARGET_ARCH,arm64-v8a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}
