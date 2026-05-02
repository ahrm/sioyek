cmake_minimum_required(VERSION 3.16)
project(sioyek VERSION 2.0.0 LANGUAGES C CXX)

include(FindPackageHandleStandardArgs)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
  enable_language(OBJCXX)
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target")
  endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6Core 6.7 REQUIRED)
find_package(Qt6Gui 6.7 REQUIRED)
find_package(Qt6Widgets 6.7 REQUIRED)
find_package(Qt6Network 6.7 REQUIRED)
find_package(Qt6OpenGL 6.7 REQUIRED)
find_package(Qt6QuickWidgets 6.7 REQUIRED)
find_package(Qt6Svg 6.7 REQUIRED)
find_package(Qt6TextToSpeech 6.7 REQUIRED)
find_package(Qt6OpenGLWidgets 6.7 REQUIRED)

find_package(ZLIB REQUIRED)
find_package(SQLite3 REQUIRED)

set(SIOYEK_HOMEBREW_PREFIXES "$ENV{HOMEBREW_PREFIX}" /opt/homebrew /usr/local)
set(SIOYEK_MUPDF_PREFIX_HINTS)
if(APPLE)
  foreach(prefix IN LISTS SIOYEK_HOMEBREW_PREFIXES)
    if(prefix)
      list(APPEND SIOYEK_MUPDF_PREFIX_HINTS "${prefix}" "${prefix}/opt/mupdf")
    endif()
  endforeach()
endif()

find_path(MUPDF_INCLUDE_DIR
    NAMES mupdf/fitz.h
    HINTS ${SIOYEK_MUPDF_PREFIX_HINTS}
    PATH_SUFFIXES include
)
find_library(MUPDF_LIBRARY
    NAMES mupdf
    HINTS ${SIOYEK_MUPDF_PREFIX_HINTS}
    PATH_SUFFIXES lib
)
find_package_handle_standard_args(MuPDF REQUIRED_VARS MUPDF_LIBRARY MUPDF_INCLUDE_DIR)
mark_as_advanced(MUPDF_INCLUDE_DIR MUPDF_LIBRARY)

add_library(MuPDF::MuPDF UNKNOWN IMPORTED)
set_target_properties(MuPDF::MuPDF PROPERTIES
    IMPORTED_LOCATION "${MUPDF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${MUPDF_INCLUDE_DIR}"
)

qt_standard_project_setup()

qt_add_executable(sioyek WIN32 MACOSX_BUNDLE
    resources.qrc
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

if(APPLE)
  target_sources(sioyek PRIVATE
        pdf_viewer/macos_specific.mm
  )
endif()

target_compile_options(sioyek PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang>:-Wno-inconsistent-missing-override>
)

target_include_directories(sioyek PRIVATE 
    pdf_viewer
)

target_compile_definitions(sioyek PRIVATE
    QT_OPENGLEXTENSIONS_LIB
    QT_OPENGL_LIB
    QT_WIDGETS_LIB
)

target_link_libraries(sioyek PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::OpenGL
    Qt6::QuickWidgets
    Qt6::Svg
    Qt6::TextToSpeech
    Qt6::Widgets
    Qt6::OpenGLWidgets
)

if(NOT ANDROID)
  target_sources(sioyek PUBLIC
        pdf_viewer/synctex/synctex_parser.c pdf_viewer/synctex/synctex_parser.h
        pdf_viewer/synctex/synctex_parser_utils.c pdf_viewer/synctex/synctex_parser_utils.h
  )
else()
  target_compile_definitions(sioyek PRIVATE
        SIOYEK_ANDROID
  )
endif()

if(WIN32)
  target_compile_definitions(sioyek PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
        NOMINMAX
  )

  if(MSVC)
    target_compile_options(sioyek PRIVATE /bigobj /MP)
  endif()
endif()

if(UNIX AND NOT APPLE)
  target_compile_definitions(sioyek PRIVATE
        LINUX_STANDARD_PATHS
  )
endif()

target_compile_definitions(sioyek PRIVATE
    SIOYEK_QT6
)

add_library(fzf STATIC
    fzf/fzf.c
    fzf/fzf.h
)

target_link_libraries(sioyek PRIVATE
    ${CMAKE_DL_LIBS}
    MuPDF::MuPDF
    ZLIB::ZLIB
    fzf
    SQLite3::SQLite3
)

if(APPLE)
  target_link_libraries(sioyek PRIVATE
      "-framework AppKit"
  )
else()
  string(REPLACE "-mno-direct-extern-access" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

if(APPLE)
  set(EXECUTABLE sioyek)
  set(ICON "")
  set(SHORT_VERSION "${PROJECT_VERSION}")
  set_target_properties(sioyek PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist"
      MACOSX_BUNDLE_BUNDLE_NAME sioyek
      MACOSX_BUNDLE_GUI_IDENTIFIER info.sioyek.sioyek
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
      MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
  )

  set(SIOYEK_BUNDLE_FILES
      pdf_viewer/prefs.config
      pdf_viewer/prefs_user.config
      pdf_viewer/keys.config
      pdf_viewer/keys_user.config
      tutorial.pdf
  )

  add_custom_command(TARGET sioyek POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
              "${CMAKE_CURRENT_SOURCE_DIR}/pdf_viewer/shaders"
              "$<TARGET_BUNDLE_CONTENT_DIR:sioyek>/Resources/shaders"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              ${SIOYEK_BUNDLE_FILES}
              "$<TARGET_BUNDLE_CONTENT_DIR:sioyek>/Resources"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      VERBATIM
  )
endif()

install(TARGETS sioyek
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(APPLE)
  install(FILES ${SIOYEK_BUNDLE_FILES}
      DESTINATION sioyek.app/Contents/Resources
  )
  install(DIRECTORY pdf_viewer/shaders
      DESTINATION sioyek.app/Contents/Resources
  )
endif()

set(SIOYEK_DEPLOY_TOOL_OPTIONS_ARG)
if(APPLE)
  list(APPEND SIOYEK_DEPLOY_TOOL_OPTIONS_ARG
      DEPLOY_TOOL_OPTIONS
      -no-codesign
      "-qmldir=${CMAKE_CURRENT_SOURCE_DIR}/pdf_viewer/touchui"
  )
  foreach(prefix IN LISTS CMAKE_PREFIX_PATH)
    if(EXISTS "${prefix}/lib")
      list(APPEND SIOYEK_DEPLOY_TOOL_OPTIONS_ARG "-libpath=${prefix}/lib")
    endif()
    if(EXISTS "${prefix}/qml")
      list(APPEND SIOYEK_DEPLOY_TOOL_OPTIONS_ARG "-qmlimport=${prefix}/qml")
    endif()
    if(EXISTS "${prefix}/share/qt/qml")
      list(APPEND SIOYEK_DEPLOY_TOOL_OPTIONS_ARG "-qmlimport=${prefix}/share/qt/qml")
    endif()
  endforeach()
endif()

qt_generate_deploy_app_script(
    TARGET sioyek
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    ${SIOYEK_DEPLOY_TOOL_OPTIONS_ARG}
)
install(SCRIPT ${deploy_script})
