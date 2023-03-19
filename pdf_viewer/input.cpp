#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <qkeyevent.h>
#include <qstring.h>
#include <qstringlist.h>
#include "input.h"
#include "main_widget.h"
#include "ui.h"

extern bool SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE;
extern bool USE_LEGACY_KEYBINDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_COMMANDS;
extern std::map<std::wstring, std::wstring> ADDITIONAL_MACROS;
extern std::wstring SEARCH_URLS[26];
extern bool ALPHABETIC_LINK_TAGS;

extern Path default_config_path;
extern Path default_keys_path;
extern std::vector<Path> user_config_paths;
extern std::vector<Path> user_keys_paths;

class SymbolCommand : public Command {
protected:
	char symbol = 0;
public:
	virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
		if (symbol == 0) {
			return Requirement{ RequirementType::Symbol, "symbol" };
		}
		else {
			return {};
		}
	}

	virtual void set_symbol_requirement(char value) {
		this->symbol = value;
	}
};

class TextCommand : public Command {
protected:
	std::optional<std::wstring> text = {};
public:
	
	virtual std::string text_requirement_name() {
		return "text";
	}

	virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
		if (text.has_value()) {
			return {};
		}
		else {
			return Requirement{ RequirementType::Text, text_requirement_name() };
		}
	}

	virtual void set_text_requirement(std::wstring value) {
		this->text = value;
	}
};

class GotoMark : public SymbolCommand {
	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "goto_mark";
	}

	std::vector<char> special_symbols() {
		std::vector<char> res = { '`', '\'', '/' };
		return res;
	}

	void perform(MainWidget* widget) {
		assert(this->symbol != 0);
		widget->goto_mark(this->symbol);
	}

	bool requires_document() { return false; }
};

class SetMark : public SymbolCommand {

	std::string get_name() {
		return "set_mark";
	}

	void perform(MainWidget* widget) {
		assert(this->symbol != 0);
		widget->set_mark_in_current_location(this->symbol);
	}
};

class NextItemCommand : public Command{
	void perform(MainWidget* widget) {
		if (num_repeats == 0) num_repeats++;
		widget->opengl_widget->goto_search_result(num_repeats);
	}

	std::string get_name() {
		return "next_item";
	}
};

class PrevItemCommand : public Command{

	void perform(MainWidget* widget) {
		if (num_repeats == 0) num_repeats++;
		widget->opengl_widget->goto_search_result(-num_repeats);
	}

	std::string get_name() {
		return "previous_item";
	}
};

class SearchCommand : public TextCommand {

	void perform(MainWidget* widget) {
		widget->perform_search(this->text.value(), false);
	}

	std::string get_name() {
		return "search";
	}

	bool pushes_state() {
		return true;
	}

	std::string text_requirement_name() {
		return "Search Term";
	}
};

class ChapterSearchCommand : public TextCommand {

	void perform(MainWidget* widget) {
		widget->perform_search(this->text.value(), false);
	}

	void pre_perform(MainWidget* widget) {
		std::optional<std::pair<int, int>> chapter_range = widget->main_document_view->get_current_page_range();
		if (chapter_range) {
			std::stringstream search_range_string;
			search_range_string << "<" << chapter_range.value().first << "," << chapter_range.value().second << ">";
			widget->text_command_line_edit->setText(search_range_string.str().c_str() + widget->text_command_line_edit->text());
		}

	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "chapter_search";
	}

	std::string text_requirement_name() {
		return "Search Term";
	}
};

class RegexSearchCommand : public TextCommand {

	void perform(MainWidget* widget) {
		widget->perform_search(this->text.value(), true);
	}

	std::string get_name() {
		return "regex_search";
	}

	bool pushes_state() {
		return true;
	}

	std::string text_requirement_name() {
		return "regex";
	}
};

class AddBookmarkCommand : public TextCommand {
	void perform(MainWidget* widget) {
		widget->main_document_view->add_bookmark(text.value());
	}

	std::string get_name() {
		return "add_bookmark";
	}

	std::string text_requirement_name() {
		return "Bookmark Text";
	}
};

class GotoBookmarkCommand : public Command {
	void perform(MainWidget* widget) {
		widget->handle_goto_bookmark();
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "goto_bookmark";
	}
};

class GotoBookmarkGlobalCommand : public Command {
	void perform(MainWidget* widget) {
		widget->handle_goto_bookmark_global();
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "goto_bookmark_g";
	}
	bool requires_document() { return false; }
};

class GotoHighlightCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_goto_highlight();
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "goto_highlight";
	}
};

class GotoHighlightGlobalCommand : public Command {
	void perform(MainWidget* widget) {
		widget->handle_goto_highlight_global();
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "goto_highlight_g";
	}
	bool requires_document() { return false; }
};

class GotoTableOfContentsCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_goto_toc();
	}

	std::string get_name() {
		return "goto_toc";
	}
};

class PortalCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_portal();
	}

	std::string get_name() {
		return "portal";
	}
};

class ToggleWindowConfigurationCommand : public Command {
	void perform(MainWidget* widget) {
		widget->toggle_window_configuration();
	}

	std::string get_name() {
		return "toggle_window_configuration";
	}
	bool requires_document() { return false; }
};

class NextStateCommand : public Command {

	void perform(MainWidget* widget) {
		widget->next_state();
	}

	std::string get_name() {
		return "next_state";
	}
	bool requires_document() { return false; }
};

class PrevStateCommand : public Command {

	void perform(MainWidget* widget) {
		widget->prev_state();
	}

	std::string get_name() {
		return "prev_state";
	}
	bool requires_document() { return false; }
};

class AddHighlightCommand : public SymbolCommand {

	void perform(MainWidget* widget) {
		widget->handle_add_highlight(symbol);
	}

	std::string get_name() {
		return "add_highlight";
	}
};

class CommandCommand : public Command {

	void perform(MainWidget* widget) {
		QStringList command_names = widget->command_manager->get_all_command_names();
		widget->set_current_widget(new CommandSelector(
			&widget->on_command_done, widget, command_names, widget->input_handler->get_command_key_mappings()));
		widget->current_widget->show();
	}

	std::string get_name() {
		return "command";
	}
	bool requires_document() { return false; }
};

class OpenDocumentCommand : public Command {

	std::wstring file_name;

	bool pushes_state() {
		return true;
	}

	std::optional<Requirement> next_requirement(MainWidget* widget) {
		if (file_name.size() == 0) {
			return Requirement{ RequirementType::File, "File" };
		}
		else {
			return {};
		}
	}

	void set_file_requirement(std::wstring value) {
		file_name = value;
	}

	void perform(MainWidget* widget) {
		widget->open_document(file_name);
	}

	std::string get_name() {
		return "open_document";
	}

	bool requires_document() { return false; }
};

class MoveDownCommand : public Command {

	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_vertical_move(rp);
	}

	std::string get_name() {
		return "move_down";
	}

};

class MoveUpCommand : public Command {

	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_vertical_move(-rp);
	}

	std::string get_name() {

		return "move_up";
	}
};

class MoveLeftCommand : public Command {
	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_horizontal_move(-rp);
	}
	std::string get_name() {

		return "move_left";
	}
};

class MoveRightCommand : public Command {
	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_horizontal_move(rp);
	}
	std::string get_name() {

		return "move_right";
	}
};

class ZoomInCommand : public Command {
	void perform(MainWidget* widget) {
		widget->main_document_view->zoom_in();
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "zoom_in";
	}
};

class FitToPageWidthCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->fit_to_page_width();
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "fit_to_page_width";
	}
};

class FitToPageWidthSmartCommand : public Command {
	void perform(MainWidget* widget) {
		widget->main_document_view->fit_to_page_width(true);
		int current_page = widget->get_current_page_number();
		widget->last_smart_fit_page = current_page;
	}

	std::string get_name() {
		return "fit_to_page_width_smart";
	}
};

class FitToPageHeightCommand : public Command {
	void perform(MainWidget* widget) {
		widget->main_document_view->fit_to_page_height();
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "fit_to_page_height";
	}
};

class FitToPageHeightSmartCommand : public Command {
	void perform(MainWidget* widget) {
		widget->main_document_view->fit_to_page_height(true);
	}

	std::string get_name() {
		return "fit_to_page_height_smart";
	}
};

class NextPageCommand : public Command {
	void perform(MainWidget* widget) {
		widget->main_document_view->move_pages(1 + num_repeats);
	}
	std::string get_name() {
		return "next_page";
	}
};

class PreviousPageCommand : public Command {
	void perform(MainWidget* widget) {
		widget->main_document_view->move_pages(-1 - num_repeats);
	}

	std::string get_name() {
		return "previous_page";
	}
};

class ZoomOutCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->zoom_out();
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "zoom_out";
	}
};

class GotoDefinitionCommand : public Command {
	void perform(MainWidget* widget) {
		if (widget->main_document_view->goto_definition()) {
			widget->opengl_widget->set_should_draw_vertical_line(false);
		}
	}

	std::string get_name() {
		return "goto_definition";
	}

	bool pushes_state() {
		return true;
	}
};

class OverviewDefinitionCommand : public Command {
	void perform(MainWidget* widget) {
		widget->overview_to_definition();
	}

	std::string get_name() {
		return "overview_definition";
	}
};

class PortalToDefinitionCommand : public Command {
	void perform(MainWidget* widget) {
		widget->portal_to_definition();
	}

	std::string get_name() {
		return "portak_to_definition";
	}
};

class MoveVisualMarkDownCommand : public Command {
	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->move_visual_mark_command(rp);
	}

	std::string get_name() {
		return "move_visual_mark_down";
	}
};

class MoveVisualMarkUpCommand : public Command {
	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->move_visual_mark_command(-rp);
	}

	std::string get_name() {
		return "move_visual_mark_up";
	}
};

class GotoPageWithPageNumberCommand : public TextCommand {

	void perform(MainWidget* widget) {
		std::wstring text_ = text.value();
		if (is_string_numeric(text_.c_str()) && text_.size() < 6) { // make sure the page number is valid
			int dest = std::stoi(text_.c_str()) - 1;
			widget->main_document_view->goto_page(dest + widget->main_document_view->get_page_offset());
		}
	}

	std::string get_name() {
		return "goto_page_with_page_number";
	}

	std::string text_requirement_name() {
		return "Page Number";
	}

	bool pushes_state() {
		return true;
	}
};

class DeletePortalCommand : public Command {
	void perform(MainWidget* widget) {
		widget->main_document_view->delete_closest_portal();
		widget->validate_render();
	}

	std::string get_name() {
		return "delete_portal";
	}
};

class DeleteBookmarkCommand : public Command {
	void perform(MainWidget* widget) {
		widget->main_document_view->delete_closest_bookmark();
		widget->validate_render();
	}

	std::string get_name() {
		return "delete_bookmark";
	}
};

class DeleteHighlightCommand : public Command {
	void perform(MainWidget* widget) {
		if (widget->selected_highlight_index != -1) {
			widget->main_document_view->delete_highlight_with_index(widget->selected_highlight_index);
			widget->selected_highlight_index = -1;
		}
		widget->validate_render();
	}

	std::string get_name() {
		return "delete_highlight";
	}
};

class GotoPortalCommand : public Command {
	void perform(MainWidget* widget) {
		std::optional<Portal> link = widget->main_document_view->find_closest_portal();
		if (link) {
			widget->open_document(link->dst);
		}
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "goto_link";
	}
};

class EditPortalCommand : public Command {
	void perform(MainWidget* widget) {
		std::optional<Portal> link = widget->main_document_view->find_closest_portal();
		if (link) {
			widget->link_to_edit = link;
			widget->open_document(link->dst);
		}
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "edit_portal";
	}
};

class OpenPrevDocCommand : public Command {
	void perform(MainWidget* widget) {
		widget->handle_open_prev_doc();
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "open_prev_doc";
	}

	bool requires_document() { return false; }
};

class OpenDocumentEmbeddedCommand : public Command {
	void perform(MainWidget* widget) {
		widget->set_current_widget(new FileSelector(
			[widget](std::wstring doc_path) {
				widget->validate_render();
				widget->open_document(doc_path);
			}, widget, ""));
		widget->current_widget->show();
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "open_document_embedded";
	}

	bool requires_document() { return false; }
};

class OpenDocumentEmbeddedFromCurrentPathCommand : public Command {
	void perform(MainWidget* widget) {
		std::wstring last_file_name = widget->get_current_file_name().value_or(L"");

		widget->set_current_widget(new FileSelector(
			[widget](std::wstring doc_path) {
				widget->validate_render();
				widget->open_document(doc_path);
			}, widget, QString::fromStdWString(last_file_name)));
		widget->current_widget->show();
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "open_document_embedded_from_current_path";
	}

	bool requires_document() { return false; }
};

class CopyCommand : public Command {
	void perform(MainWidget* widget) {
		copy_to_clipboard(widget->selected_text);
	}

	std::string get_name() {
		return "copy";
	}
};

class GotoBeginningCommand : public Command {
public:
	void perform(MainWidget* main_widget) {
		if (num_repeats) {
			main_widget->main_document_view->goto_page(num_repeats - 1 + main_widget->main_document_view->get_page_offset());
		}
		else {
			main_widget->main_document_view->set_offset_y(0.0f);
		}
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "goto_beginning";
	}
};

class GotoEndCommand : public Command {
public:
	void perform(MainWidget* main_widget) {
		if (num_repeats > 0) {
			main_widget->main_document_view->goto_page(num_repeats - 1 + main_widget->main_document_view->get_page_offset());
		}
		else {
			main_widget->main_document_view->goto_end();
		}
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "goto_end";
	}
};

class ToggleFullscreenCommand : public Command {
	void perform(MainWidget* widget) {
		widget->toggle_fullscreen();
	}
	std::string get_name() {
		return "toggle_fullscreen";
	}

	bool requires_document() { return false; }
};

class ToggleOneWindowCommand : public Command {
	void perform(MainWidget* widget) {
		widget->toggle_two_window_mode();
	}
	std::string get_name() {
		return "toggle_one_window";
	}

	bool requires_document() { return false; }
};

class ToggleHighlightCommand : public Command {
	void perform(MainWidget* widget) {
		widget->opengl_widget->toggle_highlight_links();
	}
	std::string get_name() {
		return "toggle_highlight";
	}

	bool requires_document() { return false; }
};

class ToggleSynctexCommand : public Command {
	void perform(MainWidget* widget) {
		widget->toggle_synctex_mode();
	}
	std::string get_name() {
		return "toggle_synctex";
	}

	bool requires_document() { return false; }
};

class TurnOnSynctexCommand : public Command {
	void perform(MainWidget* widget) {
		widget->set_synctex_mode(true);
	}
	std::string get_name() {
		return "turn_on_synctex";
	}

	bool requires_document() { return false; }
};

class ToggleShowLastCommand : public Command {
	void perform(MainWidget* widget) {
		widget->should_show_last_command = !widget->should_show_last_command;
	}
	std::string get_name() {
		return "toggle_show_last_command";
	}
};

class ExternalSearchCommand : public SymbolCommand {
	void perform(MainWidget* widget) {
		if ((symbol >= 'a') && (symbol <= 'z')) {
			if (SEARCH_URLS[symbol - 'a'].size() > 0) {
				search_custom_engine(widget->selected_text, SEARCH_URLS[symbol - 'a']);
			}
			else {
				std::wcout << L"No search engine defined for symbol " << symbol << std::endl;
			}
		}
	}

	std::string get_name() {
		return "external_search";
	}
};

class OpenSelectedUrlCommand : public Command {
	void perform(MainWidget* widget) {
		open_web_url((widget->selected_text).c_str());
	}
	std::string get_name() {
		return "open_selected_url";
	}
};

class ScreenDownCommand : public Command {

	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_move_screen(rp);
	}

	std::string get_name() {
		return "screen_down";
	}
};

class ScreenUpCommand : public Command {

	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_move_screen(-rp);
	}

	std::string get_name() {
		return "screen_up";
	}
};

class NextChapterCommand : public Command {

	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->main_document_view->goto_chapter(rp);
	}

	std::string get_name() {
		return "next_chapter";
	}
};

class PrevChapterCommand : public Command {

	void perform(MainWidget* widget) {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->main_document_view->goto_chapter(-rp);
	}

	std::string get_name() {
		return "prev_chapter";
	}
};

class ToggleDarkModeCommand : public Command {

	void perform(MainWidget* widget) {
		widget->opengl_widget->toggle_dark_mode();
		widget->helper_opengl_widget->toggle_dark_mode();
	}

	std::string get_name() {
		return "toggle_dark_mode";
	}

	bool requires_document() { return false; }
};

class ToggleCustomColorMode : public Command {

	void perform(MainWidget* widget) {
		widget->opengl_widget->toggle_custom_color_mode();
		widget->helper_opengl_widget->toggle_custom_color_mode();
	}

	std::string get_name() {
		return "toggle_custom_color";
	}

	bool requires_document() { return false; }
};

class TogglePresentationModeCommand : public Command {

	void perform(MainWidget* widget) {
		widget->toggle_presentation_mode();
	}

	std::string get_name() {
		return "toggle_presentation_mode";
	}

	bool requires_document() { return false; }
};

class TurnOnPresentationModeCommand : public Command {

	void perform(MainWidget* widget) {
		widget->set_presentation_mode(true);
	}

	std::string get_name() {
		return "turn_on_presentation_mode";
	}

	bool requires_document() { return false; }
};

class ToggleMouseDragMode : public Command {

	void perform(MainWidget* widget) {
		widget->toggle_mouse_drag_mode();
	}

	std::string get_name() {
		return "toggle_mouse_drag_mode";
	}

	bool requires_document() { return false; }
};

class CloseWindowCommand : public Command {

	void perform(MainWidget* widget) {
		widget->close();
	}

	std::string get_name() {
		return "close_window";
	}

	bool requires_document() { return false; }
};

class NewWindowCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_new_window();
	}

	std::string get_name() {
		return "new_window";
	}

	bool requires_document() { return false; }
};

class QuitCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_close_event();
		QApplication::quit();
	}

	std::string get_name() {
		return "quit";
	}

	bool requires_document() { return false; }
};

class EscapeCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_escape();
	}

	std::string get_name() {
		return "escape";
	}

	bool requires_document() { return false; }
};

class OpenLinkCommand : public Command {
protected:
	std::optional<std::wstring> text = {};
public:
	
	virtual std::string text_requirement_name() {
		return "Label";
	}

	virtual std::optional<Requirement> next_requirement(MainWidget* widget) {
		if (text.has_value()) {
			return {};
		}
		else {
			if ((widget->num_visible_links() < 26) && ALPHABETIC_LINK_TAGS) {
				return Requirement{ RequirementType::Symbol, "Label"};
			}
			else if ((widget->num_visible_links() < 10) && (!ALPHABETIC_LINK_TAGS)) {
				return Requirement{ RequirementType::Symbol, "Label"};
			}
			else {
				return Requirement{ RequirementType::Text, text_requirement_name() };
			}
		}
	}

	virtual void perform(MainWidget* widget) {
		widget->handle_open_link(text.value());
	}

	void pre_perform(MainWidget* widget) {
		widget->opengl_widget->set_highlight_links(true, true);

	}

	virtual std::string get_name() {
		return "open_link";
	}

	virtual void set_text_requirement(std::wstring value) {
		this->text = value;
	}

	virtual void set_symbol_requirement(char value){
		std::wstring val;
		val.push_back(value);
		this->text = val;
	}
};

class OverviewLinkCommand : public OpenLinkCommand {

	void perform(MainWidget* widget) {
		widget->handle_overview_link(text.value());
	}

	std::string get_name() {
		return "overview_link";
	}

};

class PortalToLinkCommand : public OpenLinkCommand {

	void perform(MainWidget* widget) {
		widget->handle_portal_to_link(text.value());
	}

	std::string get_name() {
		return "portal_to_link";
	}

};

class CopyLinkCommand : public TextCommand {

	void perform(MainWidget* widget) {
		widget->handle_open_link(text.value(), true);
	}

	void pre_perform(MainWidget* widget) {
		widget->opengl_widget->set_highlight_links(true, true);

	}

	std::string get_name() {
		return "copy_link";
	}

	std::string text_requirement_name() {
		return "Label";
	}
};

class KeyboardSelectCommand : public TextCommand {

	void perform(MainWidget* widget) {
		widget->handle_keyboard_select(text.value());
	}

	void pre_perform(MainWidget* widget) {
		widget->highlight_words();

	}

	std::string get_name() {
		return "keyboard_select";
	}

	std::string text_requirement_name() {
		return "Labels";
	}
};

class KeyboardOverviewCommand : public TextCommand {

	void perform(MainWidget* widget) {
		std::optional<fz_irect> rect_ = widget->get_tag_window_rect(utf8_encode(text.value()));
       if (rect_) {
           fz_irect rect = rect_.value();
           widget->overview_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
           widget->opengl_widget->set_should_highlight_words(false);
       }
	}

	void pre_perform(MainWidget* widget) {
		widget->highlight_words();

	}

	std::string get_name() {
		return "keyboard_overview";
	}

	std::string text_requirement_name() {
		return "Label";
	}
};

class KeyboardSmartjumpCommand : public TextCommand {

	void perform(MainWidget* widget) {
	   	std::optional<fz_irect> rect_ = widget->get_tag_window_rect(utf8_encode(text.value()));
       if (rect_) {
           fz_irect rect = rect_.value();
   		widget->smart_jump_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
   		widget->opengl_widget->set_should_highlight_words(false);
       }
	}

	void pre_perform(MainWidget* widget) {
		widget->highlight_words();
	}

	bool pushes_state() {
		return true;
	}

	std::string get_name() {
		return "keyboard_smart_jump";
	}

	std::string text_requirement_name() {
		return "Label";
	}
};

class KeysCommand : public Command {

	void perform(MainWidget* widget) {
		open_file(default_keys_path.get_path());
	}

	std::string get_name() {
		return "keys";
	}

	bool requires_document() { return false; }
};

class KeysUserCommand : public Command {

	void perform(MainWidget* widget) {
		std::optional<Path> key_file_path = widget->input_handler->get_or_create_user_keys_path();
		if (key_file_path) {
			open_file(key_file_path.value().get_path());
		}
	}

	std::string get_name() {
		return "keys_user";
	}

	bool requires_document() { return false; }
};

class KeysUserAllCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_keys_user_all();
	}

	std::string get_name() {
		return "keys_user_all";
	}

	bool requires_document() { return false; }
};

class PrefsCommand : public Command {

	void perform(MainWidget* widget) {
		open_file(default_config_path.get_path());
	}

	std::string get_name() {
		return "prefs";
	}

	bool requires_document() { return false; }
};

class PrefsUserCommand : public Command {

	void perform(MainWidget* widget) {
		std::optional<Path> pref_file_path = widget->config_manager->get_or_create_user_config_file();
		if (pref_file_path) {
			open_file(pref_file_path.value().get_path());
		}
	}

	std::string get_name() {
		return "prefs_user";
	}

	bool requires_document() { return false; }
};

class PrefsUserAllCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_prefs_user_all();
	}

	std::string get_name() {
		return "prefs_user_all";
	}

	bool requires_document() { return false; }
};

class FitToPageWidthRatioCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->fit_to_page_width(false, true);
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "fit_to_page_width_ratio";
	}
};

class SmartJumpUnderCursorCommand : public Command {

	void perform(MainWidget* widget) {
        QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->smart_jump_under_pos({ mouse_pos.x(), mouse_pos.y() });
	}

	std::string get_name() {
		return "smart_jump_under_cursor";
	}
};

class OverviewUnderCursorCommand : public Command {

	void perform(MainWidget* widget) {
        QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->overview_under_pos({ mouse_pos.x(), mouse_pos.y() });
	}

	std::string get_name() {
		return "overview_under_cursor";
	}
};

class SynctexUnderCursorCommand : public Command {

	void perform(MainWidget* widget) {
        QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->synctex_under_pos({ mouse_pos.x(), mouse_pos.y() });
	}

	std::string get_name() {
		return "synctex_under_cursor";
	}
};

class VisualMarkUnderCursorCommand : public Command {

	void perform(MainWidget* widget) {
        QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->visual_mark_under_pos({ mouse_pos.x(), mouse_pos.y() });
	}

	std::string get_name() {
		return "visual_mark_under_cursor";
	}
};

class CloseOverviewCommand : public Command {

	void perform(MainWidget* widget) {
        widget->opengl_widget->set_overview_page({});
	}

	std::string get_name() {
		return "close_overview";
	}
};

class CloseVisualMarkCommand : public Command {

	void perform(MainWidget* widget) {
        widget->opengl_widget->set_should_draw_vertical_line(false);
	}

	std::string get_name() {
		return "close_visual_mark";
	}
};

class ZoomInCursorCommand : public Command {

	void perform(MainWidget* widget) {
		QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->main_document_view->zoom_in_cursor({ mouse_pos.x(), mouse_pos.y() });
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "zoom_in_cursor";
	}
};

class ZoomOutCursorCommand : public Command {

	void perform(MainWidget* widget) {
		QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->main_document_view->zoom_out_cursor({ mouse_pos.x(), mouse_pos.y() });
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "zoom_out_cursor";
	}
};

class GotoLeftCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->goto_left();
	}

	std::string get_name() {
		return "goto_left";
	}
};

class GotoLeftSmartCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->goto_left_smart();
	}

	std::string get_name() {
		return "goto_left_smart";
	}
};

class GotoRightCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->goto_right();
	}

	std::string get_name() {
		return "goto_right";
	}
};

class GotoRightSmartCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->goto_right_smart();
	}

	std::string get_name() {
		return "goto_right_smart";
	}
};

class RotateClockwiseCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->rotate();
        widget->opengl_widget->rotate_clockwise();
	}

	std::string get_name() {
		return "rotate_clockwise";
	}
};

class RotateCounterClockwiseCommand : public Command {

	void perform(MainWidget* widget) {
		widget->main_document_view->rotate();
        widget->opengl_widget->rotate_counterclockwise();
	}

	std::string get_name() {
		return "rotate_counterclockwise";
	}
};

class GotoNextHighlightCommand : public Command {

	void perform(MainWidget* widget) {
		auto next_highlight = widget->main_document_view->get_document()->get_next_highlight(widget->main_document_view->get_offset_y());
        if (next_highlight.has_value()) {
			widget->long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
	}

	std::string get_name() {
		return "goto_next_highlight";
	}
};

class GotoPrevHighlightCommand : public Command {

	void perform(MainWidget* widget) {

		auto prev_highlight = widget->main_document_view->get_document()->get_prev_highlight(widget->main_document_view->get_offset_y());
        if (prev_highlight.has_value()) {
			widget->long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
	}

	std::string get_name() {
		return "goto_prev_highlight";
	}
};

class GotoNextHighlightOfTypeCommand : public Command {

	void perform(MainWidget* widget) {
		auto next_highlight = widget->main_document_view->get_document()->get_next_highlight(widget->main_document_view->get_offset_y(), widget->select_highlight_type);
        if (next_highlight.has_value()) {
			widget->long_jump_to_destination(next_highlight.value().selection_begin.y);
        }
	}

	std::string get_name() {
		return "goto_next_highlight_of_type";
	}
};

class GotoPrevHighlightOfTypeCommand : public Command {

	void perform(MainWidget* widget) {
		auto prev_highlight = widget->main_document_view->get_document()->get_prev_highlight(widget->main_document_view->get_offset_y(), widget->select_highlight_type);
        if (prev_highlight.has_value()) {
			widget->long_jump_to_destination(prev_highlight.value().selection_begin.y);
        }
	}

	std::string get_name() {
		return "goto_prev_highlight_of_type";
	}
};

class SetSelectHighlightTypeCommand : public SymbolCommand {
	void perform(MainWidget* widget) {
        widget->select_highlight_type = symbol;
	}

	std::string get_name() {
		return "set_select_highlight_type";
	}

	bool requires_document() { return false; }
};

class AddHighlightWithCurrentTypeCommand : public Command {
	void perform(MainWidget* widget) {
        if (widget->main_document_view->selected_character_rects.size() > 0) {
            widget->main_document_view->add_highlight(widget->selection_begin, widget->selection_end, widget->select_highlight_type);
            widget->main_document_view->selected_character_rects.clear();
            widget->selected_text.clear();
        }
	}

	std::string get_name() {
		return "add_highlight_with_current_type";
	}
};


class EnterPasswordCommand : public TextCommand {
	void perform(MainWidget* widget) {
		std::string password = utf8_encode(text.value());
		widget->pdf_renderer->add_password(widget->main_document_view->get_document()->get_path(), password);
	}

	std::string get_name() {
		return "enter_password";
	}

	std::string text_requirement_name() {
		return "Password";
	}
};

class ToggleFastreadCommand : public Command {
	void perform(MainWidget* widget) {
		widget->opengl_widget->toggle_fastread_mode();
	}

	std::string get_name() {
		return "toggle_fastread";
	}
};

class GotoTopOfPageCommand : public Command {
	void perform(MainWidget* widget) {
        widget->main_document_view->goto_top_of_page();
	}

	std::string get_name() {
		return "goto_top_of_page";
	}
};

class GotoBottomOfPageCommand : public Command {
	void perform(MainWidget* widget) {
        widget->main_document_view->goto_bottom_of_page();
	}

	std::string get_name() {
		return "goto_bottom_of_page";
	}
};

class ReloadCommand : public Command {
	void perform(MainWidget* widget) {
        widget->reload();
	}

	std::string get_name() {
		return "relaod";
	}
};

class ReloadConfigCommand : public Command {
	void perform(MainWidget* widget) {
		widget->on_config_file_changed(widget->config_manager);
	}

	std::string get_name() {
		return "relaod_config";
	}

	bool requires_document() { return false; }
};

class SetStatusStringCommand : public TextCommand {

	void perform(MainWidget* widget) {
		widget->set_status_message(text.value());
	}

	std::string get_name() {
		return "set_status_string";
	}

	std::string text_requirement_name() {
		return "Status String";
	}

	bool requires_document() { return false; }
};

class ClearStatusStringCommand : public Command {
	void perform(MainWidget* widget) {
        widget->set_status_message(L"");
	}

	std::string get_name() {
		return "clear_status_string";
	}

	bool requires_document() { return false; }
};

class ToggleTittlebarCommand : public Command {
	void perform(MainWidget* widget) {
		widget->toggle_titlebar();
	}

	std::string get_name() {
		return "toggle_titlebar";
	}

	bool requires_document() { return false; }
};

class NextPreviewCommand : public Command {
	void perform(MainWidget* widget) {
        if (widget->smart_view_candidates.size() > 0) {
            widget->index_into_candidates = (widget->index_into_candidates + 1) % widget->smart_view_candidates.size();
            widget->set_overview_position(widget->smart_view_candidates[widget->index_into_candidates].page, widget->smart_view_candidates[widget->index_into_candidates].y);
        }
	}

	std::string get_name() {
		return "next_preview";
	}
};

class PreviousPreviewCommand : public Command {
	void perform(MainWidget* widget) {
        if (widget->smart_view_candidates.size() > 0) {
            widget->index_into_candidates = mod(widget->index_into_candidates - 1, widget->smart_view_candidates.size());
            widget->set_overview_position(widget->smart_view_candidates[widget->index_into_candidates].page, widget->smart_view_candidates[widget->index_into_candidates].y);
        }
	}

	std::string get_name() {
		return "previous_preview";
	}
};

class GotoOverviewCommand : public Command {
	void perform(MainWidget* widget) {
		widget->goto_overview();
	}

	std::string get_name() {
		return "goto_overview";
	}
};

class PortalToOverviewCommand : public Command {
	void perform(MainWidget* widget) {
		widget->handle_portal_to_overview();
	}

	std::string get_name() {
		return "portal_to_overview";
	}
};

class GotoSelectedTextCommand : public Command {

	void perform(MainWidget* widget) {
		widget->long_jump_to_destination(widget->selection_begin.y);
	}

	std::string get_name() {
		return "goto_selected_text";
	}
};

class FocusTextCommand : public TextCommand {

	void perform(MainWidget* widget) {
		std::wstring text_ = text.value();
		widget->handle_focus_text(text_);
	}

	std::string get_name() {
		return "focus_text";
	}

	std::string text_requirement_name() {
		return "Text to focus";
	}
};

class GotoWindowCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_goto_window();
	}

	std::string get_name() {
		return "goto_window";
	}

	bool requires_document() { return false; }
};

class ToggleSmoothScrollModeCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_toggle_smooth_scroll_mode();
	}

	std::string get_name() {
		return "toggle_smooth_scroll_mode";
	}

	bool requires_document() { return false; }
};

class ToggleScrollbarCommand : public Command {

	void perform(MainWidget* widget) {
        widget->toggle_scrollbar();
	}

	std::string get_name() {
		return "toggle_scrollbar";
	}
};

class OverviewToPortalCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_overview_to_portal();
	}

	std::string get_name() {
		return "overview_to_portal";
	}
};

class SelectRectCommand : public Command {

	void perform(MainWidget* widget) {
        widget->set_rect_select_mode(true);
	}

	std::string get_name() {
		return "select_rect";
	}
};

class ToggleTypingModeCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_toggle_typing_mode();
	}

	std::string get_name() {
		return "toggle_typing_mode";
	}
};

class DonateCommand : public Command {

	void perform(MainWidget* widget) {
        open_web_url(L"https://www.buymeacoffee.com/ahrm");
	}

	std::string get_name() {
		return "donate";
	}
	bool requires_document() { return false; }
};

class OverviewNextItemCommand : public Command {

	void perform(MainWidget* widget) {
        if (num_repeats == 0) num_repeats++;
        widget->opengl_widget->goto_search_result(num_repeats, true);
	}

	std::string get_name() {
		return "overview_next_item";
	}
};

class OverviewPrevItemCommand : public Command {

	void perform(MainWidget* widget) {
        if (num_repeats == 0) num_repeats++;
        widget->opengl_widget->goto_search_result(-num_repeats, true);
	}

	std::string get_name() {
		return "overview_prev_item";
	}
};

class DeleteHighlightUnderCursorCommand : public Command {

	void perform(MainWidget* widget) {
		widget->handle_delete_highlight_under_cursor();
	}

	std::string get_name() {
		return "delete_highlight_under_cursor";
	}
};

class NoopCommand : public Command {

	void perform(MainWidget* widget) {
	}

	std::string get_name() {
		return "noop";
	}
	bool requires_document() { return false; }
};

class ImportCommand : public Command {

	void perform(MainWidget* widget) {
        std::wstring import_file_name = select_json_file_name();
        widget->db_manager->import_json(import_file_name, widget->checksummer);
	}

	std::string get_name() {
		return "import";
	}
	bool requires_document() { return false; }
};

class ExportCommand : public Command {

	void perform(MainWidget* widget) {
        std::wstring export_file_name = select_new_json_file_name();
        widget->db_manager->export_json(export_file_name, widget->checksummer);
	}

	std::string get_name() {
		return "export";
	}
	bool requires_document() { return false; }
};

class EnterVisualMarkModeCommand : public Command {

	void perform(MainWidget* widget) {
        widget->visual_mark_under_pos({ widget->width()/2, widget->height()/2});
	}

	std::string get_name() {
		return "enter_visual_mark_mode";
	}
};

class SetPageOffsetCommand : public TextCommand {

	void perform(MainWidget* widget) {
        if (is_string_numeric(text.value().c_str()) && text.value().size() < 6) { // make sure the page number is valid
            widget->main_document_view->set_page_offset(std::stoi(text.value().c_str()));
        }
	}

	std::string get_name() {
		return "set_page_offset";
	}
};

class ToggleVisualScrollCommand : public Command {

	void perform(MainWidget* widget) {
        widget->toggle_visual_scroll_mode();
	}

	std::string get_name() {
		return "toggle_visual_scroll";
	}
};

class ToggleHorizontalLockCommand : public Command {

	void perform(MainWidget* widget) {
        widget->horizontal_scroll_locked = !widget->horizontal_scroll_locked;
	}

	std::string get_name() {
		return "toggle_horizontal_lock";
	}
};

class ExecuteCommand : public TextCommand {

	void perform(MainWidget* widget) {
        widget->execute_command(text.value());
	}

	std::string get_name() {
		return "execute";
	}
	bool requires_document() { return false; }
};

class EmbedAnnotationsCommand : public Command {

	void perform(MainWidget* widget) {
        std::wstring embedded_pdf_file_name = select_new_pdf_file_name();
        if (embedded_pdf_file_name.size() > 0) {
            widget->main_document_view->get_document()->embed_annotations(embedded_pdf_file_name);
        }
	}

	std::string get_name() {
		return "embed_annotations";
	}
};

class CopyWindowSizeConfigCommand : public Command {

	void perform(MainWidget* widget) {
        copy_to_clipboard(widget->get_window_configuration_string());
	}

	std::string get_name() {
		return "copy_window_size_config";
	}
	bool requires_document() { return false; }
};

class ToggleSelectHighlightCommand : public Command {

	void perform(MainWidget* widget) {
        widget->is_select_highlight_mode = !widget->is_select_highlight_mode;
	}

	std::string get_name() {
		return "toggle_select_highlight";
	}
};

class OpenLastDocumentCommand : public Command {

	void perform(MainWidget* widget) {
        auto last_opened_file = widget->get_last_opened_file_checksum();
        if (last_opened_file) {
            widget->open_document_with_hash(last_opened_file.value());
        }
	}

	std::string get_name() {
		return "open_last_document";
	}

	bool requires_document() { return false; }
};

class ToggleStatusbarCommand : public Command {

	void perform(MainWidget* widget) {
		widget->toggle_statusbar();
	}

	std::string get_name() {
		return "toggle_statusbar";
	}

	bool requires_document() { return false; }
};

class LazyCommand : public Command {
private:
	CommandManager* command_manager;
	MainWidget* widget;
	std::string command_name;
	std::wstring command_params;
	std::unique_ptr<Command> actual_command = nullptr;
	NoopCommand noop;

	void parse_command_text(std::wstring command_text) {
		int index = command_text.find(L"(");
		if (index != -1) {
			command_name = utf8_encode(command_text.substr(0, index));
			command_params = command_text.substr(index + 1, command_text.size() - index - 2);
		}
		else {
			command_name = utf8_encode(command_text);
		}
	}

	Command* get_command() {
		if (actual_command) {
			return actual_command.get();
		}
		else {
			actual_command = std::move(command_manager->get_command_with_name(command_name));
			if (!actual_command) return &noop;

			auto req = actual_command->next_requirement(widget);
			if (req) {
				if (req.value().type == RequirementType::Text) {
					actual_command->set_text_requirement(command_params);
				}
				if (req.value().type == RequirementType::File) {
					actual_command->set_file_requirement(command_params);
				}
				if (req.value().type == RequirementType::Symbol) {
					if (command_params.size() > 0) {
						actual_command->set_symbol_requirement((char)command_params[0]);
					}
				}
			}
			return actual_command.get();
		}
	}


public:
	LazyCommand(CommandManager* manager, std::wstring command_text) {
		command_manager = manager;
		parse_command_text(command_text);
	}

	void set_text_requirement(std::wstring value) { get_command()->set_text_requirement(value); }
	void set_symbol_requirement(char value) { get_command()->set_symbol_requirement(value); }
	void set_file_requirement(std::wstring value) { get_command()->set_file_requirement(value); }
	void set_rect_requirement(fz_rect value) { get_command()->set_rect_requirement(value); }
	void set_num_repeats(int nr) { get_command()->set_num_repeats(nr); }
	std::vector<char> special_symbols() { return get_command()->special_symbols(); }
	void pre_perform(MainWidget* widget) { get_command()->pre_perform(widget); }
	bool pushes_state() { return get_command()->pushes_state(); }
	bool requires_document() { return get_command()->requires_document(); }

	virtual void perform(MainWidget* w) {
		auto com = get_command();
		if (com) {
			com->run(w);
		}
	}

	std::string get_name() {
		auto com = get_command();
		if (com) {
			return com->get_name();
		}
		else {
			return "";
		}
	}

};


class CustomCommand : public Command {

	std::wstring raw_command;
	std::string name;
	std::optional<fz_rect> command_rect;
	std::optional<std::wstring> command_text;

public:

	CustomCommand(std::string name_, std::wstring command_) {
		raw_command = command_;
		name = name_;
	}

	std::optional<Requirement> next_requirement(MainWidget* widget) {
		if (command_requires_rect(raw_command) && (!command_rect.has_value())) {
			Requirement req = { RequirementType::Rect, "Command Rect"};
			return req;
		}
		if (command_requires_text(raw_command) && (!command_text.has_value())) {
			Requirement req = { RequirementType::Text, "Command Text"};
			return req;
		}
		return {};
	}

	void set_rect_requirement(fz_rect rect) {
		command_rect = rect;
	}

	void set_text_requirement(std::wstring txt) {
		command_text = txt;
	}

	void perform(MainWidget* widget) {
		widget->execute_command(raw_command, command_text.value_or(L""));
	}

	std::string get_name() {
		return name;
	}

};


class ConfigCommand : public TextCommand {
	std::string config_name;
public:
	ConfigCommand(std::string config_name_) {
		config_name = config_name_;
	}

	void perform(MainWidget* widget) {
        widget->config_manager->deserialize_config(config_name, text.value());
	}

	std::string get_name() {
		return "setconfig_" + config_name;
	}
	
	bool requires_document() { return false; }
};

class MacroCommand : public Command {
	std::vector<std::unique_ptr<Command>> commands;
	//std::wstring commands;
	std::string name;
	CommandManager* command_manager;

public:
	//MacroCommand(std::string name_, std::vector<std::unique_ptr<NewCommand>> commands_) {

	MacroCommand(CommandManager* manager, std::string name_, std::wstring commands_) {
		//commands = std::move(commands_);
		command_manager = manager;
		name = name_;

		auto parts = QString::fromStdWString(commands_).split(';');
		for (int i = 0; i < parts.size(); i++) {
			if (parts.at(i).size() > 0) {
				commands.push_back(std::make_unique<LazyCommand>(manager, parts.at(i).toStdWString()));
			}
		}

	}


	void perform(MainWidget* widget) {
		for (std::unique_ptr<Command>& subcommand : commands) {
			subcommand->run(widget);
		}
	}

	std::string get_name() {
		return name;
	}
};

CommandManager::CommandManager(ConfigManager* config_manager) {

	new_commands["goto_beginning"] = []() {return std::make_unique< GotoBeginningCommand>(); };
	new_commands["goto_end"] = []() {return std::make_unique< GotoEndCommand>(); };
	new_commands["goto_definition"] = []() {return std::make_unique< GotoDefinitionCommand>(); };
	new_commands["overview_definition"] = []() {return std::make_unique< OverviewDefinitionCommand>(); };
	new_commands["portal_to_definition"] = []() {return std::make_unique< PortalToDefinitionCommand>(); };
	new_commands["next_item"] = []() {return std::make_unique< NextItemCommand>(); };
	new_commands["previous_item"] = []() {return std::make_unique< PrevItemCommand>(); };
	new_commands["set_mark"] = []() {return std::make_unique< SetMark>(); };
	new_commands["goto_mark"] = []() {return std::make_unique< GotoMark>(); };
	new_commands["goto_page_with_page_number"] = []() {return std::make_unique< GotoPageWithPageNumberCommand>(); };
	new_commands["search"] = []() {return std::make_unique< SearchCommand>(); };
	new_commands["regex_search"] = []() {return std::make_unique< RegexSearchCommand>(); };
	new_commands["chapter_search"] = []() {return std::make_unique< ChapterSearchCommand>(); };
	new_commands["move_down"] = []() {return std::make_unique< MoveDownCommand>(); };
	new_commands["move_up"] = []() {return std::make_unique< MoveUpCommand>(); };
	new_commands["move_left"] = []() {return std::make_unique< MoveLeftCommand>(); };
	new_commands["move_right"] = []() {return std::make_unique< MoveRightCommand>(); };
	new_commands["zoom_in"] = []() {return std::make_unique< ZoomInCommand>(); };
	new_commands["zoom_out"] = []() {return std::make_unique< ZoomOutCommand>(); };
	new_commands["fit_to_page_width"] = []() {return std::make_unique< FitToPageWidthCommand>(); };
	new_commands["fit_to_page_height"] = []() {return std::make_unique< FitToPageHeightCommand>(); };
	new_commands["fit_to_page_height_smart"] = []() {return std::make_unique< FitToPageHeightSmartCommand>(); };
	new_commands["fit_to_page_width_smart"] = []() {return std::make_unique< FitToPageWidthSmartCommand>(); };
	new_commands["next_page"] = []() {return std::make_unique< NextPageCommand>(); };
	new_commands["previous_page"] = []() {return std::make_unique< PreviousPageCommand>(); };
	new_commands["open_document"] = []() {return std::make_unique< OpenDocumentCommand>(); };
	new_commands["add_bookmark"] = []() {return std::make_unique< AddBookmarkCommand>(); };
	new_commands["add_highlight"] = []() {return std::make_unique< AddHighlightCommand>(); };
	new_commands["goto_toc"] = []() {return std::make_unique< GotoTableOfContentsCommand>(); };
	new_commands["goto_highlight"] = []() {return std::make_unique< GotoHighlightCommand>(); };
	new_commands["goto_bookmark"] = []() {return std::make_unique< GotoBookmarkCommand>(); };
	new_commands["goto_bookmark_g"] = []() {return std::make_unique< GotoBookmarkGlobalCommand>(); };
	new_commands["goto_highlight_g"] = []() {return std::make_unique< GotoHighlightGlobalCommand>(); };
	new_commands["link"] = []() {return std::make_unique< PortalCommand>(); };
	new_commands["portal"] = []() {return std::make_unique< PortalCommand>(); };
	new_commands["next_state"] = []() {return std::make_unique< NextStateCommand>(); };
	new_commands["prev_state"] = []() {return std::make_unique< PrevStateCommand>(); };
	new_commands["delete_link"] = []() {return std::make_unique< DeletePortalCommand>(); };
	new_commands["delete_portal"] = []() {return std::make_unique< DeletePortalCommand>(); };
	new_commands["delete_bookmark"] = []() {return std::make_unique< DeleteBookmarkCommand>(); };
	new_commands["delete_highlight"] = []() {return std::make_unique< DeleteHighlightCommand>(); };
	new_commands["goto_link"] = []() {return std::make_unique< GotoPortalCommand>(); };
	new_commands["goto_portal"] = []() {return std::make_unique< GotoPortalCommand>(); };
	new_commands["edit_link"] = []() {return std::make_unique< EditPortalCommand>(); };
	new_commands["edit_portal"] = []() {return std::make_unique< EditPortalCommand>(); };
	new_commands["open_prev_doc"] = []() {return std::make_unique< OpenPrevDocCommand>(); };
	new_commands["open_document_embedded"] = []() {return std::make_unique< OpenDocumentEmbeddedCommand>(); };
	new_commands["open_document_embedded_from_current_path"] = []() {return std::make_unique< OpenDocumentEmbeddedFromCurrentPathCommand>(); };
	new_commands["copy"] = []() {return std::make_unique< CopyCommand>(); };
	new_commands["toggle_fullscreen"] = []() {return std::make_unique< ToggleFullscreenCommand>(); };
	new_commands["toggle_one_window"] = []() {return std::make_unique< ToggleOneWindowCommand>(); };
	new_commands["toggle_highlight"] = []() {return std::make_unique< ToggleHighlightCommand>(); };
	new_commands["toggle_synctex"] = []() {return std::make_unique< ToggleSynctexCommand>(); };
	new_commands["turn_on_synctex"] = []() {return std::make_unique< TurnOnSynctexCommand>(); };
	new_commands["toggle_show_last_command"] = []() {return std::make_unique< ToggleShowLastCommand>(); };
	new_commands["command"] = []() {return std::make_unique< CommandCommand>(); };
	new_commands["external_search"] = []() {return std::make_unique< ExternalSearchCommand>(); };
	new_commands["open_selected_url"] = []() {return std::make_unique< OpenSelectedUrlCommand>(); };
	new_commands["screen_down"] = []() {return std::make_unique< ScreenDownCommand>(); };
	new_commands["screen_up"] = []() {return std::make_unique< ScreenUpCommand>(); };
	new_commands["next_chapter"] = []() {return std::make_unique< NextChapterCommand>(); };
	new_commands["prev_chapter"] = []() {return std::make_unique< PrevChapterCommand>(); };
	new_commands["toggle_dark_mode"] = []() {return std::make_unique< ToggleDarkModeCommand>(); };
	new_commands["toggle_presentation_mode"] = []() {return std::make_unique< TogglePresentationModeCommand>(); };
	new_commands["turn_on_presentation_mode"] = []() {return std::make_unique< TurnOnPresentationModeCommand>(); };
	new_commands["toggle_mouse_drag_mode"] = []() {return std::make_unique< ToggleMouseDragMode>(); };
	new_commands["close_window"] = []() {return std::make_unique< CloseWindowCommand>(); };
	new_commands["quit"] = []() {return std::make_unique< QuitCommand>(); };
	new_commands["escape"] = []() {return std::make_unique< EscapeCommand>(); };
	new_commands["q"] = []() {return std::make_unique< QuitCommand>(); };
	new_commands["open_link"] = []() {return std::make_unique< OpenLinkCommand>(); };
	new_commands["overview_link"] = []() {return std::make_unique< OverviewLinkCommand>(); };
	new_commands["portal_to_link"] = []() {return std::make_unique< PortalToLinkCommand>(); };
	new_commands["copy_link"] = []() {return std::make_unique< CopyLinkCommand>(); };
	new_commands["keyboard_select"] = []() {return std::make_unique< KeyboardSelectCommand>(); };
	new_commands["keyboard_smart_jump"] = []() {return std::make_unique< KeyboardSmartjumpCommand>(); };
	new_commands["keyboard_overview"] = []() {return std::make_unique< KeyboardOverviewCommand>(); };
	new_commands["keys"] = []() {return std::make_unique< KeysCommand>(); };
	new_commands["keys_user"] = []() {return std::make_unique< KeysUserCommand>(); };
	new_commands["prefs"] = []() {return std::make_unique< PrefsCommand>(); };
	new_commands["prefs_user"] = []() {return std::make_unique< PrefsUserCommand>(); };
	new_commands["move_visual_mark_down"] = []() {return std::make_unique< MoveVisualMarkDownCommand>(); };
	new_commands["move_visual_mark_up"] = []() {return std::make_unique< MoveVisualMarkUpCommand>(); };
	new_commands["toggle_custom_color"] = []() {return std::make_unique< ToggleCustomColorMode>(); };
	new_commands["set_select_highlight_type"] = []() {return std::make_unique< SetSelectHighlightTypeCommand>(); };
	new_commands["toggle_window_configuration"] = []() {return std::make_unique< ToggleWindowConfigurationCommand>(); };
	new_commands["prefs_user_all"] = []() {return std::make_unique< PrefsUserAllCommand>(); };
	new_commands["keys_user_all"] = []() {return std::make_unique< KeysUserAllCommand>(); };
	new_commands["fit_to_page_width_ratio"] = []() {return std::make_unique< FitToPageWidthRatioCommand>(); };
	new_commands["smart_jump_under_cursor"] = []() {return std::make_unique< SmartJumpUnderCursorCommand>(); };
	new_commands["overview_under_cursor"] = []() {return std::make_unique< OverviewUnderCursorCommand>(); };
	new_commands["close_overview"] = []() {return std::make_unique< CloseOverviewCommand>(); };
	new_commands["visual_mark_under_cursor"] = []() {return std::make_unique< VisualMarkUnderCursorCommand>(); };
	new_commands["close_visual_mark"] = []() {return std::make_unique< CloseVisualMarkCommand>(); };
	new_commands["zoom_in_cursor"] = []() {return std::make_unique< ZoomInCursorCommand>(); };
	new_commands["zoom_out_cursor"] = []() {return std::make_unique< ZoomOutCursorCommand>(); };
	new_commands["goto_left"] = []() {return std::make_unique< GotoLeftCommand>(); };
	new_commands["goto_left_smart"] = []() {return std::make_unique< GotoLeftSmartCommand>(); };
	new_commands["goto_right"] = []() {return std::make_unique< GotoRightCommand>(); };
	new_commands["goto_right_smart"] = []() {return std::make_unique< GotoRightSmartCommand>(); };
	new_commands["rotate_clockwise"] = []() {return std::make_unique< RotateClockwiseCommand>(); };
	new_commands["rotate_counterclockwise"] = []() {return std::make_unique< RotateCounterClockwiseCommand>(); };
	new_commands["goto_next_highlight"] = []() {return std::make_unique< GotoNextHighlightCommand>(); };
	new_commands["goto_prev_highlight"] = []() {return std::make_unique< GotoPrevHighlightCommand>(); };
	new_commands["goto_next_highlight_of_type"] = []() {return std::make_unique< GotoNextHighlightOfTypeCommand>(); };
	new_commands["goto_prev_highlight_of_type"] = []() {return std::make_unique< GotoPrevHighlightOfTypeCommand>(); };
	new_commands["add_highlight_with_current_type"] = []() {return std::make_unique< AddHighlightWithCurrentTypeCommand>(); };
	new_commands["enter_password"] = []() {return std::make_unique< EnterPasswordCommand>(); };
	new_commands["toggle_fastread"] = []() {return std::make_unique< ToggleFastreadCommand>(); };
	new_commands["goto_top_of_page"] = []() {return std::make_unique< GotoTopOfPageCommand>(); };
	new_commands["goto_bottom_of_page"] = []() {return std::make_unique< GotoBottomOfPageCommand>(); };
	new_commands["new_window"] = []() {return std::make_unique< NewWindowCommand>(); };
	new_commands["reload"] = []() {return std::make_unique< ReloadCommand>(); };
	new_commands["reload_config"] = []() {return std::make_unique< ReloadConfigCommand>(); };
	new_commands["synctex_under_cursor"] = []() {return std::make_unique< SynctexUnderCursorCommand>(); };
	new_commands["set_status_string"] = []() {return std::make_unique< SetStatusStringCommand>(); };
	new_commands["clear_status_string"] = []() {return std::make_unique< ClearStatusStringCommand>(); };
	new_commands["toggle_titlebar"] = []() {return std::make_unique< ToggleTittlebarCommand>(); };
	new_commands["next_preview"] = []() {return std::make_unique< NextPreviewCommand>(); };
	new_commands["previous_preview"] = []() {return std::make_unique< PreviousPreviewCommand>(); };
	new_commands["goto_overview"] = []() {return std::make_unique< GotoOverviewCommand>(); };
	new_commands["portal_to_overview"] = []() {return std::make_unique< PortalToOverviewCommand>(); };
	new_commands["goto_selected_text"] = []() {return std::make_unique< GotoSelectedTextCommand>(); };
	new_commands["focus_text"] = []() {return std::make_unique< FocusTextCommand>(); };
	new_commands["goto_window"] = []() {return std::make_unique< GotoWindowCommand>(); };
	new_commands["toggle_smooth_scroll_mode"] = []() {return std::make_unique< ToggleSmoothScrollModeCommand>(); };
	new_commands["goto_begining"] = []() {return std::make_unique< GotoBeginningCommand>(); };
	new_commands["toggle_scrollbar"] = []() {return std::make_unique< ToggleScrollbarCommand>(); };
	new_commands["overview_to_portal"] = []() {return std::make_unique< OverviewToPortalCommand>(); };
	new_commands["select_rect"] = []() {return std::make_unique< SelectRectCommand>(); };
	new_commands["toggle_typing_mode"] = []() {return std::make_unique< ToggleTypingModeCommand>(); };
	new_commands["donate"] = []() {return std::make_unique< DonateCommand>(); };
	new_commands["overview_next_item"] = []() {return std::make_unique< OverviewNextItemCommand>(); };
	new_commands["overview_prev_item"] = []() {return std::make_unique< OverviewPrevItemCommand>(); };
	new_commands["delete_highlight_under_cursor"] = []() {return std::make_unique< DeleteHighlightUnderCursorCommand>(); };
	new_commands["noop"] = []() {return std::make_unique< NoopCommand>(); };
	new_commands["import"] = []() {return std::make_unique< ImportCommand>(); };
	new_commands["export"] = []() {return std::make_unique< ExportCommand>(); };
	new_commands["enter_visual_mark_mode"] = []() {return std::make_unique< EnterVisualMarkModeCommand>(); };
	new_commands["set_page_offset"] = []() {return std::make_unique< SetPageOffsetCommand>(); };
	new_commands["toggle_visual_scroll"] = []() {return std::make_unique< ToggleVisualScrollCommand>(); };
	new_commands["toggle_horizontal_scroll_lock"] = []() {return std::make_unique< ToggleHorizontalLockCommand>(); };
	new_commands["execute"] = []() {return std::make_unique< ExecuteCommand>(); };
	new_commands["embed_annotations"] = []() {return std::make_unique< EmbedAnnotationsCommand>(); };
	new_commands["copy_window_size_config"] = []() {return std::make_unique< CopyWindowSizeConfigCommand>(); };
	new_commands["toggle_select_highlight"] = []() {return std::make_unique< ToggleSelectHighlightCommand>(); };
	new_commands["open_last_document"] = []() {return std::make_unique< OpenLastDocumentCommand>(); };
	new_commands["toggle_statusbar"] = []() {return std::make_unique< ToggleStatusbarCommand>(); };


	for (auto [command_name_, command_value] : ADDITIONAL_COMMANDS) {
		std::string command_name = utf8_encode(command_name_);
		std::wstring local_command_value = command_value;
		new_commands[command_name] = [command_name, local_command_value]() {return  std::make_unique<CustomCommand>(command_name, local_command_value); };
	}

	for (auto [command_name_, macro_value] : ADDITIONAL_MACROS) {
		std::string command_name = utf8_encode(command_name_);
		std::wstring local_macro_value = macro_value;
		new_commands[command_name] = [command_name, local_macro_value, this]() {return std::make_unique<MacroCommand>(this, command_name, local_macro_value); };
	}

	std::vector<Config> configs = config_manager->get_configs();

	for (auto conf : configs) {
		std::string confname = utf8_encode(conf.name);
		std::string config_set_command_name = "setconfig_" + confname;
		//commands.push_back({ config_set_command_name, true, false , false, false, true, {} });
		new_commands[config_set_command_name] = [confname]() {return std::make_unique<ConfigCommand>(confname); };

	}

}

std::unique_ptr<Command> CommandManager::get_command_with_name(std::string name) {
	if (new_commands.find(name) != new_commands.end()) {
		return new_commands[name]();
	}
	//for (const auto &com : commands) {
	//	if (com.name == name) {
	//		return &com;
	//	}
	//}
	return nullptr;
}

QStringList CommandManager::get_all_command_names() {
	QStringList res;
	//for (const auto &com : commands) {
	//	res.push_back(QString::fromStdString(com.name));
	//}
	for (const auto &com : new_commands) {
		res.push_back(QString::fromStdString(com.first));
	}
	return res;
}

void print_tree_node(InputParseTreeNode node) {
	if (node.requires_text) {
		std::wcout << "text node" << std::endl;
		return;
	}
	if (node.requires_symbol) {
		std::wcout << "symbol node" << std::endl;
		return;
	}

	if (node.control_modifier) {
		std::wcout << "Ctrl+";
	}

	if (node.shift_modifier) {
		std::wcout << "Shift+";
	}

	if (node.alt_modifier) {
		std::wcout << "Alt+";
	}
	std::wcout << node.command << std::endl;
}

InputParseTreeNode parse_token(std::wstring token) {
	InputParseTreeNode res;

	if (token == L"sym") {
		res.requires_symbol = true;
		return res;
	}
	if (token == L"txt") {
		res.requires_text = true;
		return res;
	}

	std::vector<std::wstring> subcommands;
	split_key_string(token, L"-", subcommands);

	for (size_t i = 0; i < subcommands.size() - 1; i++) {
		if (subcommands[i] == L"C") {
			res.control_modifier = true;
		}

		if (subcommands[i] == L"S") {
			res.shift_modifier = true;
		}

		if (subcommands[i] == L"A") {
			res.alt_modifier = true;
		}
	}

	std::wstring command_string = subcommands[subcommands.size() - 1];
	if (command_string.size() == 1) {
		res.command = subcommands[subcommands.size() - 1][0];
	}
	else {

		if (int f_key = get_f_key(command_string)) {
			res.command = Qt::Key::Key_F1 - 1 + f_key;
		}
		else {

			std::map<std::wstring, Qt::Key> keymap_temp = {
				{L"up", Qt::Key::Key_Up},
				{L"down", Qt::Key::Key_Down},
				{L"left", Qt::Key::Key_Left},
				{L"right", Qt::Key::Key_Right},
				{L"backspace", Qt::Key::Key_Backspace},
				{L"space", Qt::Key::Key_Space},
				{L"pageup", Qt::Key::Key_PageUp},
				{L"pagedown", Qt::Key::Key_PageDown},
				{L"home", Qt::Key::Key_Home},
				{L"end", Qt::Key::Key_End},
				{L"pagedown", Qt::Key::Key_End},
				{L"tab", Qt::Key::Key_Tab},
				{L"return", Qt::Key::Key_Return},
			};
			std::map<std::wstring, Qt::Key> keymap;

			for (auto item : keymap_temp) {
				keymap[item.first] = item.second;
				keymap[L"<" + item.first + L">"] = item.second;
			}

			res.command = keymap[command_string];
		}

	}

	return res;
}
void get_tokens(std::wstring line, std::vector<std::wstring>& tokens) {
	std::wstring stack;

	int stack_depth = 0;

	for (wchar_t c : line) {
		if (stack_depth && (c != '>') && (c != '<')) {
			stack.push_back(c);
		}
		else if ((c == '>')) {
			stack_depth--;
			if (stack_depth == 0) {
				tokens.push_back(stack);
				stack.clear();
			}
			else {
				stack.push_back(c);
			}
		}
		else if (c == '<') {
			if (stack_depth) {
				stack.push_back(c);
			}
			stack_depth++;
		}
		else {
			tokens.push_back(std::wstring(1, c));
		}

	}
}

InputParseTreeNode* parse_lines(
	InputParseTreeNode* root,
	const std::vector<std::wstring>& lines,
	const std::vector<std::vector<std::string>>& command_names,
	const std::vector<std::wstring>& command_file_names,
	const std::vector<int>& command_line_numbers
	) {

	for (size_t j = 0; j < lines.size(); j++) {
		std::wstring line = lines[j];

		// for example convert "<a-<space>> to ["a", "space"]
		std::vector<std::wstring> tokens;
		get_tokens(line, tokens);

		InputParseTreeNode* parent_node = root;

		for (size_t i = 0; i < tokens.size(); i++) {
			InputParseTreeNode node = parse_token(tokens[i]);
			bool existing_node = false;
			for (InputParseTreeNode* child : parent_node->children) {
				if (child->is_same(&node)) {
					parent_node = child;
					existing_node = true;
					break;
				}
			}
			if (!existing_node) {
				if ((tokens[i] != L"sym") && (tokens[i] != L"txt")) {
					if (parent_node->is_final) {
                        std::wcout
                            << L"Warning: key defined in " << command_file_names[j]
                            << L":" << command_line_numbers[j]
                            << L" for " << utf8_decode(command_names[j][0])
                            << L" is unreachable, shadowed by final key sequence defined in "
                            << parent_node->defining_file_path
                            << L":" << parent_node->defining_file_line << L"\n";
					}
					auto new_node = new InputParseTreeNode(node);
					new_node->defining_file_line = command_line_numbers[j];
					new_node->defining_file_path = command_file_names[j];
					parent_node->children.push_back(new_node);
					parent_node = parent_node->children[parent_node->children.size() - 1];
				}
				else {
					if (tokens[i] == L"sym") {
						parent_node->requires_symbol = true;
						parent_node->is_final = true;
					}

					if (tokens[i] == L"txt") {
						parent_node->requires_text = true;
						parent_node->is_final = true;
					}
				}
			}
			else if (((size_t)i == (tokens.size() - 1)) &&
				(SHOULD_WARN_ABOUT_USER_KEY_OVERRIDE ||
					(command_file_names[j].compare(parent_node->defining_file_path)) == 0)) {
				if ((parent_node->name.size() == 0) || parent_node->name[0].compare(command_names[j][0]) != 0) {

					std::wcout << L"Warning: key defined in " << parent_node->defining_file_path
						<< L":" << parent_node->defining_file_line
						<< L" overwritten by " << command_file_names[j]
						<< L":" << command_line_numbers[j];
					if (parent_node->name.size() > 0) {
						std::wcout << L". Overriding command: " << line 
							<< L": replacing " << utf8_decode(parent_node->name[0])
							<< L" with " << utf8_decode(command_names[j][0]);
					}
					std::wcout << L"\n";
				}
			}
			if ((size_t) i == (tokens.size() - 1)) {
				parent_node->is_final = true;
				parent_node->name.clear();
                parent_node->defining_file_line = command_line_numbers[j];
                parent_node->defining_file_path = command_file_names[j];
				for (size_t k = 0; k < command_names[j].size(); k++) {
					parent_node->name.push_back(command_names[j][k]);
				}
			}
			else {
				if (parent_node->is_final && (parent_node->name.size() > 0)) {
					std::wcout << L"Warning: unmapping " << utf8_decode(parent_node->name[0]) << L" because of " << utf8_decode(command_names[j][0]) << L" which uses " << line << L"\n";
				}
				parent_node->is_final = false;
			}

		}
	}

	return root;
}

InputParseTreeNode* parse_lines(
	const std::vector<std::wstring>& lines,
	const std::vector<std::vector<std::string>>& command_names,
	const std::vector<std::wstring>& command_file_names,
	const std::vector<int>& command_line_numbers
	) {
	// parse key configs into a trie where leaves are annotated with the name of the command

	InputParseTreeNode* root = new InputParseTreeNode;
	root->is_root = true;

	parse_lines(root, lines, command_names, command_file_names, command_line_numbers);

	return root;

}

std::vector<std::string> parse_command_name(const std::wstring& command_names) {
	QStringList parts = QString::fromStdWString(command_names).split(';');
	std::vector<std::string> res;
	for (int i = 0; i < parts.size(); i++) {
		res.push_back(parts.at(i).toStdString());
	}
	return res;
}

void get_keys_file_lines(const Path& file_path,
	std::vector<std::vector<std::string>>& command_names,
	std::vector<std::wstring>& command_keys,
	std::vector<std::wstring>& command_files,
	std::vector<int>& command_line_numbers) {

	std::ifstream infile = std::ifstream(utf8_encode(file_path.get_path()));

	int line_number = 0;
	std::wstring default_path_name = file_path.get_path();
	while (infile.good()) {
		line_number++;
		std::string line_;
		std::wstring line;
		std::getline(infile, line_);
		line = utf8_decode(line_);

		if (line.size() == 0 || line[0] == '#') {
			continue;
		}
		std::wstringstream ss(line);
		std::wstring command_name;
		std::wstring command_key;
		ss >> command_name >> command_key;
		//command_names.push_back(utf8_encode(command_name));
		command_names.push_back(parse_command_name(command_name));
		command_keys.push_back(command_key);
		command_files.push_back(default_path_name);
		command_line_numbers.push_back(line_number);
	}

	infile.close();
}

InputParseTreeNode* parse_key_config_files(const Path& default_path,
	const std::vector<Path>& user_paths) {

	std::wifstream default_infile = open_wifstream(default_path.get_path());

	std::vector<std::vector<std::string>> command_names;
	std::vector<std::wstring> command_keys;
	std::vector<std::wstring> command_files;
	std::vector<int> command_line_numbers;

	get_keys_file_lines(default_path, command_names, command_keys, command_files, command_line_numbers);
	for (auto upath : user_paths) {
		get_keys_file_lines(upath, command_names, command_keys, command_files, command_line_numbers);
	}

	return parse_lines(command_keys, command_names, command_files, command_line_numbers);
}


InputHandler::InputHandler(const Path& default_path, const std::vector<Path>& user_paths, CommandManager* cm) {
	command_manager = cm;
	user_key_paths = user_paths;
	reload_config_files(default_path, user_paths);
}

void InputHandler::reload_config_files(const Path& default_config_path, const std::vector<Path>& user_config_paths)
{
	delete_current_parse_tree(root);
	root = parse_key_config_files(default_config_path, user_config_paths);
	current_node = root;
}


bool is_digit(int key) {
	return key >= Qt::Key::Key_0 && key <= Qt::Key::Key_9;
}

std::vector<std::unique_ptr<Command>> InputHandler::handle_key(QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed, int* num_repeats) {
	std::vector<std::unique_ptr<Command>> res;

	int key = 0;
	if (!USE_LEGACY_KEYBINDS){
		std::vector<QString> special_texts = {"\b", "\t", " ", "\r", "\n"};
		if (((key_event->key() >= 'A') && (key_event->key() <= 'Z')) || ((key_event->text().size() > 0) &&
			(std::find(special_texts.begin(), special_texts.end(), key_event->text()) == special_texts.end()))) {
			if (!control_pressed && !alt_pressed) {
				// shift is already handled in the returned text
				shift_pressed = false;
				std::wstring text = key_event->text().toStdWString();
				key = key_event->text().toStdWString()[0];
			}
			else {
				key = key_event->key();
				if (key >= 'A' && key <= 'Z') {
					key = key - 'A' + 'a';
				}
			}
		}
		else {
			key = key_event->key();

			if (key == Qt::Key::Key_Backtab) {
				key = Qt::Key::Key_Tab;
			}
		}
	}
	else {
		key = key_event->key();
		if (key >= 'A' && key <= 'Z') {
			key = key - 'A' + 'a';
		}

	}

	if (current_node == root && is_digit(key)) {
		if (!(key == '0' && (number_stack.size() == 0)) && (!control_pressed) && (!shift_pressed) && (!alt_pressed)) {
			number_stack.push_back('0' + key - Qt::Key::Key_0);
			return {};
		}
	}

	for (InputParseTreeNode* child : current_node->children) {
		//if (child->command == key && child->shift_modifier == shift_pressed && child->control_modifier == control_pressed){
		if (child->matches(key, shift_pressed, control_pressed, alt_pressed)){
			if (child->is_final == true) {
				current_node = root;
				//cout << child->name << endl;
				*num_repeats = 0;
				if (number_stack.size() > 0) {
					*num_repeats = atoi(number_stack.c_str());
					number_stack.clear();
				}

				//return command_manager.get_command_with_name(child->name);
				for (size_t i = 0; i < child->name.size(); i++) {
					res.push_back(command_manager->get_command_with_name(child->name[i]));
				}
				return res;
			}
			else{
				current_node = child;
				return {};
			}
		}
	}
	std::wcout << "Warning: invalid command (key:" << (char)key << "); resetting to root" << std::endl;
	number_stack.clear();
	current_node = root;
	return {};
}

void InputHandler::delete_current_parse_tree(InputParseTreeNode* node_to_delete)
{
	bool is_root = false;

	if (node_to_delete != nullptr) {
		is_root = node_to_delete->is_root;

		for (size_t i = 0; i < node_to_delete->children.size(); i++) {
			delete_current_parse_tree(node_to_delete->children[i]);
		}
		delete node_to_delete;
	}

	if (is_root) {
		root = nullptr;
	}
}

bool InputParseTreeNode::is_same(const InputParseTreeNode* other) {
	return (command == other->command) &&
		(shift_modifier == other->shift_modifier) &&
		(control_modifier == other->control_modifier) &&
		(alt_modifier == other->alt_modifier) &&
		(requires_symbol == other->requires_symbol) &&
		(requires_text == other->requires_text);
}

bool InputParseTreeNode::matches(int key, bool shift, bool ctrl, bool alt)
{
	return (key == this->command) && (shift == this->shift_modifier) && (ctrl == this->control_modifier) && (alt == this->alt_modifier);
}

std::optional<Path> InputHandler::get_or_create_user_keys_path() {
	if (user_key_paths.size() == 0) {
		return {};
	}

	for (int i = user_key_paths.size() - 1; i >= 0; i--) {
		if (user_key_paths[i].file_exists()) {
			return user_key_paths[i];
		}
	}
	user_key_paths.back().file_parent().create_directories();
	create_file_if_not_exists(user_key_paths.back().get_path());
	return user_key_paths.back();
}

std::unordered_map<std::string, std::vector<std::string>> InputHandler::get_command_key_mappings() const{
	std::unordered_map<std::string, std::vector<std::string>> res;
	std::vector<InputParseTreeNode*> prefix;
	add_command_key_mappings(root, res, prefix);
	return res;
}

void InputHandler::add_command_key_mappings(InputParseTreeNode* thisroot,
	std::unordered_map<std::string, std::vector<std::string>>& map,
	std::vector<InputParseTreeNode*> prefix) const {

	if (thisroot->is_final) {
		if (thisroot->name.size() == 1) {
			map[thisroot->name[0]].push_back(get_key_string_from_tree_node_sequence(prefix));
		}
		else if (thisroot->name.size() > 1) {
			for (const auto& name : thisroot->name) {
				map[name].push_back("{" + get_key_string_from_tree_node_sequence(prefix) + "}");
			}
		}
	}
	else{
		for (size_t i = 0; i < thisroot->children.size(); i++) {
			prefix.push_back(thisroot->children[i]);
			add_command_key_mappings(thisroot->children[i], map, prefix);
			prefix.pop_back();
		}

	}
}

std::string InputHandler::get_key_name_from_key_code(int key_code) const{
	std::string result;
	std::map<int, std::string> keymap = {
		{Qt::Key::Key_Up, "up"},
		{Qt::Key::Key_Down, "down"},
		{Qt::Key::Key_Left, "left"},
		{Qt::Key::Key_Right, "right"},
		{Qt::Key::Key_Backspace, "backspace"},
		{Qt::Key::Key_Space, "space"},
		{Qt::Key::Key_PageUp, "pageup"},
		{Qt::Key::Key_PageDown, "pagedown"},
		{Qt::Key::Key_Home, "home"},
		{Qt::Key::Key_End, "end"},
		{Qt::Key::Key_Tab, "tab"},
		{Qt::Key::Key_Backtab, "tab"},
	};

	//if (((key_code <= 'z') && (key_code >= 'a')) || ((key_code <= 'Z') && (key_code >= 'A'))) {
	if (key_code < 127) {
		result.push_back(key_code);
		return result;
	}
	else if (keymap.find(key_code) != keymap.end()) {
		return "<" + keymap[key_code] + ">";
	}
	else if ((key_code >= Qt::Key::Key_F1) && (key_code <= Qt::Key::Key_F35)) {
		int f_number = 1 + (key_code - Qt::Key::Key_F1);
		return "<f" + QString::number(f_number).toStdString() + ">";
	}
	else {
		return "UNK";
	}
}

std::string InputHandler::get_key_string_from_tree_node_sequence(const std::vector<InputParseTreeNode*> seq) const{
	std::string res;
	for (size_t i = 0; i < seq.size(); i++) {
		if (seq[i]->alt_modifier || seq[i]->shift_modifier || seq[i]->control_modifier ) {
			res += "<";
		}
		std::string current_key_command_name = get_key_name_from_key_code(seq[i]->command);

		if (seq[i]->alt_modifier) {
			res += "A-";
		}
		if (seq[i]->control_modifier) {
			res += "C-";
		}
		if (seq[i]->shift_modifier) {
			res += "S-";
		}
		res += current_key_command_name;
		if (seq[i]->alt_modifier || seq[i]->shift_modifier || seq[i]->control_modifier ) {
			res += ">";
		}
	}
	return res;
}
std::vector<Path> InputHandler::get_all_user_keys_paths() {
	std::vector<Path> res;

	for (int i = user_key_paths.size() - 1; i >= 0; i--) {
		if (user_key_paths[i].file_exists()) {
			res.push_back(user_key_paths[i]);
		}
	}

	return res;
}

std::optional<Requirement> Command::next_requirement(MainWidget* widget) {
	return {};
}
void Command::set_text_requirement(std::wstring value) {}
void Command::set_symbol_requirement(char value) {}
void Command::set_file_requirement(std::wstring value) {}
void Command::set_rect_requirement(fz_rect value) {}

bool Command::pushes_state() {
	return false;
}

bool Command::requires_document() {
	return true;
}

void Command::set_num_repeats(int nr) {
	num_repeats = nr;
}

void Command::pre_perform(MainWidget* widget) {

}

void Command::run(MainWidget* widget) {
	if (this->requires_document() && !(widget->main_document_view_has_document())) {
		return;
	}
	perform(widget);
}

std::vector<char> Command::special_symbols() {
	std::vector<char> res;
	return res;
}


std::string Command::get_name() {
	return "";
}

std::unique_ptr<Command> CommandManager::create_macro_command(std::string name, std::wstring macro_string) {
	return std::make_unique<MacroCommand>(this, name, macro_string);
}
