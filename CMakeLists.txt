cmake_minimum_required(VERSION 3.16)

project(sioyek LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Network OpenGL 3DInput REQUIRED)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)

# MuPDF Configuration
set(MUPDF_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/mupdf/include")
set(MUPDF_LIB_DIR "${CMAKE_SOURCE_DIR}/mupdf/build/release")

find_library(MUPDF_LIB mupdf PATHS ${MUPDF_LIB_DIR} NO_DEFAULT_PATH)
find_library(MUPDF_THIRD_LIB mupdf-third PATHS ${MUPDF_LIB_DIR} NO_DEFAULT_PATH)
find_library(MUPDF_THREADS_LIB mupdf-threads PATHS ${MUPDF_LIB_DIR} NO_DEFAULT_PATH)

if(NOT MUPDF_LIB)
    find_library(MUPDF_LIB mupdf)
endif()
if(NOT MUPDF_THIRD_LIB)
    find_library(MUPDF_THIRD_LIB mupdf-third)
endif()
if(NOT MUPDF_THREADS_LIB)
    find_library(MUPDF_THREADS_LIB mupdf-threads)
endif()

# Source Files
set(SOURCES
    pdf_viewer/book.cpp
    pdf_viewer/checksum.cpp
    pdf_viewer/config.cpp
    pdf_viewer/coordinates.cpp
    pdf_viewer/database.cpp
    pdf_viewer/document.cpp
    pdf_viewer/document_view.cpp
    pdf_viewer/input.cpp
    pdf_viewer/input_processor.cpp
    pdf_viewer/main.cpp
    pdf_viewer/main_widget.cpp
    pdf_viewer/new_file_checker.cpp
    pdf_viewer/OpenWithApplication.cpp
    pdf_viewer/path.cpp
    pdf_viewer/pdf_renderer.cpp
    pdf_viewer/pdf_view_opengl_widget.cpp
    pdf_viewer/RunGuard.cpp
    pdf_viewer/sqlite3.c
    pdf_viewer/synctex/synctex_parser.c
    pdf_viewer/synctex/synctex_parser_utils.c
    pdf_viewer/ui.cpp
    pdf_viewer/ui_manager.cpp
    pdf_viewer/utils.cpp
)

set(HEADERS
    pdf_viewer/book.h
    pdf_viewer/checksum.h
    pdf_viewer/config.h
    pdf_viewer/coordinates.h
    pdf_viewer/database.h
    pdf_viewer/document.h
    pdf_viewer/document_view.h
    pdf_viewer/fts_fuzzy_match.h
    pdf_viewer/input.h
    pdf_viewer/input_processor.h
    pdf_viewer/main_widget.h
    pdf_viewer/new_file_checker.h
    pdf_viewer/OpenWithApplication.h
    pdf_viewer/path.h
    pdf_viewer/pdf_renderer.h
    pdf_viewer/pdf_view_opengl_widget.h
    pdf_viewer/RunGuard.h
    pdf_viewer/ui.h
    pdf_viewer/ui_manager.h
    pdf_viewer/utils.h
)

add_executable(sioyek ${SOURCES} ${HEADERS})

target_include_directories(sioyek PRIVATE
    pdf_viewer
    ${MUPDF_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/zlib
)

target_link_libraries(sioyek PRIVATE
    Qt5::Widgets
    Qt5::Network
    Qt5::OpenGL
    Qt5::3DInput
    ${MUPDF_LIB}
    ${MUPDF_THIRD_LIB}
    Freetype::Freetype
    ZLIB::ZLIB
    dl
    pthread
)

if(MUPDF_THREADS_LIB)
    target_link_libraries(sioyek PRIVATE ${MUPDF_THREADS_LIB})
endif()

if(UNIX AND NOT APPLE)
    add_definitions(-DLINUX_STANDARD_PATHS)
    add_definitions(-DNON_PORTABLE)

    # Check if we are using local mupdf build or system one
    # If system one, we might need extra libs
    if(NOT MUPDF_LIB MATCHES "${CMAKE_SOURCE_DIR}/mupdf/build/release")
         # Assuming system mupdf needs these
         target_link_libraries(sioyek PRIVATE gumbo harfbuzz jbig2dec jpeg mujs openjp2)
    else()
         # Local mupdf-third should contain dependencies, but .pro links harfbuzz explicitly sometimes?
         # "CONFIG(linux_app_image){ LIBS += ... -lmupdf -lmupdf-third -lmupdf-threads -lharfbuzz -lz }"
         # It seems harfbuzz is linked explicitly even with local mupdf-third?
         # I'll try to find it.
         find_library(HARFBUZZ_LIB harfbuzz)
         if(HARFBUZZ_LIB)
            target_link_libraries(sioyek PRIVATE ${HARFBUZZ_LIB})
         endif()
    endif()
endif()

if(APPLE)
    # macOS settings
endif()

if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
    target_link_libraries(sioyek PRIVATE opengl32)
endif()
