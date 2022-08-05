'''
script used to generate sioyek.py file
'''
sioyek_commands = {
    "goto_begining": [False,	False,	False,	True],
    "goto_end": [False,	False,	False,	True],
    "goto_definition": [False,	False,	False,	False],
    "overview_definition": [False,	False,	False,	False],
    "portal_to_definition": [False,	False,	False,	False],
    "next_item": [False,False,	False,	True],
    "previous_item": [ False, False , False, True],
    "set_mark": [ False, True , False, False],
    "goto_mark": [ False, True , False, False],
    "goto_page_with_page_number": [ True, False , False, True],
    "search": [ True, False , False, False],
    "ranged_search": [ True, False , False, False],
    "chapter_search": [ True, False , False, False],
    "move_down": [ False, False , False, False],
    "move_up": [ False, False , False, False],
    "move_left": [ False, False , False, False],
    "move_right": [ False, False , False, False],
    "zoom_in": [ False, False , False, False],
    "zoom_out": [ False, False , False, False],
    "fit_to_page_width": [ False, False , False, False],
    "fit_to_page_height": [ False, False , False, False],
    "fit_to_page_height_smart": [ False, False , False, False],
    "fit_to_page_width_smart": [ False, False , False, False],
    "next_page": [ False, False , False, False],
    "previous_page": [ False, False , False, False],
    "open_document": [ False, False , True, True],
    "debug": [ False, False , False, False],
    "add_bookmark": [ True, False , False, False],
    "add_highlight": [ False, True , False, False],
    "goto_toc": [ False, False , False, False],
    "goto_highlight": [ False, False , False, False],
    "goto_bookmark": [ False, False , False, False],
    "goto_bookmark_g": [ False, False , False, False],
    "goto_highlight_g": [ False, False , False, False],
    "goto_highlight_ranged": [ False, False , False, False],
    "link": [ False, False , False, False],
    "portal": [ False, False , False, False],
    "next_state": [ False, False , False, False],
    "prev_state": [ False, False , False, False],
    "pop_state": [ False, False , False, False],
    "test_command": [ False, False , False, False],
    "delete_link": [ False, False , False, False],
    "delete_portal": [ False, False , False, False],
    "delete_bookmark": [ False, False , False, False],
    "delete_highlight": [ False, False , False, False],
    "goto_link": [ False, False , False, False],
    "goto_portal": [ False, False , False, False],
    "edit_link": [ False, False , False, False],
    "edit_portal": [ False, False , False, False],
    "open_prev_doc": [ False, False , False, False],
    "open_document_embedded": [ False, False , False, False],
    "open_document_embedded_from_current_path": [ False, False , False, False],
    "copy": [ False, False , False, False],
    "toggle_fullscreen": [ False, False , False, False],
    "toggle_one_window": [ False, False , False, False],
    "toggle_highlight": [ False, False , False, False],
    "toggle_synctex": [ False, False , False, False],
    "command": [ False, False , False, False],
    "external_search": [ False, True , False, False],
    "open_selected_url": [ False, False , False, False],
    "screen_down": [ False, False , False, False],
    "screen_up": [ False, False , False, False],
    "next_chapter": [ False, False , False, True],
    "prev_chapter": [ False, False , False, True],
    "toggle_dark_mode": [ False, False , False, False],
    "toggle_presentation_mode": [ False, False , False, False],
    "toggle_mouse_drag_mode": [ False, False , False, False],
    "close_window": [ False, False , False, False],
    "quit": [ False, False , False, False],
    "q": [ False, False , False, False],
    "open_link": [ True, False , False, False],
    "keyboard_select": [ True, False , False, False],
    "keyboard_smart_jump": [ True, False , False, False],
    "keyboard_overview": [ True, False , False, False],
    "keys": [ False, False , False, False],
    "keys_user": [ False, False , False, False],
    "prefs": [ False, False , False, False],
    "prefs_user": [ False, False , False, False],
    "import": [ False, False , False, False],
    "export": [ False, False , False, False],
    "enter_visual_mark_mode": [ False, False , False, False],
    "move_visual_mark_down": [ False, False , False, False],
    "move_visual_mark_up": [ False, False , False, False],
    "set_page_offset": [ True, False , False, False],
    "toggle_visual_scroll": [ False, False , False, False],
    "toggle_horizontal_scroll_lock": [ False, False , False, False],
    "toggle_custom_color": [ False, False , False, False],
    "execute": [ True, False , False, False],
    "execute_predefined_command": [ False, True, False, False],
    "embed_annotations": [ False, False, False, False],
    "copy_window_size_config": [ False, False, False, False],
    "toggle_select_highlight": [ False, False, False, False],
    "set_select_highlight_type": [ False, True, False, False],
    "open_last_document": [ False, False, False, False],
    "toggle_window_configuration": [ False, False, False, False],
    "prefs_user_all": [ False, False, False, False],
    "keys_user_all": [ False, False, False, False],
    "fit_to_page_width_ratio": [ False, False, False, False],
    "smart_jump_under_cursor": [ False, False, False, False],
    "overview_under_cursor": [ False, False, False, False],
    "close_overview": [ False, False, False, False],
    "visual_mark_under_cursor": [ False, False, False, False],
    "close_visual_mark": [ False, False, False, False],
    "zoom_in_cursor": [ False, False, False, False],
    "zoom_out_cursor": [ False, False, False, False],
    "goto_left": [ False, False, False, False],
    "goto_left_smart": [ False, False, False, False],
    "goto_right": [ False, False, False, False],
    "goto_right_smart": [ False, False, False, False],
    "rotate_clockwise": [ False, False, False, False],
    "rotate_counterclockwise": [ False, False, False, False],
    "goto_next_highlight": [ False, False, False, False],
    "goto_prev_highlight": [ False, False, False, False],
    "goto_next_highlight_of_type": [ False, False, False, False],
    "goto_prev_highlight_of_type": [ False, False, False, False],
    "add_highlight_with_current_type": [ False, False, False, False],
    "enter_password": [ True, False , False, False],
    "toggle_fastread": [ False, False , False, False],
    "goto_top_of_page": [ False, False , False, False],
    "goto_bottom_of_page": [ False, False , False, False],
    "new_window": [ False, False , False, False],
    "toggle_statusbar": [ False, False , False, False],
    "reload": [ False, False , False, False],
    "synctex_under_cursor": [ False, False , False, False],
    "set_status_string": [ True, False , False, False],
    "clear_status_string": [ False, False , False, False],
    "toggle_titlebar": [ False, False , False, False],
    "next_preview": [ False, False , False, False],
    "previous_preview": [ False, False , False, False],
    "goto_overview": [ False, False , False, False],
    "portal_to_overview": [ False, False , False, False],
    }

def print_method(command_name, requires_text, requires_symbol, requires_filename):
    res = ''
    res += 'def {}(self'.format(command_name)
    if requires_text:
        res += ', text'
    elif requires_symbol:
        res += ', symbol'
    elif requires_filename:
        res += ', filename'
    res += ', focus=False):\n'
    if requires_text:
        res += ' ' * 4 + 'data = text\n'
    elif requires_symbol:
        res += ' ' * 4 + 'data = symbol\n'
    elif requires_filename:
        res += ' ' * 4 + 'data = filename\n'
    else:
        res += ' ' * 4 + 'data = None\n'

    
    res += ' ' * 4 + 'self.run_command("{}", data, focus=focus)\n'.format(command_name)
    print(res)

if __name__ == '__main__':
    for command_name, (requires_text, requires_symbol, requires_filename, _) in sioyek_commands.items():
        print_method(command_name, requires_text, requires_symbol, requires_filename)
