cmake_minimum_required(VERSION 3.16)
project(sioyek VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS 3DInput Gui Network OpenGL QuickWidgets Svg TextToSpeech Widgets OpenGLWidgets)

qt_standard_project_setup()

qt_add_executable(sioyek WIN32 MACOSX_BUNDLE
    pdf_viewer/OpenWithApplication.cpp pdf_viewer/OpenWithApplication.h
    pdf_viewer/RunGuard.cpp pdf_viewer/RunGuard.h
    pdf_viewer/book.cpp pdf_viewer/book.h
    pdf_viewer/checksum.cpp pdf_viewer/checksum.h
    pdf_viewer/config.cpp pdf_viewer/config.h
    pdf_viewer/coordinates.cpp pdf_viewer/coordinates.h
    pdf_viewer/database.cpp pdf_viewer/database.h
    pdf_viewer/document.cpp pdf_viewer/document.h
    pdf_viewer/document_view.cpp pdf_viewer/document_view.h
    pdf_viewer/fts_fuzzy_match.h
    pdf_viewer/input.cpp pdf_viewer/input.h
    pdf_viewer/main.cpp
    pdf_viewer/main_widget.cpp pdf_viewer/main_widget.h
    pdf_viewer/mysortfilterproxymodel.cpp pdf_viewer/mysortfilterproxymodel.h
    pdf_viewer/new_file_checker.cpp pdf_viewer/new_file_checker.h
    pdf_viewer/path.cpp pdf_viewer/path.h
    pdf_viewer/pdf_renderer.cpp pdf_viewer/pdf_renderer.h
    pdf_viewer/pdf_view_opengl_widget.cpp pdf_viewer/pdf_view_opengl_widget.h
    pdf_viewer/rapidfuzz_amalgamated.hpp
    pdf_viewer/sqlite3.c pdf_viewer/sqlite3.h
    pdf_viewer/sqlite3ext.h
    pdf_viewer/touchui/TouchAudioButtons.cpp pdf_viewer/touchui/TouchAudioButtons.h
    pdf_viewer/touchui/TouchCheckbox.cpp pdf_viewer/touchui/TouchCheckbox.h
    pdf_viewer/touchui/TouchConfigMenu.cpp pdf_viewer/touchui/TouchConfigMenu.h
    pdf_viewer/touchui/TouchCopyOptions.cpp pdf_viewer/touchui/TouchCopyOptions.h
    pdf_viewer/touchui/TouchDeleteButton.cpp pdf_viewer/touchui/TouchDeleteButton.h
    pdf_viewer/touchui/TouchDrawControls.cpp pdf_viewer/touchui/TouchDrawControls.h
    pdf_viewer/touchui/TouchGenericButtons.cpp pdf_viewer/touchui/TouchGenericButtons.h
    pdf_viewer/touchui/TouchHighlightButtons.cpp pdf_viewer/touchui/TouchHighlightButtons.h
    pdf_viewer/touchui/TouchListView.cpp pdf_viewer/touchui/TouchListView.h
    pdf_viewer/touchui/TouchMacroEditor.cpp pdf_viewer/touchui/TouchMacroEditor.h
    pdf_viewer/touchui/TouchMainMenu.cpp pdf_viewer/touchui/TouchMainMenu.h
    pdf_viewer/touchui/TouchMarkSelector.cpp pdf_viewer/touchui/TouchMarkSelector.h
    pdf_viewer/touchui/TouchPageSelector.cpp pdf_viewer/touchui/TouchPageSelector.h
    pdf_viewer/touchui/TouchRangeSelectUI.cpp pdf_viewer/touchui/TouchRangeSelectUI.h
    pdf_viewer/touchui/TouchRectangleSelectUI.cpp pdf_viewer/touchui/TouchRectangleSelectUI.h
    pdf_viewer/touchui/TouchSearchButtons.cpp pdf_viewer/touchui/TouchSearchButtons.h
    pdf_viewer/touchui/TouchSettings.cpp pdf_viewer/touchui/TouchSettings.h
    pdf_viewer/touchui/TouchSlider.cpp pdf_viewer/touchui/TouchSlider.h
    pdf_viewer/touchui/TouchTextEdit.cpp pdf_viewer/touchui/TouchTextEdit.h
    pdf_viewer/ui.cpp pdf_viewer/ui.h
    pdf_viewer/utf8.h
    pdf_viewer/utf8/checked.h
    pdf_viewer/utf8/core.h
    pdf_viewer/utf8/unchecked.h
    pdf_viewer/utils.cpp pdf_viewer/utils.h
)
target_include_directories(sioyek PRIVATE
    mupdf/include
    pdf_viewer
)

target_compile_definitions(sioyek PRIVATE
    QT_3DINPUT_LIB
    QT_OPENGLEXTENSIONS_LIB
    QT_OPENGL_LIB
    QT_WIDGETS_LIB
)

target_link_libraries(sioyek PRIVATE
    Qt::3DInput
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::OpenGL
    Qt::QuickWidgets
    Qt::Svg
    Qt::TextToSpeech
    Qt::Widgets
)


# Resources:
set(resources_resource_files
    "begin.png"
    "data/embedding.npy"
    "data/linear.npy"
    "end.png"
    "icons/B.svg"
    "icons/P.svg"
    "icons/arrow-begin.svg"
    "icons/arrow-end.svg"
    "icons/bookmark-add.svg"
    "icons/bookmark-g.svg"
    "icons/bookmark.svg"
    "icons/copy.svg"
    "icons/document-page-number.svg"
    "icons/draw.svg"
    "icons/fit-horizontal-enabled.svg"
    "icons/fit-horizontal.svg"
    "icons/fullscreen-enabled.svg"
    "icons/fullscreen.svg"
    "icons/go-to-file.svg"
    "icons/google-scholar.svg"
    "icons/google.svg"
    "icons/highlight-g.svg"
    "icons/highlight.svg"
    "icons/home.svg"
    "icons/hourglass.svg"
    "icons/jump-right.svg"
    "icons/left.svg"
    "icons/link-enabled.svg"
    "icons/link.svg"
    "icons/lock-enabled.svg"
    "icons/lock.svg"
    "icons/next.svg"
    "icons/page.svg"
    "icons/paper-download.svg"
    "icons/pause.svg"
    "icons/play.svg"
    "icons/portal-enabled.svg"
    "icons/portal.svg"
    "icons/previous.svg"
    "icons/question.svg"
    "icons/right.svg"
    "icons/ruler-enabled.svg"
    "icons/ruler.svg"
    "icons/search.svg"
    "icons/select_text.svg"
    "icons/stop.svg"
    "icons/table-of-contents.svg"
    "icons/text-selection.svg"
    "icons/tts-enabled.svg"
    "icons/tts.svg"
    "icons/unlink.svg"
    "pdf_viewer/keys.config"
    "pdf_viewer/prefs.config"
    "pdf_viewer/shaders/custom_colors.fragment"
    "pdf_viewer/shaders/dark_mode.fragment"
    "pdf_viewer/shaders/debug.fragment"
    "pdf_viewer/shaders/highlight.fragment"
    "pdf_viewer/shaders/line.fragment"
    "pdf_viewer/shaders/line.vertex"
    "pdf_viewer/shaders/separator.fragment"
    "pdf_viewer/shaders/simple.fragment"
    "pdf_viewer/shaders/simple.vertex"
    "pdf_viewer/shaders/stencil.fragment"
    "pdf_viewer/shaders/stencil.vertex"
    "pdf_viewer/shaders/unrendered_page.fragment"
    "pdf_viewer/shaders/vertical_bar.fragment"
    "pdf_viewer/shaders/vertical_bar_dark.fragment"
    "pdf_viewer/touchui/TouchAudioButtons.qml"
    "pdf_viewer/touchui/TouchButtonGroup.qml"
    "pdf_viewer/touchui/TouchCheckbox.qml"
    "pdf_viewer/touchui/TouchConfigMenu.qml"
    "pdf_viewer/touchui/TouchCopyOptions.qml"
    "pdf_viewer/touchui/TouchDeleteButton.qml"
    "pdf_viewer/touchui/TouchDrawControls.qml"
    "pdf_viewer/touchui/TouchGenericButtons.qml"
    "pdf_viewer/touchui/TouchHighlightButtons.qml"
    "pdf_viewer/touchui/TouchListView.qml"
    "pdf_viewer/touchui/TouchMacroEditor.qml"
    "pdf_viewer/touchui/TouchMainMenu.qml"
    "pdf_viewer/touchui/TouchMarkSelector.qml"
    "pdf_viewer/touchui/TouchPageSelector.qml"
    "pdf_viewer/touchui/TouchRangeSelectUI.qml"
    "pdf_viewer/touchui/TouchRectangleSelectUI.qml"
    "pdf_viewer/touchui/TouchSearchButtons.qml"
    "pdf_viewer/touchui/TouchSettings.qml"
    "pdf_viewer/touchui/TouchSlider.qml"
    "pdf_viewer/touchui/TouchTextEdit.qml"
    "pdf_viewer/touchui/TouchTreeView.qml"
    "tutorial.pdf"
)

qt_add_resources(sioyek "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

if(NOT ANDROID)
    target_sources(sioyek PUBLIC
        pdf_viewer/synctex/synctex_parser.c pdf_viewer/synctex/synctex_parser.h
        pdf_viewer/synctex/synctex_parser_utils.c pdf_viewer/synctex/synctex_parser_utils.h
    )

    target_include_directories(sioyek PRIVATE
        zlib
    )
endif()

if((QT_VERSION_MAJOR GREATER 5))
    target_compile_definitions(sioyek PRIVATE
        SIOYEK_QT6
    )

    target_link_libraries(sioyek PRIVATE
        Qt::OpenGLWidgets
    )
endif()



target_link_libraries(sioyek PRIVATE -ldl)
target_link_libraries(sioyek PRIVATE -L/home/hex/sioyek-private/mupdf/build/release)
target_link_libraries(sioyek PRIVATE -lmupdf)
target_link_libraries(sioyek PRIVATE -lmupdf-third)
target_link_libraries(sioyek PRIVATE -lmupdf-threads)
target_link_libraries(sioyek PRIVATE -lharfbuzz)
target_link_libraries(sioyek PRIVATE -lz)

# if((( ( UNIX ) AND NOT APPLE ) AND NOT ANDROID))
#     target_link_libraries(sioyek PRIVATE
#         # Remove: Lmupdf/build/release
#         WrapHarfbuzz::WrapHarfbuzz
#         dl
#         mupdf
#         mupdf-third
#         mupdf-threads
#         z
#     )
# endif()

if(APPLE)
    target_link_libraries(sioyek PRIVATE
        # Remove: Lmupdf/build/release
        dl
        mupdf
        mupdf-third
        mupdf-threads
        z
    )

    target_compile_options(sioyek
        -std=c++17
    )
endif()

install(TARGETS sioyek
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET sioyek
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
