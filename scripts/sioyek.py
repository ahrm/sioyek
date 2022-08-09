import subprocess
import sqlite3
from dataclasses import dataclass
from functools import lru_cache
import fitz
import regex
import os


def merge_rects(rects):
    '''
    Merge close rectangles in a line (e.g. rectangles corresponding to a single character or word)
    '''
    if len(rects) == 0:
        return []

    resulting_rects = [rects[0]]
    y_threshold = abs(rects[0].y1 - rects[0].y0) * 0.3

    def extend_last_rect(new_rect):
        resulting_rects[-1].x1 = max(resulting_rects[-1].x1, new_rect.x1)
        resulting_rects[-1].y0 = min(resulting_rects[-1].y0, new_rect.y0)
        resulting_rects[-1].y1 = min(resulting_rects[-1].y1, new_rect.y1)

    for rect in rects[1:]:
        if abs(rect.y0 - resulting_rects[-1].y0) < y_threshold:
            extend_last_rect(rect)
        else:
            resulting_rects.append(rect)
    return resulting_rects

class Sioyek:

    def __init__(self, sioyek_path, local_database_path=None, shared_database_path=None):
        self.path = sioyek_path
        self.is_dummy_mode = False

        self.local_database = None
        self.shared_database = None
        self.cached_path_hash_map = None

        if local_database_path != None:
            self.local_database_path = local_database_path
            self.local_database = sqlite3.connect(self.local_database_path)

        if shared_database_path != None:
            self.shared_database_path = shared_database_path
            self.shared_database = sqlite3.connect(self.shared_database_path)
    
    def get_local_database(self):
        return self.local_database

    def get_shared_database(self):
        return self.shared_database

    def set_dummy_mode(self, mode):
        self.is_dummy_mode = mode
    
    def get_path_hash_map(self):

        if self.cached_path_hash_map == None:
            query = 'SELECT * from document_hash'
            cursor = self.get_local_database().execute(query)
            results = cursor.fetchall()
            res = dict()
            for _, path, hash_ in results:
                res[path] = hash_
            self.cached_path_hash_map = res

        return self.cached_path_hash_map

    def run_command(self, command_name, text=None, focus=False):
        if text == None:
            params = [self.path, '--execute-command', command_name]
        else:
            params = [self.path, '--execute-command', command_name, '--execute-command-data', text]
        
        if focus == False:
            params.append('--nofocus')
        
        if self.is_dummy_mode:
            print('dummy mode, executing: ', params)
        else:
            subprocess.run(params)

    def goto_begining(self, focus=False):
        data = None
        self.run_command("goto_begining", data, focus=focus)

    def goto_end(self, focus=False):
        data = None
        self.run_command("goto_end", data, focus=focus)

    def goto_definition(self, focus=False):
        data = None
        self.run_command("goto_definition", data, focus=focus)

    def overview_definition(self, focus=False):
        data = None
        self.run_command("overview_definition", data, focus=focus)

    def portal_to_definition(self, focus=False):
        data = None
        self.run_command("portal_to_definition", data, focus=focus)

    def next_item(self, focus=False):
        data = None
        self.run_command("next_item", data, focus=focus)

    def previous_item(self, focus=False):
        data = None
        self.run_command("previous_item", data, focus=focus)

    def set_mark(self, symbol, focus=False):
        data = symbol
        self.run_command("set_mark", data, focus=focus)

    def goto_mark(self, symbol, focus=False):
        data = symbol
        self.run_command("goto_mark", data, focus=focus)

    def goto_page_with_page_number(self, text, focus=False):
        data = text
        self.run_command("goto_page_with_page_number", data, focus=focus)

    def search(self, text, focus=False):
        data = text
        self.run_command("search", data, focus=focus)

    def ranged_search(self, text, focus=False):
        data = text
        self.run_command("ranged_search", data, focus=focus)

    def chapter_search(self, text, focus=False):
        data = text
        self.run_command("chapter_search", data, focus=focus)

    def move_down(self, focus=False):
        data = None
        self.run_command("move_down", data, focus=focus)

    def move_up(self, focus=False):
        data = None
        self.run_command("move_up", data, focus=focus)

    def move_left(self, focus=False):
        data = None
        self.run_command("move_left", data, focus=focus)

    def move_right(self, focus=False):
        data = None
        self.run_command("move_right", data, focus=focus)

    def zoom_in(self, focus=False):
        data = None
        self.run_command("zoom_in", data, focus=focus)

    def zoom_out(self, focus=False):
        data = None
        self.run_command("zoom_out", data, focus=focus)

    def fit_to_page_width(self, focus=False):
        data = None
        self.run_command("fit_to_page_width", data, focus=focus)

    def fit_to_page_height(self, focus=False):
        data = None
        self.run_command("fit_to_page_height", data, focus=focus)

    def fit_to_page_height_smart(self, focus=False):
        data = None
        self.run_command("fit_to_page_height_smart", data, focus=focus)

    def fit_to_page_width_smart(self, focus=False):
        data = None
        self.run_command("fit_to_page_width_smart", data, focus=focus)

    def next_page(self, focus=False):
        data = None
        self.run_command("next_page", data, focus=focus)

    def previous_page(self, focus=False):
        data = None
        self.run_command("previous_page", data, focus=focus)

    def open_document(self, filename, focus=False):
        data = filename
        self.run_command("open_document", data, focus=focus)

    def debug(self, focus=False):
        data = None
        self.run_command("debug", data, focus=focus)

    def add_bookmark(self, text, focus=False):
        data = text
        self.run_command("add_bookmark", data, focus=focus)

    def add_highlight(self, symbol, focus=False):
        data = symbol
        self.run_command("add_highlight", data, focus=focus)

    def goto_toc(self, focus=False):
        data = None
        self.run_command("goto_toc", data, focus=focus)

    def goto_highlight(self, focus=False):
        data = None
        self.run_command("goto_highlight", data, focus=focus)

    def goto_bookmark(self, focus=False):
        data = None
        self.run_command("goto_bookmark", data, focus=focus)

    def goto_bookmark_g(self, focus=False):
        data = None
        self.run_command("goto_bookmark_g", data, focus=focus)

    def goto_highlight_g(self, focus=False):
        data = None
        self.run_command("goto_highlight_g", data, focus=focus)

    def goto_highlight_ranged(self, focus=False):
        data = None
        self.run_command("goto_highlight_ranged", data, focus=focus)

    def link(self, focus=False):
        data = None
        self.run_command("link", data, focus=focus)

    def portal(self, focus=False):
        data = None
        self.run_command("portal", data, focus=focus)

    def next_state(self, focus=False):
        data = None
        self.run_command("next_state", data, focus=focus)

    def prev_state(self, focus=False):
        data = None
        self.run_command("prev_state", data, focus=focus)

    def pop_state(self, focus=False):
        data = None
        self.run_command("pop_state", data, focus=focus)

    def test_command(self, focus=False):
        data = None
        self.run_command("test_command", data, focus=focus)

    def delete_link(self, focus=False):
        data = None
        self.run_command("delete_link", data, focus=focus)

    def delete_portal(self, focus=False):
        data = None
        self.run_command("delete_portal", data, focus=focus)

    def delete_bookmark(self, focus=False):
        data = None
        self.run_command("delete_bookmark", data, focus=focus)

    def delete_highlight(self, focus=False):
        data = None
        self.run_command("delete_highlight", data, focus=focus)

    def goto_link(self, focus=False):
        data = None
        self.run_command("goto_link", data, focus=focus)

    def goto_portal(self, focus=False):
        data = None
        self.run_command("goto_portal", data, focus=focus)

    def edit_link(self, focus=False):
        data = None
        self.run_command("edit_link", data, focus=focus)

    def edit_portal(self, focus=False):
        data = None
        self.run_command("edit_portal", data, focus=focus)

    def open_prev_doc(self, focus=False):
        data = None
        self.run_command("open_prev_doc", data, focus=focus)

    def open_document_embedded(self, focus=False):
        data = None
        self.run_command("open_document_embedded", data, focus=focus)

    def open_document_embedded_from_current_path(self, focus=False):
        data = None
        self.run_command("open_document_embedded_from_current_path", data, focus=focus)

    def copy(self, focus=False):
        data = None
        self.run_command("copy", data, focus=focus)

    def toggle_fullscreen(self, focus=False):
        data = None
        self.run_command("toggle_fullscreen", data, focus=focus)

    def toggle_one_window(self, focus=False):
        data = None
        self.run_command("toggle_one_window", data, focus=focus)

    def toggle_highlight(self, focus=False):
        data = None
        self.run_command("toggle_highlight", data, focus=focus)

    def toggle_synctex(self, focus=False):
        data = None
        self.run_command("toggle_synctex", data, focus=focus)

    def command(self, focus=False):
        data = None
        self.run_command("command", data, focus=focus)

    def external_search(self, symbol, focus=False):
        data = symbol
        self.run_command("external_search", data, focus=focus)

    def open_selected_url(self, focus=False):
        data = None
        self.run_command("open_selected_url", data, focus=focus)

    def screen_down(self, focus=False):
        data = None
        self.run_command("screen_down", data, focus=focus)

    def screen_up(self, focus=False):
        data = None
        self.run_command("screen_up", data, focus=focus)

    def next_chapter(self, focus=False):
        data = None
        self.run_command("next_chapter", data, focus=focus)

    def prev_chapter(self, focus=False):
        data = None
        self.run_command("prev_chapter", data, focus=focus)

    def toggle_dark_mode(self, focus=False):
        data = None
        self.run_command("toggle_dark_mode", data, focus=focus)

    def toggle_presentation_mode(self, focus=False):
        data = None
        self.run_command("toggle_presentation_mode", data, focus=focus)

    def toggle_mouse_drag_mode(self, focus=False):
        data = None
        self.run_command("toggle_mouse_drag_mode", data, focus=focus)

    def close_window(self, focus=False):
        data = None
        self.run_command("close_window", data, focus=focus)

    def quit(self, focus=False):
        data = None
        self.run_command("quit", data, focus=focus)

    def open_link(self, text, focus=False):
        data = text
        self.run_command("open_link", data, focus=focus)

    def keyboard_select(self, text, focus=False):
        data = text
        self.run_command("keyboard_select", data, focus=focus)

    def keyboard_smart_jump(self, text, focus=False):
        data = text
        self.run_command("keyboard_smart_jump", data, focus=focus)

    def keyboard_overview(self, text, focus=False):
        data = text
        self.run_command("keyboard_overview", data, focus=focus)

    def keys(self, focus=False):
        data = None
        self.run_command("keys", data, focus=focus)

    def keys_user(self, focus=False):
        data = None
        self.run_command("keys_user", data, focus=focus)

    def prefs(self, focus=False):
        data = None
        self.run_command("prefs", data, focus=focus)

    def prefs_user(self, focus=False):
        data = None
        self.run_command("prefs_user", data, focus=focus)

    def import_(self, focus=False):
        data = None
        self.run_command("import", data, focus=focus)

    def export(self, focus=False):
        data = None
        self.run_command("export", data, focus=focus)

    def enter_visual_mark_mode(self, focus=False):
        data = None
        self.run_command("enter_visual_mark_mode", data, focus=focus)

    def move_visual_mark_down(self, focus=False):
        data = None
        self.run_command("move_visual_mark_down", data, focus=focus)

    def move_visual_mark_up(self, focus=False):
        data = None
        self.run_command("move_visual_mark_up", data, focus=focus)

    def set_page_offset(self, text, focus=False):
        data = text
        self.run_command("set_page_offset", data, focus=focus)

    def toggle_visual_scroll(self, focus=False):
        data = None
        self.run_command("toggle_visual_scroll", data, focus=focus)

    def toggle_horizontal_scroll_lock(self, focus=False):
        data = None
        self.run_command("toggle_horizontal_scroll_lock", data, focus=focus)

    def toggle_custom_color(self, focus=False):
        data = None
        self.run_command("toggle_custom_color", data, focus=focus)

    def execute(self, text, focus=False):
        data = text
        self.run_command("execute", data, focus=focus)

    def execute_predefined_command(self, symbol, focus=False):
        data = symbol
        self.run_command("execute_predefined_command", data, focus=focus)

    def embed_annotations(self, focus=False):
        data = None
        self.run_command("embed_annotations", data, focus=focus)

    def copy_window_size_config(self, focus=False):
        data = None
        self.run_command("copy_window_size_config", data, focus=focus)

    def toggle_select_highlight(self, focus=False):
        data = None
        self.run_command("toggle_select_highlight", data, focus=focus)

    def set_select_highlight_type(self, symbol, focus=False):
        data = symbol
        self.run_command("set_select_highlight_type", data, focus=focus)

    def open_last_document(self, focus=False):
        data = None
        self.run_command("open_last_document", data, focus=focus)

    def toggle_window_configuration(self, focus=False):
        data = None
        self.run_command("toggle_window_configuration", data, focus=focus)

    def prefs_user_all(self, focus=False):
        data = None
        self.run_command("prefs_user_all", data, focus=focus)

    def keys_user_all(self, focus=False):
        data = None
        self.run_command("keys_user_all", data, focus=focus)

    def fit_to_page_width_ratio(self, focus=False):
        data = None
        self.run_command("fit_to_page_width_ratio", data, focus=focus)

    def smart_jump_under_cursor(self, focus=False):
        data = None
        self.run_command("smart_jump_under_cursor", data, focus=focus)

    def overview_under_cursor(self, focus=False):
        data = None
        self.run_command("overview_under_cursor", data, focus=focus)

    def close_overview(self, focus=False):
        data = None
        self.run_command("close_overview", data, focus=focus)

    def visual_mark_under_cursor(self, focus=False):
        data = None
        self.run_command("visual_mark_under_cursor", data, focus=focus)

    def close_visual_mark(self, focus=False):
        data = None
        self.run_command("close_visual_mark", data, focus=focus)

    def zoom_in_cursor(self, focus=False):
        data = None
        self.run_command("zoom_in_cursor", data, focus=focus)

    def zoom_out_cursor(self, focus=False):
        data = None
        self.run_command("zoom_out_cursor", data, focus=focus)

    def goto_left(self, focus=False):
        data = None
        self.run_command("goto_left", data, focus=focus)

    def goto_left_smart(self, focus=False):
        data = None
        self.run_command("goto_left_smart", data, focus=focus)

    def goto_right(self, focus=False):
        data = None
        self.run_command("goto_right", data, focus=focus)

    def goto_right_smart(self, focus=False):
        data = None
        self.run_command("goto_right_smart", data, focus=focus)

    def rotate_clockwise(self, focus=False):
        data = None
        self.run_command("rotate_clockwise", data, focus=focus)

    def rotate_counterclockwise(self, focus=False):
        data = None
        self.run_command("rotate_counterclockwise", data, focus=focus)

    def goto_next_highlight(self, focus=False):
        data = None
        self.run_command("goto_next_highlight", data, focus=focus)

    def goto_prev_highlight(self, focus=False):
        data = None
        self.run_command("goto_prev_highlight", data, focus=focus)

    def goto_next_highlight_of_type(self, focus=False):
        data = None
        self.run_command("goto_next_highlight_of_type", data, focus=focus)

    def goto_prev_highlight_of_type(self, focus=False):
        data = None
        self.run_command("goto_prev_highlight_of_type", data, focus=focus)

    def add_highlight_with_current_type(self, focus=False):
        data = None
        self.run_command("add_highlight_with_current_type", data, focus=focus)

    def enter_password(self, text, focus=False):
        data = text
        self.run_command("enter_password", data, focus=focus)

    def toggle_fastread(self, focus=False):
        data = None
        self.run_command("toggle_fastread", data, focus=focus)

    def goto_top_of_page(self, focus=False):
        data = None
        self.run_command("goto_top_of_page", data, focus=focus)

    def goto_bottom_of_page(self, focus=False):
        data = None
        self.run_command("goto_bottom_of_page", data, focus=focus)

    def new_window(self, focus=False):
        data = None
        self.run_command("new_window", data, focus=focus)

    def toggle_statusbar(self, focus=False):
        data = None
        self.run_command("toggle_statusbar", data, focus=focus)

    def reload(self, focus=False):
        data = None
        self.run_command("reload", data, focus=focus)

    def synctex_under_cursor(self, focus=False):
        data = None
        self.run_command("synctex_under_cursor", data, focus=focus)

    def set_status_string(self, text, focus=False):
        data = text
        self.run_command("set_status_string", data, focus=focus)

    def clear_status_string(self, focus=False):
        data = None
        self.run_command("clear_status_string", data, focus=focus)

    def toggle_titlebar(self, focus=False):
        data = None
        self.run_command("toggle_titlebar", data, focus=focus)

    def next_preview(self, focus=False):
        data = None
        self.run_command("next_preview", data, focus=focus)

    def previous_preview(self, focus=False):
        data = None
        self.run_command("previous_preview", data, focus=focus)

    def goto_overview(self, focus=False):
        data = None
        self.run_command("goto_overview", data, focus=focus)

    def portal_to_overview(self, focus=False):
        data = None
        self.run_command("portal_to_overview", data, focus=focus)

    def goto_selected_text(self, focus=False):
        data = None
        self.run_command("goto_selected_text", data, focus=focus)
    
    def get_document(self, path):
        return Document(path, self)
    
    def close(self):
        self.local_database.close()
        self.shared_database.close()

@dataclass
class DocumentPos:
    page: int
    offset_x: float
    offset_y: float

@dataclass
class AbsoluteDocumentPos:
    offset_x: float
    offset_y: float


class Highlight:

    def __init__(self, document, text, highlight_type, begin, end):
        self.doc = document
        self.text = text
        self.highlight_type = highlight_type
        self.selection_begin = begin
        self.selection_end = end
    
    def get_begin_document_pos(self):
        begin_page, begin_offset_y = self.doc.absolute_to_document_y(self.selection_begin[1])
        return DocumentPos(begin_page, self.selection_begin[0], begin_offset_y)

    def get_end_document_pos(self):
        end_page, end_offset_y = self.doc.absolute_to_document_y(self.selection_end[1])
        return DocumentPos(end_page, self.selection_end[0], end_offset_y)

    
    def __repr__(self):
        return f"Highlight of type {self.highlight_type}: {self.text}"

class Bookmark:

    def __init__(self, document, description, y_offset):
        self.doc = document
        self.description = description
        self.y_offset = y_offset
    
    @lru_cache(maxsize=None)
    def get_document_position(self):
        return self.doc.absolute_to_document_y(self.y_offset)
    

    def __repr__(self):
        return f"Bookmark at {self.y_offset}: {self.description}"

class Document:

    def __init__(self, path, sioyek):
        self.path = path
        self.doc = fitz.open(self.path)
        self.set_page_dimensions()
        self.sioyek = sioyek
        self.cached_hash = None

    def to_absolute(self, document_pos):
        offset_x = document_pos.offset_x
        offset_y = document_pos.offset_y + self.cum_page_heights[document_pos.page]
        return AbsoluteDocumentPos(offset_x, offset_y)
        

    def absolute_to_document_y(self, offset_y):
        page = 0
        while offset_y > self.page_heights[page]:
            offset_y -= self.page_heights[page]
            page += 1
        return (page, offset_y)

    def to_document(self, absolute_document_pos):
        page, offset_y = self.absolute_to_document_y(absolute_document_pos.offset_y)
        return DocumentPos(page, absolute_document_pos.offset_x, offset_y)

    
    @lru_cache(maxsize=None)
    def get_page(self, page_number):
        return self.doc.load_page(page_number)
    
    @lru_cache(maxsize=None)
    def get_page_pdf_annotations(self, page_number):
        page = self.get_page(page_number)
        res = []
        annot = page.first_annot
        while annot != None:
            res.append(annot)
            annot = annot.next

        return res

    @lru_cache(maxsize=None)
    def get_page_pdf_bookmarks(self, page_number):
        def is_bookmark(annot):
            return annot.type[1] == 'Text'
        return [annot for annot in self.get_page_pdf_annotations(page_number) if is_bookmark(annot)]

    @lru_cache(maxsize=None)
    def get_page_pdf_highlights(self, page_number):
        def is_highlight(annot):
            return annot.type[1] == 'Highlight'
        return [annot for annot in self.get_page_pdf_annotations(page_number) if is_highlight(annot)]

    def embed_highlight(self, highlight, colormap=None):
        docpos = highlight.get_begin_document_pos()
        page = self.get_page(docpos.page)
        # quads = page.search_for(highlight.text, flags=fitz.TEXT_PRESERVE_WHITESPACE, hit_max=50)
        quads = self.get_best_selection_rects(docpos.page, highlight.text, merge=True)

        annot = page.add_highlight_annot(quads)
        if colormap is not None:
            if highlight.highlight_type in colormap.keys():
                color = colormap[highlight.highlight_type]
                annot.set_colors(stroke=color, fill=color)
                annot.update()

    def embed_bookmark(self, bookmark):
        page_number, offset_y = bookmark.get_document_position()
        page = self.get_page(page_number)
        # print((0, offset_y), bookmark.description)
        page.add_text_annot((0, offset_y), bookmark.description)
    
    def embed_new_bookmarks(self):
        new_bookmarks = self.get_non_embedded_bookmarks()
        for bookmark in new_bookmarks:
            self.embed_bookmark(bookmark)
    
    def embed_new_highlights(self, colormap=None):
        new_highlights = self.get_non_embedded_highlights()
        for highlight in new_highlights:
            self.embed_highlight(highlight, colormap)
    
    def embed_new_annotations(self, save=False, colormap=None):
        self.embed_new_bookmarks()
        self.embed_new_highlights(colormap=colormap)

        if save:
            self.save_changes()
    
    def save_changes(self):
        self.doc.saveIncr()

    def get_non_embedded_highlights(self):

        candidate_highlights = self.get_highlights()
        new_highlights = []

        for highlight in candidate_highlights:
            highlight_document_pos = highlight.get_begin_document_pos()
            pdf_page_highlights = self.get_page_pdf_highlights(highlight_document_pos.page)
            found = False
            for pdf_highlight in pdf_page_highlights:
                if abs(highlight_document_pos.offset_y - pdf_highlight.rect[1]) < 50:
                    found = True
                    break
            if not found:
                new_highlights.append(highlight)
        return new_highlights

    def get_non_embedded_bookmarks(self):

        candidate_bookmarks = self.get_bookmarks()
        new_bookmarks = []

        for bookmark in candidate_bookmarks:
            pdf_page_bookmarks = self.get_page_pdf_bookmarks(bookmark.get_document_position()[0])
            found = False
            for pdf_bookmark in pdf_page_bookmarks:
                if bookmark.description == pdf_bookmark.info['content']:
                    found = True
                    break
            if not found:
                new_bookmarks.append(bookmark)
        return new_bookmarks
            
    def get_page_text_and_rects(self, page_number):
        page = self.get_page(page_number)
        word_data = page.get_text('words')

        word_texts = []
        word_rects = []
        resulting_string = ""
        string_rects = []

        for i in range(len(word_data)):
            word_text = word_data[i][4]
            block_no = word_data[i][5]
            line_no = word_data[i][6]
            if i > 0:
                if block_no != word_data[i-1][5]:
                    word_text = word_text + '\n'
            word_texts.append(word_text)
            word_rects.append(fitz.Rect(word_data[i][0:4]))

            additional_string = word_text

            if word_text[-1] != '\n':
                additional_string += ' '
            
            resulting_string += additional_string
            string_rects.extend([fitz.Rect(word_data[i][0:4])] * len(additional_string))
        
        return resulting_string, string_rects, word_texts, word_rects

    def set_page_dimensions(self):
        self.page_heights = []
        self.page_widths = []
        self.cum_page_heights = []

        cum_height = 0
        for i in range(self.doc.page_count):
            page = self.doc.load_page(i)
            width, height = page.mediabox_size
            self.page_heights.append(height)
            self.page_widths.append(width)
            self.cum_page_heights.append(cum_height)
            cum_height += height
    
    def get_best_selection_rects(self, page_number, text, merge=False):
        for i in range(10):
            rects = self.get_text_selection_rects(page_number, text, num_errors=i)
            if len(rects) > 0:
                if i > 0 and merge == True:
                    rects = merge_rects(rects)
                return rects
        return None

    def get_best_selection(self, page_number, text):
        for i in range(10):
            res = self.get_text_selection_begin_and_end(page_number, text, num_errors=i)
            if res[0][0] != None:
                return res
        return None

    def get_text_selection_rects(self, page_number, text, num_errors=0):
        if num_errors == 0:
            page = self.get_page(page_number)
            rects = page.search_for(text)
            return rects
        else:
            page_text, page_rects, _, _ = self.get_page_text_and_rects(page_number)
            match = regex.search('(' + regex.escape(text) + '){e<=' + str(num_errors) +'}', page_text)
            if match:
                match_begin, match_end = match.span()
                # print('match: ')
                # print(page_text[match_begin + 1: match_end+1])
                rects = page_rects[match_begin + 1: match_end+1]
                return list(dict.fromkeys(rects))
            else:
                return []

    def get_text_selection_begin_and_end(self, page_number, text, num_errors=0):
        rects = self.get_text_selection_rects(page_number, text, num_errors)
        if len(rects) > 0:
            begin_x, begin_y = rects[0].top_left
            end_x, end_y = rects[-1].bottom_right
            return (begin_x, begin_y), (end_x, end_y)
        else:
            return (None, None), (None, None)

    def highlight_selection(self, page_number, selection_begin, selection_end, focus=False):
        highlight_string = '{},{},{} {},{},{}'.format(
            page_number,
            selection_begin[0], selection_begin[1],
            page_number,
            selection_end[0], selection_end[1])

        self.sioyek.keyboard_select(highlight_string, focus=focus)

    def highlight_page_text(self, page_number, text, focus=False):
        (begin_x, begin_y), (end_x, end_y) = self.get_text_selection_begin_and_end(page_number, text)
        if begin_x:
            self.highlight_selection(page_number, (begin_x, begin_y), (end_x, end_y), focus=focus)

    def highlight_page_text_fault_tolerant(self, page_number, text, focus=False):
        best_selection = self.get_best_selection(page_number, text)
        if best_selection:
            self.highlight_selection(page_number, best_selection[0], best_selection[1], focus=focus)
    
    def get_sentences(self):
        res = []
        for i in range(self.doc.page_count):
            page = self.doc.load_page(i)
            sentences = page.get_text().replace('\n', '').split('.')
            res.extend([(s, i) for s in sentences])
        return res
    
    def get_hash(self):
        path_hash_map = self.sioyek.get_path_hash_map()

        for path, hash_ in path_hash_map.items():
            if os.path.normpath(self.path) == os.path.normpath(path):
                self.cached_hash = hash_
        return self.cached_hash
    
    def get_bookmarks(self):
        doc_hash = self.get_hash()
        BOOKMARK_SELECT_QUERY = "select * from bookmarks where document_path='{}'".format(doc_hash)
        shared_database = self.sioyek.get_shared_database()
        cursor = shared_database.execute(BOOKMARK_SELECT_QUERY)
        bookmarks = [Bookmark(self, desc, y_offset) for _, _, desc, y_offset in cursor.fetchall()]
        return bookmarks

    def get_highlights(self):
        doc_hash = self.get_hash()
        HIGHLIGHT_SELECT_QUERY = "select * from highlights where document_path='{}'".format(doc_hash)

        shared_database = self.sioyek.get_shared_database()
        cursor = shared_database.execute(HIGHLIGHT_SELECT_QUERY)
        highlights = [Highlight(self, text, highlight_type, (begin_x, begin_y), (end_x, end_y)) for _, _, text, highlight_type, begin_x, begin_y, end_x, end_y in cursor.fetchall()]
        return highlights
    
    def close(self):
        self.doc.close()
