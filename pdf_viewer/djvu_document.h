#pragma once

#include <mupdf/fitz.h>

// Register the DjVuLibre-backed document handler with a MuPDF context.
// MuPDF clones share their document-handler registry, so this only needs to
// be called for Sioyek's root context before renderer threads are started.
void register_djvu_document_handler(fz_context* context);
