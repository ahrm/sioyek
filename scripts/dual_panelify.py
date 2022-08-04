'''
Creates a dual panel version of current PDF file. Here is example configuration in `prefs_user.config`:

new_command _dual_panelify python /path/to/dual_panelify.py "%{sioyek_path}" "%{file_path}"

now you can execute _dual_panelify command using sioyek's command window.
'''

import sys
import random
import subprocess
import datetime
import fitz
import numpy as np

from PyPDF2 import PdfWriter, PdfReader, PageObject

UPDATE_EVERY_SECONDS = 3

def first_and_last_nonzero(arr):
    first_index = -1
    last_index=-1
    index = 0

    while arr[index] == 0 and index < arr.size:
        index += 1
    
    if index < arr.size:
        first_index = index-1
    
    index = arr.size - 1
    while arr[index] == 0 and index >= 0:
        index -= 1
    last_index = index + 1
    return first_index, last_index

def get_pixmap_bounding_box(pixmap):
    pixmap_np = np.frombuffer(pixmap.samples, dtype=np.uint8).reshape(pixmap.height, pixmap.width, 3).mean(axis=2)

    vertical_hist = 255 - pixmap_np.sum(axis=0)
    vertical_hist_copy = vertical_hist.copy()
    v_nth = vertical_hist.size // 3
    vertical_hist_copy.partition(v_nth)
    v_thresh = vertical_hist_copy[v_nth]
    v_foreground = vertical_hist > v_thresh
    v_first_nonzero, v_last_nonzero = first_and_last_nonzero(v_foreground)

    bottom_right_x = v_last_nonzero
    bottom_right_y = pixmap.height
    
    top_left_x = v_first_nonzero
    top_left_y = 0

    return fitz.Rect(top_left_x, top_left_y, bottom_right_x, bottom_right_y)

def rect_union(rect1, rect2):
    return fitz.Rect(min(rect1.x0, rect2.x0), min(rect1.y0, rect2.y0), max(rect1.x1, rect2.x1), max(rect1.y1, rect2.y1))

def get_document_cropbox(doc):
    pages = [random.randint(0, doc.page_count-1) for _ in range(5)]
    boxes = []

    for i in pages:
        page = doc.load_page(i)
        pixmap = page.get_pixmap()
        boxes.append(get_pixmap_bounding_box(pixmap))
    
    res = boxes[0]
    for box in boxes[1:]:
        res = rect_union(res, box)
    return res

if __name__ == '__main__':
    last_update_time = datetime.datetime.now()

    sioyek_path = sys.argv[1]
    single_panel_file_path = sys.argv[2]

    dual_panel_file_path = single_panel_file_path.replace('.pdf', '_dual_panel.pdf')

    pdf_reader = PdfReader(single_panel_file_path)
    pdf_writer = PdfWriter()

    doc = fitz.open(single_panel_file_path)
    cropbox = get_document_cropbox(doc)

    for i in range(pdf_reader.numPages // 2):
        if (datetime.datetime.now() - last_update_time).seconds > UPDATE_EVERY_SECONDS:
            last_update_time = datetime.datetime.now()
            subprocess.run([sioyek_path, '--execute-command', 'set_status_string', '--execute-command-data', 'Dual panelifying {} / {}'.format((i+1) * 2, pdf_reader.numPages)])

        page1 = pdf_reader.getPage(2 * i)
        page2 = pdf_reader.getPage(2 * i+1)

        original_width1 = page1.mediaBox.width
        original_width2 = page2.mediaBox.width

        page1.mediaBox.setLowerLeft(cropbox.bottom_left)
        page1.mediaBox.setUpperRight(cropbox.top_right)
        page2.mediaBox.setLowerLeft(cropbox.bottom_left)
        page2.mediaBox.setUpperRight(cropbox.top_right)

        total_width = original_width1 + page2.mediaBox.width
        total_height = -max([page1.mediaBox.height, page2.mediaBox.height])
        new_page = PageObject.createBlankPage(None, total_width, total_height)
        new_page.mergePage(page1)
        new_page.mergeTranslatedPage(page2, page1.mediaBox.width, 0)
        pdf_writer.add_page(new_page)
    
    if pdf_reader.numPages % 2 == 1:
        pdf_writer.add_page(pdf_reader.getPage(pdf_reader.numPages - 1))

    subprocess.run([sioyek_path, '--execute-command', 'set_status_string', '--execute-command-data', 'Writing new file to disk'])
    with open(dual_panel_file_path, 'wb') as f:
        pdf_writer.write(f)

    subprocess.run([sioyek_path, '--execute-command', 'clear_status_string'])
    subprocess.run([sioyek_path, '--new-window', dual_panel_file_path])