import subprocess

class Sioyek:

    def __init__(self, sioyek_path):
        self.path = sioyek_path
    
    def run_command(self, command_name, text=None, focus=False):
        if text == None:
            params = [self.path, '--execute-command', command_name]
        else:
            params = [self.path, '--execute-command', command_name, '--execute-command-data', text]
        
        if focus == False:
            params.append('--nofocus')
        
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
