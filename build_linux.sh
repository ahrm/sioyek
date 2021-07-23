cd mupdf
make USE_SYSTEM_HARFBUZZ=yes
cd ..
qmake pdf_viewer_linux.pro
make
