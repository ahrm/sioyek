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

#include "touchui/TouchListView.h"

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
extern bool TOUCH_MODE;
extern bool VERBOSE;
extern float FREETEXT_BOOKMARK_FONT_SIZE;

Command::Command(MainWidget* widget_) : widget(widget_) {

}

class SymbolCommand : public Command {
protected:
	char symbol = 0;
public:
	SymbolCommand(MainWidget* w) : Command(w) {}
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
	
	TextCommand(MainWidget* w) : Command(w) {}

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
public:

	GotoMark(MainWidget* w) : SymbolCommand(w) {}

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

	void perform() {
		assert(this->symbol != 0);
		widget->goto_mark(this->symbol);
	}

	bool requires_document() { return false; }
};

class SetMark : public SymbolCommand {
public:
	SetMark(MainWidget* w) : SymbolCommand(w) {}

	std::string get_name() {
		return "set_mark";
	}

	void perform() {
		assert(this->symbol != 0);
		widget->set_mark_in_current_location(this->symbol);
	}
};

class ToggleDrawingMask : public SymbolCommand {
public:
	ToggleDrawingMask(MainWidget* w) : SymbolCommand(w) {}

	std::string get_name() {
		return "toggle_drawing_mask";
	}

	void perform() {
		widget->handle_toggle_drawing_mask(this->symbol);
	}
};

class GotoLoadedDocumentCommand : public Command{
public:
	GotoLoadedDocumentCommand(MainWidget* w) : Command(w) {}

	void perform() {
		widget->handle_goto_loaded_document();
	}

	std::string get_name() {
		return "goto_tab";
	}
};

class NextItemCommand : public Command{
public:
	NextItemCommand(MainWidget* w) : Command(w) {}

	void perform() {
		if (num_repeats == 0) num_repeats++;
		widget->opengl_widget->goto_search_result(num_repeats);
	}

	std::string get_name() {
		return "next_item";
	}
};

class PrevItemCommand : public Command{
public:
	PrevItemCommand(MainWidget* w) : Command(w) {};

	void perform() {
		if (num_repeats == 0) num_repeats++;
		widget->opengl_widget->goto_search_result(-num_repeats);
	}

	std::string get_name() {
		return "previous_item";
	}
};

class ToggleTextMarkCommand : public Command{
public:
	ToggleTextMarkCommand(MainWidget* w) : Command(w) {};

	void perform() {
		//if (num_repeats == 0) num_repeats++;
		widget->handle_toggle_text_mark();
		//widget->invalidate_render();
	}

	std::string get_name() {
		return "toggle_text_mark";
	}
};

class MoveTextMarkForwardCommand : public Command{
public:
	MoveTextMarkForwardCommand(MainWidget* w) : Command(w) {};

	void perform() {
		//if (num_repeats == 0) num_repeats++;
		widget->handle_move_text_mark_forward(false);
		//widget->invalidate_render();
	}

	std::string get_name() {
		return "move_text_mark_forward";
	}
};

class MoveTextMarkDownCommand : public Command{
public:
	MoveTextMarkDownCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_move_text_mark_down();
	}

	std::string get_name() {
		return "move_text_mark_down";
	}
};

class MoveTextMarkUpCommand : public Command{
public:
	MoveTextMarkUpCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_move_text_mark_up();
	}

	std::string get_name() {
		return "move_text_mark_up";
	}
};

class MoveTextMarkForwardWordCommand : public Command{
public:
	MoveTextMarkForwardWordCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_move_text_mark_forward(true);
	}

	std::string get_name() {
		return "move_text_mark_forward_word";
	}
};

class MoveTextMarkBackwardCommand : public Command{
public:
	MoveTextMarkBackwardCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_move_text_mark_backward(false);
	}

	std::string get_name() {
		return "move_text_mark_backward";
	}
};

class MoveTextMarkBackwardWordCommand : public Command{
public:
	MoveTextMarkBackwardWordCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_move_text_mark_backward(true);
	}

	std::string get_name() {
		return "move_text_mark_backward_word";
	}
};

class StartReadingCommand : public Command{
public:
	StartReadingCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_start_reading();
	}

	std::string get_name() {
		return "start_reading";
	}
};

class StopReadingCommand : public Command{
public:
	StopReadingCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_stop_reading();
	}

	std::string get_name() {
		return "stop_reading";
	}
};

class SearchCommand : public TextCommand {
public:
	SearchCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
		widget->perform_search(this->text.value(), false);
		if (TOUCH_MODE) {
			widget->show_search_buttons();
		}
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

class AddAnnotationToSelectedHighlightCommand : public TextCommand {
public:
	AddAnnotationToSelectedHighlightCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
		widget->add_text_annotation_to_selected_highlight(this->text.value());
	}

	void pre_perform() {
		widget->text_command_line_edit->setText(
			QString::fromStdWString(widget->doc()->get_highlights()[widget->selected_highlight_index].text_annot)
		);
	}

	std::string get_name() {
		return "add_annot_to_highlight";
	}

	std::string text_requirement_name() {
		return "Comment";
	}
};

class SetFreehandThickness : public TextCommand {
public:
	SetFreehandThickness(MainWidget* w) : TextCommand(w) {};

	void perform() {
		float thickness = QString::fromStdWString(this->text.value()).toFloat();
		widget->set_freehand_thickness(thickness);
		//widget->perform_search(this->text.value(), false);
		//if (TOUCH_MODE) {
		//	widget->show_search_buttons();
		//}
	}

	std::string get_name() {
		return "set_freehand_thickness";
	}


	std::string text_requirement_name() {
		return "Thickness";
	}
};

class GotoPageWithLabel : public TextCommand {
public:
	GotoPageWithLabel(MainWidget* w) : TextCommand(w) {};

	void perform() {
		widget->goto_page_with_label(text.value());
	}

	std::string get_name() {
		return "goto_page_with_label";
	}

	bool pushes_state() {
		return true;
	}

	std::string text_requirement_name() {
		return "Page Label";
	}
};

class ChapterSearchCommand : public TextCommand {
public:
	ChapterSearchCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
		widget->perform_search(this->text.value(), false);
	}

	void pre_perform() {
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
public:
	RegexSearchCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
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
public:
	AddBookmarkCommand(MainWidget* w) : TextCommand(w) {}

	void perform() {
		widget->main_document_view->add_bookmark(text.value());
	}

	std::string get_name() {
		return "add_bookmark";
	}

	std::string text_requirement_name() {
		return "Bookmark Text";
	}
};

class AddBookmarkMarkedCommand: public Command {

public:

	std::optional<std::wstring> text_;
	std::optional<AbsoluteDocumentPos> point_;

	AddBookmarkMarkedCommand(MainWidget* w) : Command(w) {};

	std::optional<Requirement> next_requirement(MainWidget* widget) {

		if (!text_.has_value()) {
			Requirement req = { RequirementType::Text, "Bookmark Text"};
			return req;
		}
		if (!point_.has_value()) {
			Requirement req = { RequirementType::Point, "Bookmark Location"};
			return req;
		}
		return {};
	}

	void set_text_requirement(std::wstring value) {
		text_ = value;
	}

	void set_point_requirement(AbsoluteDocumentPos value) {
		point_ = value;
	}


	void perform() {
		widget->doc()->add_marked_bookmark(text_.value(), point_.value());
		//widget->delete_freehand_drawings(rect_.value());
		//widget->freehand_drawing_mode = original_drawing_mode;
	}

	std::string get_name() {
		return "add_marked_bookmark";
	}
};

class AddBookmarkFreetextCommand: public Command {

public:

	std::optional<std::wstring> text_;
	std::optional<fz_rect> rect_;
	int pending_index = -1;

	AddBookmarkFreetextCommand(MainWidget* w) : Command(w) {};

	std::optional<Requirement> next_requirement(MainWidget* widget) {

		if (!rect_.has_value()) {
			Requirement req = { RequirementType::Rect, "Bookmark Location"};
			return req;
		}
		if (!text_.has_value()) {
			Requirement req = { RequirementType::Text, "Bookmark Text"};
			return req;
		}
		return {};
	}

	void set_text_requirement(std::wstring value) {
		text_ = value;
	}

	void set_rect_requirement(fz_rect value) {
		rect_ = value;
		pending_index = widget->doc()->add_incomplete_freetext_bookmark(value);
		widget->selected_bookmark_index = pending_index;
	}


	void on_cancel() {

		if (pending_index != -1) {
			widget->doc()->undo_pending_bookmark(pending_index);
		}
	}

	void perform() {
		//widget->doc()->add_freetext_bookmark(text_.value(), rect_.value());
		widget->doc()->add_pending_freetext_bookmark(pending_index, text_.value());
		widget->clear_selected_rect();
		widget->invalidate_render();
	}

	std::string get_name() {
		return "add_freetext_bookmark";
	}
};

class GotoBookmarkCommand : public Command {
public:
	GotoBookmarkCommand(MainWidget* w) : Command(w) {};
	void perform() {
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
public:
	GotoBookmarkGlobalCommand(MainWidget* w) : Command(w) {};
	void perform() {
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

class IncreaseFreetextBookmarkFontSizeCommand : public Command {
public:
	IncreaseFreetextBookmarkFontSizeCommand(MainWidget* w) : Command(w) {};

	void perform() {
		FREETEXT_BOOKMARK_FONT_SIZE *= 1.1f;
		if (FREETEXT_BOOKMARK_FONT_SIZE > 100) {
			FREETEXT_BOOKMARK_FONT_SIZE = 100;
		}
		widget->update_selected_bookmark_font_size();

	}

	std::string get_name() {
		return "increase_freetext_font_size";
	}
};

class DecreaseFreetextBookmarkFontSizeCommand : public Command {
public:
	DecreaseFreetextBookmarkFontSizeCommand(MainWidget* w) : Command(w) {};

	void perform() {
		FREETEXT_BOOKMARK_FONT_SIZE /= 1.1f;
		if (FREETEXT_BOOKMARK_FONT_SIZE < 1) {
			FREETEXT_BOOKMARK_FONT_SIZE = 1;
		}
		widget->update_selected_bookmark_font_size();
	}

	std::string get_name() {
		return "decrease_freetext_font_size";
	}
};

class GotoHighlightCommand : public Command {
public:
	GotoHighlightCommand(MainWidget* w) : Command(w) {};

	void perform() {
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
public:
	GotoHighlightGlobalCommand(MainWidget* w) : Command(w) {};
	void perform() {
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
public:
	GotoTableOfContentsCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_goto_toc();
	}

	std::string get_name() {
		return "goto_toc";
	}
};

class PortalCommand : public Command {
public:
	PortalCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_portal();
	}

	std::string get_name() {
		return "portal";
	}
};

class ToggleWindowConfigurationCommand : public Command {
public:
	ToggleWindowConfigurationCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->toggle_window_configuration();
	}

	std::string get_name() {
		return "toggle_window_configuration";
	}
	bool requires_document() { return false; }
};

class NextStateCommand : public Command {
public:
	NextStateCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->next_state();
	}

	std::string get_name() {
		return "next_state";
	}
	bool requires_document() { return false; }
};

class PrevStateCommand : public Command {
public:
	PrevStateCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->prev_state();
	}

	std::string get_name() {
		return "prev_state";
	}
	bool requires_document() { return false; }
};

class AddHighlightCommand : public SymbolCommand {
public:
	AddHighlightCommand(MainWidget* w) : SymbolCommand(w) {};

	void perform() {
		widget->handle_add_highlight(symbol);
	}

	std::string get_name() {
		return "add_highlight";
	}
};

class CommandCommand : public Command {
public:
	CommandCommand(MainWidget* w) : Command(w) {};

	void perform() {
		QStringList command_names = widget->command_manager->get_all_command_names();
		if (!TOUCH_MODE) {

			widget->set_current_widget(new CommandSelector(
				&widget->on_command_done, widget, command_names, widget->input_handler->get_command_key_mappings()));
		}
		else {

			//        TouchListView* tlv = new TouchListView(command_names, widget);
			//        tlv->resize(250, 400);
			//        widget->set_current_widget(tlv);
			TouchCommandSelector* tcs = new TouchCommandSelector(command_names, widget);
			widget->set_current_widget(tcs);
		}

		widget->show_current_widget();

	}

	std::string get_name() {
		return "command";
	}
	bool requires_document() { return false; }
};

class OpenDocumentCommand : public Command {
public:
	OpenDocumentCommand(MainWidget* w) : Command(w) {};

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

	void perform() {
		widget->open_document(file_name);
	}

	std::string get_name() {
		return "open_document";
	}

	bool requires_document() { return false; }
};

class MoveDownCommand : public Command {
public:
	MoveDownCommand(MainWidget* w) : Command(w) {};

	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_vertical_move(rp);
	}

	std::string get_name() {
		return "move_down";
	}

};

class MoveUpCommand : public Command {
public:
	MoveUpCommand(MainWidget* w) : Command(w) {};

	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_vertical_move(-rp);
	}

	std::string get_name() {

		return "move_up";
	}
};

class MoveLeftCommand : public Command {
public:
	MoveLeftCommand(MainWidget* w) : Command(w) {};
	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_horizontal_move(-rp);
	}
	std::string get_name() {

		return "move_left";
	}
};

class MoveRightCommand : public Command {
public:
	MoveRightCommand(MainWidget* w) : Command(w) {};
	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_horizontal_move(rp);
	}
	std::string get_name() {

		return "move_right";
	}
};

class ZoomInCommand : public Command {
public:
	ZoomInCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->main_document_view->zoom_in();
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "zoom_in";
	}
};

class FitToPageWidthCommand : public Command {
public:
	FitToPageWidthCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->fit_to_page_width();
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "fit_to_page_width";
	}
};

class FitToPageWidthSmartCommand : public Command {
public:
	FitToPageWidthSmartCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->main_document_view->fit_to_page_width(true);
		int current_page = widget->get_current_page_number();
		widget->last_smart_fit_page = current_page;
	}

	std::string get_name() {
		return "fit_to_page_width_smart";
	}
};

class FitToPageHeightCommand : public Command {
public:
	FitToPageHeightCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->main_document_view->fit_to_page_height();
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "fit_to_page_height";
	}
};

class FitToPageHeightSmartCommand : public Command {
public:
	FitToPageHeightSmartCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->main_document_view->fit_to_page_height(true);
	}

	std::string get_name() {
		return "fit_to_page_height_smart";
	}
};

class NextPageCommand : public Command {
public:
	NextPageCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->main_document_view->move_pages(std::max(1, num_repeats));
	}
	std::string get_name() {
		return "next_page";
	}
};

class PreviousPageCommand : public Command {
public:
	PreviousPageCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->main_document_view->move_pages(std::min(-1, -num_repeats));
	}

	std::string get_name() {
		return "previous_page";
	}
};

class ZoomOutCommand : public Command {
public:
	ZoomOutCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->zoom_out();
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "zoom_out";
	}
};

class GotoDefinitionCommand : public Command {
public:
	GotoDefinitionCommand(MainWidget* w) : Command(w) {};
	void perform() {
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
public:
	OverviewDefinitionCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->overview_to_definition();
	}

	std::string get_name() {
		return "overview_definition";
	}
};

class PortalToDefinitionCommand : public Command {
public:
	PortalToDefinitionCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->portal_to_definition();
	}

	std::string get_name() {
		return "portak_to_definition";
	}
};

class MoveVisualMarkDownCommand : public Command {
public:
	MoveVisualMarkDownCommand(MainWidget* w) : Command(w) {};
	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->move_visual_mark_command(rp);
	}

	std::string get_name() {
		return "move_visual_mark_down";
	}
};

class MoveVisualMarkUpCommand : public Command {
public:
	MoveVisualMarkUpCommand(MainWidget* w) : Command(w) {};
	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->move_visual_mark_command(-rp);
	}

	std::string get_name() {
		return "move_visual_mark_up";
	}
};

class MoveVisualMarkNextCommand : public Command {
public:
	MoveVisualMarkNextCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->move_visual_mark_next();
	}

	std::string get_name() {
		return "move_visual_mark_next";
	}
};

class MoveVisualMarkPrevCommand : public Command {
public:
	MoveVisualMarkPrevCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->move_visual_mark_prev();
	}

	std::string get_name() {
		return "move_visual_mark_prev";
	}
};


class GotoPageWithPageNumberCommand : public TextCommand {
public:
	GotoPageWithPageNumberCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
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

class EditSelectedBookmarkCommand : public TextCommand {
public:
	std::wstring initial_text;
	float initial_font_size;
	int index = -1;

	EditSelectedBookmarkCommand(MainWidget* w) : TextCommand(w) {};

	void pre_perform() {

		initial_text = widget->doc()->get_bookmarks()[widget->selected_bookmark_index].description;
		initial_font_size = widget->doc()->get_bookmarks()[widget->selected_bookmark_index].font_size;
		index = widget->selected_bookmark_index;

		widget->text_command_line_edit->setText(
			QString::fromStdWString(widget->doc()->get_bookmarks()[widget->selected_bookmark_index].description)
		);
	}

	void on_cancel() {
		widget->doc()->get_bookmarks()[index].description = initial_text;
		widget->doc()->get_bookmarks()[index].font_size = initial_font_size;
	}

	void perform() {
		std::wstring text_ = text.value();
		widget->change_selected_bookmark_text(text_);
	}

	std::string get_name() {
		return "edit_selected_bookmark";
	}

	std::string text_requirement_name() {
		return "Bookmark Text";
	}

	bool pushes_state() {
		return true;
	}
};

class DeletePortalCommand : public Command {
public:
	DeletePortalCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->main_document_view->delete_closest_portal();
		widget->validate_render();
	}

	std::string get_name() {
		return "delete_portal";
	}
};

class DeleteBookmarkCommand : public Command {
public:
	DeleteBookmarkCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->main_document_view->delete_closest_bookmark();
		widget->validate_render();
	}

	std::string get_name() {
		return "delete_bookmark";
	}
};

class DeleteHighlightCommand : public Command {
public:
	DeleteHighlightCommand(MainWidget* w) : Command(w) {};
	void perform() {
        widget->handle_delete_selected_highlight();
	}

	std::string get_name() {
		return "delete_highlight";
	}
};

class GotoPortalCommand : public Command {
public:
	GotoPortalCommand(MainWidget* w) : Command(w) {};
	void perform() {
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
public:
	EditPortalCommand(MainWidget* w) : Command(w) {};
	void perform() {
		std::optional<Portal> link = widget->main_document_view->find_closest_portal();
		if (link) {
			widget->portal_to_edit = link;
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
public:
	OpenPrevDocCommand(MainWidget* w) : Command(w) {};
	void perform() {
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
public:
	OpenDocumentEmbeddedCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->set_current_widget(new FileSelector(
			[widget = widget](std::wstring doc_path) {
				widget->validate_render();
				widget->open_document(doc_path);
			}, widget, ""));
		widget->show_current_widget();
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
public:
	OpenDocumentEmbeddedFromCurrentPathCommand(MainWidget* w) : Command(w) {};
	void perform() {
		std::wstring last_file_name = widget->get_current_file_name().value_or(L"");

		widget->set_current_widget(new FileSelector(
			[widget = widget](std::wstring doc_path) {
				widget->validate_render();
				widget->open_document(doc_path);
			}, widget, QString::fromStdWString(last_file_name)));
		widget->show_current_widget();
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
public:
	CopyCommand(MainWidget* w) : Command(w) {};
	void perform() {
		copy_to_clipboard(widget->get_selected_text());
	}

	std::string get_name() {
		return "copy";
	}
};

class GotoBeginningCommand : public Command {
public:
	GotoBeginningCommand(MainWidget* w) : Command(w) {};
public:
	void perform() {
		if (num_repeats) {
			widget->main_document_view->goto_page(num_repeats - 1 + widget->main_document_view->get_page_offset());
		}
		else {
			widget->main_document_view->set_offset_y(0.0f);
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
	GotoEndCommand(MainWidget* w) : Command(w) {};
public:
	void perform() {
		if (num_repeats > 0) {
			widget->main_document_view->goto_page(num_repeats - 1 + widget->main_document_view->get_page_offset());
		}
		else {
			widget->main_document_view->goto_end();
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
public:
	ToggleFullscreenCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->toggle_fullscreen();
	}
	std::string get_name() {
		return "toggle_fullscreen";
	}

	bool requires_document() { return false; }
};

class ToggleOneWindowCommand : public Command {
public:
	ToggleOneWindowCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->toggle_two_window_mode();
	}
	std::string get_name() {
		return "toggle_one_window";
	}

	bool requires_document() { return false; }
};

class ToggleHighlightCommand : public Command {
public:
	ToggleHighlightCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->opengl_widget->toggle_highlight_links();
	}
	std::string get_name() {
		return "toggle_highlight";
	}

	bool requires_document() { return false; }
};

class ToggleSynctexCommand : public Command {
public:
	ToggleSynctexCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->toggle_synctex_mode();
	}
	std::string get_name() {
		return "toggle_synctex";
	}

	bool requires_document() { return false; }
};

class TurnOnSynctexCommand : public Command {
public:
	TurnOnSynctexCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->set_synctex_mode(true);
	}
	std::string get_name() {
		return "turn_on_synctex";
	}

	bool requires_document() { return false; }
};

class ToggleShowLastCommand : public Command {
public:
	ToggleShowLastCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->should_show_last_command = !widget->should_show_last_command;
	}
	std::string get_name() {
		return "toggle_show_last_command";
	}
};

class ExternalSearchCommand : public SymbolCommand {
public:
	ExternalSearchCommand(MainWidget* w) : SymbolCommand(w) {};
	void perform() {
		if ((symbol >= 'a') && (symbol <= 'z')) {
			if (SEARCH_URLS[symbol - 'a'].size() > 0) {
				search_custom_engine(widget->get_selected_text(), SEARCH_URLS[symbol - 'a']);
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
public:
	OpenSelectedUrlCommand(MainWidget* w) : Command(w) {};
	void perform() {
		open_web_url((widget->get_selected_text()).c_str());
	}
	std::string get_name() {
		return "open_selected_url";
	}
};

class ScreenDownCommand : public Command {
public:
	ScreenDownCommand(MainWidget* w) : Command(w) {};

	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_move_screen(rp);
	}

	std::string get_name() {
		return "screen_down";
	}
};

class ScreenUpCommand : public Command {
public:
	ScreenUpCommand(MainWidget* w) : Command(w) {};

	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->handle_move_screen(-rp);
	}

	std::string get_name() {
		return "screen_up";
	}
};

class NextChapterCommand : public Command {
public:
	NextChapterCommand(MainWidget* w) : Command(w) {};

	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->main_document_view->goto_chapter(rp);
	}

	std::string get_name() {
		return "next_chapter";
	}
};

class PrevChapterCommand : public Command {
public:
	PrevChapterCommand(MainWidget* w) : Command(w) {};

	void perform() {
		int rp = num_repeats == 0 ? 1 : num_repeats;
		widget->main_document_view->goto_chapter(-rp);
	}

	std::string get_name() {
		return "prev_chapter";
	}
};

class ToggleDarkModeCommand : public Command {
public:
	ToggleDarkModeCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->opengl_widget->toggle_dark_mode();
		widget->helper_opengl_widget->toggle_dark_mode();
	}

	std::string get_name() {
		return "toggle_dark_mode";
	}

	bool requires_document() { return false; }
};

class ToggleCustomColorMode : public Command {
public:
	ToggleCustomColorMode(MainWidget* w) : Command(w) {};

	void perform() {
		widget->opengl_widget->toggle_custom_color_mode();
		widget->helper_opengl_widget->toggle_custom_color_mode();
	}

	std::string get_name() {
		return "toggle_custom_color";
	}

	bool requires_document() { return false; }
};

class TogglePresentationModeCommand : public Command {
public:
	TogglePresentationModeCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->toggle_presentation_mode();
	}

	std::string get_name() {
		return "toggle_presentation_mode";
	}

	bool requires_document() { return false; }
};

class TurnOnPresentationModeCommand : public Command {
public:
	TurnOnPresentationModeCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->set_presentation_mode(true);
	}

	std::string get_name() {
		return "turn_on_presentation_mode";
	}

	bool requires_document() { return false; }
};

class ToggleMouseDragMode : public Command {
public:
	ToggleMouseDragMode(MainWidget* w) : Command(w) {};

	void perform() {
		widget->toggle_mouse_drag_mode();
	}

	std::string get_name() {
		return "toggle_mouse_drag_mode";
	}

	bool requires_document() { return false; }
};

class ToggleFreehandDrawingMode : public Command {
public:
	ToggleFreehandDrawingMode(MainWidget* w) : Command(w) {};

	void perform() {
		widget->toggle_freehand_drawing_mode();
	}

	std::string get_name() {
		return "toggle_freehand_drawing_mode";
	}

	bool requires_document() { return false; }
};

class TogglePenDrawingMode : public Command {
public:
	TogglePenDrawingMode(MainWidget* w) : Command(w) {};

	void perform() {
		widget->toggle_pen_drawing_mode();
	}

	std::string get_name() {
		return "toggle_pen_drawing_mode";
	}

	bool requires_document() { return false; }
};

class CloseWindowCommand : public Command {
public:
	CloseWindowCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->close();
	}

	std::string get_name() {
		return "close_window";
	}

	bool requires_document() { return false; }
};

class NewWindowCommand : public Command {
public:
	NewWindowCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_new_window();
	}

	std::string get_name() {
		return "new_window";
	}

	bool requires_document() { return false; }
};

class QuitCommand : public Command {
public:
	QuitCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_close_event();
		QApplication::quit();
	}

	std::string get_name() {
		return "quit";
	}

	bool requires_document() { return false; }
};

class EscapeCommand : public Command {
public:
	EscapeCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_escape();
	}

	std::string get_name() {
		return "escape";
	}

	bool requires_document() { return false; }
};

class TogglePDFAnnotationsCommand : public Command {
public:
	TogglePDFAnnotationsCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->toggle_pdf_annotations();
	}

	std::string get_name() {
		return "toggle_pdf_annotations";
	}

	bool requires_document() { return true; }
};

class OpenLinkCommand : public Command {
public:
	OpenLinkCommand(MainWidget* w) : Command(w) {};
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

	virtual void perform() {
		widget->handle_open_link(text.value());
	}

	void pre_perform() {
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
public:
	OverviewLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

	void perform() {
		widget->handle_overview_link(text.value());
	}

	std::string get_name() {
		return "overview_link";
	}

};

class PortalToLinkCommand : public OpenLinkCommand {
public:
	PortalToLinkCommand(MainWidget* w) : OpenLinkCommand(w) {};

	void perform() {
		widget->handle_portal_to_link(text.value());
	}

	std::string get_name() {
		return "portal_to_link";
	}

};

class CopyLinkCommand : public TextCommand {
public:
	CopyLinkCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
		widget->handle_open_link(text.value(), true);
	}

	void pre_perform() {
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
public:
	KeyboardSelectCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
		widget->handle_keyboard_select(text.value());
	}

	void pre_perform() {
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
public:
	KeyboardOverviewCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
		std::optional<fz_irect> rect_ = widget->get_tag_window_rect(utf8_encode(text.value()));
       if (rect_) {
           fz_irect rect = rect_.value();
           widget->overview_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
           widget->opengl_widget->set_should_highlight_words(false);
       }
	}

	void pre_perform() {
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
public:
	KeyboardSmartjumpCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
	   	std::optional<fz_irect> rect_ = widget->get_tag_window_rect(utf8_encode(text.value()));
       if (rect_) {
           fz_irect rect = rect_.value();
   		widget->smart_jump_under_pos({ (rect.x0 + rect.x1) / 2, (rect.y0 + rect.y1) / 2 });
   		widget->opengl_widget->set_should_highlight_words(false);
       }
	}

	void pre_perform() {
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
public:
	KeysCommand(MainWidget* w) : Command(w) {};

	void perform() {
		open_file(default_keys_path.get_path());
	}

	std::string get_name() {
		return "keys";
	}

	bool requires_document() { return false; }
};

class KeysUserCommand : public Command {
public:
	KeysUserCommand(MainWidget* w) : Command(w) {};

	void perform() {
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
public:
	KeysUserAllCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_keys_user_all();
	}

	std::string get_name() {
		return "keys_user_all";
	}

	bool requires_document() { return false; }
};

class PrefsCommand : public Command {
public:
	PrefsCommand(MainWidget* w) : Command(w) {};

	void perform() {
		open_file(default_config_path.get_path());
	}

	std::string get_name() {
		return "prefs";
	}

	bool requires_document() { return false; }
};

class PrefsUserCommand : public Command {
public:
	PrefsUserCommand(MainWidget* w) : Command(w) {};

	void perform() {
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
public:
	PrefsUserAllCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_prefs_user_all();
	}

	std::string get_name() {
		return "prefs_user_all";
	}

	bool requires_document() { return false; }
};

class FitToPageWidthRatioCommand : public Command {
public:
	FitToPageWidthRatioCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->fit_to_page_width(false, true);
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "fit_to_page_width_ratio";
	}
};

class SmartJumpUnderCursorCommand : public Command {
public:
	SmartJumpUnderCursorCommand(MainWidget* w) : Command(w) {};

	void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->smart_jump_under_pos({ mouse_pos.x(), mouse_pos.y() });
	}

	std::string get_name() {
		return "smart_jump_under_cursor";
	}
};

class DownloadPaperUnderCursorCommand : public Command {
public:
	DownloadPaperUnderCursorCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->download_paper_under_cursor();
	}

	std::string get_name() {
		return "download_paper_under_cursor";
	}
};


class OverviewUnderCursorCommand : public Command {
public:
	OverviewUnderCursorCommand(MainWidget* w) : Command(w) {};

	void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->overview_under_pos({ mouse_pos.x(), mouse_pos.y() });
	}

	std::string get_name() {
		return "overview_under_cursor";
	}
};

class SynctexUnderCursorCommand : public Command {
public:
	SynctexUnderCursorCommand(MainWidget* w) : Command(w) {};

	void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->synctex_under_pos({ mouse_pos.x(), mouse_pos.y() });
	}

	std::string get_name() {
		return "synctex_under_cursor";
	}
};

class VisualMarkUnderCursorCommand : public Command {
public:
	VisualMarkUnderCursorCommand(MainWidget* w) : Command(w) {};

	void perform() {
        QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->visual_mark_under_pos({ mouse_pos.x(), mouse_pos.y() });
	}

	std::string get_name() {
		return "visual_mark_under_cursor";
	}
};

class CloseOverviewCommand : public Command {
public:
	CloseOverviewCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->opengl_widget->set_overview_page({});
	}

	std::string get_name() {
		return "close_overview";
	}
};

class CloseVisualMarkCommand : public Command {
public:
	CloseVisualMarkCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->opengl_widget->set_should_draw_vertical_line(false);
	}

	std::string get_name() {
		return "close_visual_mark";
	}
};

class ZoomInCursorCommand : public Command {
public:
	ZoomInCursorCommand(MainWidget* w) : Command(w) {};

	void perform() {
		QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->main_document_view->zoom_in_cursor({ mouse_pos.x(), mouse_pos.y() });
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "zoom_in_cursor";
	}
};

class ZoomOutCursorCommand : public Command {
public:
	ZoomOutCursorCommand(MainWidget* w) : Command(w) {};

	void perform() {
		QPoint mouse_pos = widget->mapFromGlobal(QCursor::pos());
        widget->main_document_view->zoom_out_cursor({ mouse_pos.x(), mouse_pos.y() });
		widget->last_smart_fit_page = {};
	}

	std::string get_name() {
		return "zoom_out_cursor";
	}
};

class GotoLeftCommand : public Command {
public:
	GotoLeftCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->goto_left();
	}

	std::string get_name() {
		return "goto_left";
	}
};

class GotoLeftSmartCommand : public Command {
public:
	GotoLeftSmartCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->goto_left_smart();
	}

	std::string get_name() {
		return "goto_left_smart";
	}
};

class GotoRightCommand : public Command {
public:
	GotoRightCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->goto_right();
	}

	std::string get_name() {
		return "goto_right";
	}
};

class GotoRightSmartCommand : public Command {
public:
	GotoRightSmartCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->goto_right_smart();
	}

	std::string get_name() {
		return "goto_right_smart";
	}
};

class RotateClockwiseCommand : public Command {
public:
	RotateClockwiseCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->rotate();
        widget->opengl_widget->rotate_clockwise();
	}

	std::string get_name() {
		return "rotate_clockwise";
	}
};

class RotateCounterClockwiseCommand : public Command {
public:
	RotateCounterClockwiseCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->main_document_view->rotate();
        widget->opengl_widget->rotate_counterclockwise();
	}

	std::string get_name() {
		return "rotate_counterclockwise";
	}
};

class GotoNextHighlightCommand : public Command {
public:
	GotoNextHighlightCommand(MainWidget* w) : Command(w) {};

	void perform() {
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
public:
	GotoPrevHighlightCommand(MainWidget* w) : Command(w) {};

	void perform() {

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
public:
	GotoNextHighlightOfTypeCommand(MainWidget* w) : Command(w) {};

	void perform() {
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
public:
	GotoPrevHighlightOfTypeCommand(MainWidget* w) : Command(w) {};

	void perform() {
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
public:
	SetSelectHighlightTypeCommand(MainWidget* w) : SymbolCommand(w) {};
	void perform() {
        widget->select_highlight_type = symbol;
	}

	std::string get_name() {
		return "set_select_highlight_type";
	}

	bool requires_document() { return false; }
};

class SetFreehandType : public SymbolCommand {
public:
	SetFreehandType(MainWidget* w) : SymbolCommand(w) {};
	void perform() {
        widget->current_freehand_type = symbol;
	}

	std::string get_name() {
		return "set_freehand_type";
	}

	bool requires_document() { return false; }
};

class AddHighlightWithCurrentTypeCommand : public Command {
public:
	AddHighlightWithCurrentTypeCommand(MainWidget* w) : Command(w) {};
	void perform() {
        if (widget->main_document_view->selected_character_rects.size() > 0) {
            widget->main_document_view->add_highlight(widget->selection_begin, widget->selection_end, widget->select_highlight_type);
			widget->clear_selected_text();
        }
	}

	std::string get_name() {
		return "add_highlight_with_current_type";
	}
};

class UndoDrawingCommand : public Command {
public:
	UndoDrawingCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->handle_undo_drawing();
	}

	std::string get_name() {
		return "undo_drawing";
	}

	bool requires_document() { return true; }
};


class EnterPasswordCommand : public TextCommand {
public:
	EnterPasswordCommand(MainWidget* w) : TextCommand(w) {};
	void perform() {
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
public:
	ToggleFastreadCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->opengl_widget->toggle_fastread_mode();
	}

	std::string get_name() {
		return "toggle_fastread";
	}
};

class GotoTopOfPageCommand : public Command {
public:
	GotoTopOfPageCommand(MainWidget* w) : Command(w) {};
	void perform() {
        widget->main_document_view->goto_top_of_page();
	}

	std::string get_name() {
		return "goto_top_of_page";
	}
};

class GotoBottomOfPageCommand : public Command {
public:
	GotoBottomOfPageCommand(MainWidget* w) : Command(w) {};
	void perform() {
        widget->main_document_view->goto_bottom_of_page();
	}

	std::string get_name() {
		return "goto_bottom_of_page";
	}
};

class ReloadCommand : public Command {
public:
	ReloadCommand(MainWidget* w) : Command(w) {};
	void perform() {
        widget->reload();
	}

	std::string get_name() {
		return "relaod";
	}
};

class ReloadNoFlickerCommand : public Command {
public:
	ReloadNoFlickerCommand(MainWidget* w) : Command(w) {};
	void perform() {
        widget->reload(false);
	}

	std::string get_name() {
		return "relaod_no_flicker";
	}
};

class ReloadConfigCommand : public Command {
public:
	ReloadConfigCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->on_config_file_changed(widget->config_manager);
	}

	std::string get_name() {
		return "relaod_config";
	}

	bool requires_document() { return false; }
};

class TurnOnAllDrawings : public Command {
public:
	TurnOnAllDrawings(MainWidget* w) : Command(w) {};
	void perform() {
		widget->hande_turn_on_all_drawings();
	}

	std::string get_name() {
		return "turn_on_all_drawings";
	}

	bool requires_document() { return false; }
};

class TurnOffAllDrawings : public Command {
public:
	TurnOffAllDrawings(MainWidget* w) : Command(w) {};
	void perform() {
		widget->hande_turn_off_all_drawings();
	}

	std::string get_name() {
		return "turn_off_all_drawings";
	}

	bool requires_document() { return false; }
};

class SetStatusStringCommand : public TextCommand {
public:
	SetStatusStringCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
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
public:
	ClearStatusStringCommand(MainWidget* w) : Command(w) {};
	void perform() {
        widget->set_status_message(L"");
	}

	std::string get_name() {
		return "clear_status_string";
	}

	bool requires_document() { return false; }
};

class ToggleTittlebarCommand : public Command {
public:
	ToggleTittlebarCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->toggle_titlebar();
	}

	std::string get_name() {
		return "toggle_titlebar";
	}

	bool requires_document() { return false; }
};

class NextPreviewCommand : public Command {
public:
	NextPreviewCommand(MainWidget* w) : Command(w) {};
	void perform() {
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
public:
	PreviousPreviewCommand(MainWidget* w) : Command(w) {};
	void perform() {
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
public:
	GotoOverviewCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->goto_overview();
	}

	std::string get_name() {
		return "goto_overview";
	}
};

class PortalToOverviewCommand : public Command {
public:
	PortalToOverviewCommand(MainWidget* w) : Command(w) {};
	void perform() {
		widget->handle_portal_to_overview();
	}

	std::string get_name() {
		return "portal_to_overview";
	}
};

class GotoSelectedTextCommand : public Command {
public:
	GotoSelectedTextCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->long_jump_to_destination(widget->selection_begin.y);
	}

	std::string get_name() {
		return "goto_selected_text";
	}
};

class FocusTextCommand : public TextCommand {
public:
	FocusTextCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
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
public:
	GotoWindowCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_goto_window();
	}

	std::string get_name() {
		return "goto_window";
	}

	bool requires_document() { return false; }
};

class ToggleSmoothScrollModeCommand : public Command {
public:
	ToggleSmoothScrollModeCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_toggle_smooth_scroll_mode();
	}

	std::string get_name() {
		return "toggle_smooth_scroll_mode";
	}

	bool requires_document() { return false; }
};

class ToggleScrollbarCommand : public Command {
public:
	ToggleScrollbarCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->toggle_scrollbar();
	}

	std::string get_name() {
		return "toggle_scrollbar";
	}
};

class OverviewToPortalCommand : public Command {
public:
	OverviewToPortalCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_overview_to_portal();
	}

	std::string get_name() {
		return "overview_to_portal";
	}
};

class DebugCommand : public Command {
public:
	DebugCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_debug_command();
	}

	std::string get_name() {
		return "debug";
	}
};

class SelectRectCommand : public Command {
public:
	SelectRectCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->set_rect_select_mode(true);
	}

	std::string get_name() {
		return "select_rect";
	}
};

class ToggleTypingModeCommand : public Command {
public:
	ToggleTypingModeCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_toggle_typing_mode();
	}

	std::string get_name() {
		return "toggle_typing_mode";
	}
};

class DonateCommand : public Command {
public:
	DonateCommand(MainWidget* w) : Command(w) {};

	void perform() {
        open_web_url(L"https://www.buymeacoffee.com/ahrm");
	}

	std::string get_name() {
		return "donate";
	}
	bool requires_document() { return false; }
};

class OverviewNextItemCommand : public Command {
public:
	OverviewNextItemCommand(MainWidget* w) : Command(w) {};

	void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->opengl_widget->goto_search_result(num_repeats, true);
	}

	std::string get_name() {
		return "overview_next_item";
	}
};

class OverviewPrevItemCommand : public Command {
public:
	OverviewPrevItemCommand(MainWidget* w) : Command(w) {};

	void perform() {
        if (num_repeats == 0) num_repeats++;
        widget->opengl_widget->goto_search_result(-num_repeats, true);
	}

	std::string get_name() {
		return "overview_prev_item";
	}
};

class DeleteHighlightUnderCursorCommand : public Command {
public:
	DeleteHighlightUnderCursorCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_delete_highlight_under_cursor();
	}

	std::string get_name() {
		return "delete_highlight_under_cursor";
	}
};

class NoopCommand : public Command {
public:

	NoopCommand(MainWidget* widget_) : Command(widget_) {}

	void perform() {
	}

	std::string get_name() {
		return "noop";
	}
	bool requires_document() { return false; }
};

class ImportCommand : public Command {
public:
	ImportCommand(MainWidget* w) : Command(w) {};

	void perform() {
        std::wstring import_file_name = select_json_file_name();
        widget->db_manager->import_json(import_file_name, widget->checksummer);
	}

	std::string get_name() {
		return "import";
	}
	bool requires_document() { return false; }
};

class ExportCommand : public Command {
public:
	ExportCommand(MainWidget* w) : Command(w) {};

	void perform() {
        std::wstring export_file_name = select_new_json_file_name();
        widget->db_manager->export_json(export_file_name, widget->checksummer);
	}

	std::string get_name() {
		return "export";
	}
	bool requires_document() { return false; }
};

class EnterVisualMarkModeCommand : public Command {
public:
	EnterVisualMarkModeCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->visual_mark_under_pos({ widget->width()/2, widget->height()/2});
	}

	std::string get_name() {
		return "enter_visual_mark_mode";
	}
};

class SetPageOffsetCommand : public TextCommand {
public:
	SetPageOffsetCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
        if (is_string_numeric(text.value().c_str()) && text.value().size() < 6) { // make sure the page number is valid
            widget->main_document_view->set_page_offset(std::stoi(text.value().c_str()));
        }
	}

	std::string get_name() {
		return "set_page_offset";
	}
};

class ToggleVisualScrollCommand : public Command {
public:
	ToggleVisualScrollCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->toggle_visual_scroll_mode();
	}

	std::string get_name() {
		return "toggle_visual_scroll";
	}
};

class ToggleHorizontalLockCommand : public Command {
public:
	ToggleHorizontalLockCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->horizontal_scroll_locked = !widget->horizontal_scroll_locked;
	}

	std::string get_name() {
		return "toggle_horizontal_scroll_lock";
	}
};

class ExecuteCommand : public TextCommand {
public:
	ExecuteCommand(MainWidget* w) : TextCommand(w) {};

	void perform() {
        widget->execute_command(text.value());
	}

	std::string get_name() {
		return "execute";
	}
	bool requires_document() { return false; }
};

class ImportAnnotationsCommand : public Command {
public:
	ImportAnnotationsCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->doc()->import_annotations();
	}

	std::string get_name() {
		return "import_annotations";
	}
};

class EmbedAnnotationsCommand : public Command {
public:
	EmbedAnnotationsCommand(MainWidget* w) : Command(w) {};

	void perform() {
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
public:
	CopyWindowSizeConfigCommand(MainWidget* w) : Command(w) {};

	void perform() {
        copy_to_clipboard(widget->get_window_configuration_string());
	}

	std::string get_name() {
		return "copy_window_size_config";
	}
	bool requires_document() { return false; }
};

class ToggleSelectHighlightCommand : public Command {
public:
	ToggleSelectHighlightCommand(MainWidget* w) : Command(w) {};

	void perform() {
        widget->is_select_highlight_mode = !widget->is_select_highlight_mode;
	}

	std::string get_name() {
		return "toggle_select_highlight";
	}
};

class OpenLastDocumentCommand : public Command {
public:
	OpenLastDocumentCommand(MainWidget* w) : Command(w) {};

	void perform() {
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

class AddMarkedDataCommand : public Command {
public:
	AddMarkedDataCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_add_marked_data();
	}

	std::string get_name() {
		return "add_marked_data";
	}

	bool requires_document() { return true; }
};

class UndoMarkedDataCommand : public Command {
public:
	UndoMarkedDataCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_undo_marked_data();
	}

	std::string get_name() {
		return "undo_marked_data";
	}

	bool requires_document() { return true; }
};

class GotoRandomPageCommand : public Command {
public:
	GotoRandomPageCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_goto_random_page();
	}

	std::string get_name() {
		return "goto_random_page";
	}

	bool requires_document() { return true; }
};

class RemoveMarkedDataCommand : public Command {
public:
	RemoveMarkedDataCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_remove_marked_data();
	}

	std::string get_name() {
		return "remove_marked_data";
	}

	bool requires_document() { return true; }
};

class ExportMarkedDataCommand : public Command {
public:
	ExportMarkedDataCommand(MainWidget* w) : Command(w) {};

	void perform() {
		widget->handle_export_marked_data();
	}

	std::string get_name() {
		return "export_marked_data";
	}

	bool requires_document() { return true; }
};

class ToggleStatusbarCommand : public Command {
public:
	ToggleStatusbarCommand(MainWidget* w) : Command(w) {};

	void perform() {
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
			actual_command = std::move(command_manager->get_command_with_name(widget, command_name));
			if (!actual_command) return &noop;

			auto req = actual_command->next_requirement(widget);
			if (command_params.size() > 0) {
				// set the params if command was called with parameters, for example
				// add_bookmark(some text)
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
			}
			return actual_command.get();
		}
	}


public:
	LazyCommand(MainWidget* widget_, CommandManager* manager, std::wstring command_text) : Command(widget_), noop(widget_) {
		command_manager = manager;
		parse_command_text(command_text);
	}

	void set_text_requirement(std::wstring value) { get_command()->set_text_requirement(value); }
	void set_symbol_requirement(char value) { get_command()->set_symbol_requirement(value); }
	void set_file_requirement(std::wstring value) { get_command()->set_file_requirement(value); }
	void set_rect_requirement(fz_rect value) { get_command()->set_rect_requirement(value); }
	void set_point_requirement(AbsoluteDocumentPos value) { get_command()->set_point_requirement(value); }
	void set_num_repeats(int nr) { get_command()->set_num_repeats(nr); }
	std::vector<char> special_symbols() { return get_command()->special_symbols(); }
	void pre_perform() { get_command()->pre_perform(); }
	bool pushes_state() { return get_command()->pushes_state(); }
	bool requires_document() { return get_command()->requires_document(); }
	std::optional<Requirement> next_requirement(MainWidget* widget) {
		return get_command()->next_requirement(widget);
	}

	virtual void perform() {
		auto com = get_command();
		if (com) {
			com->run();
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


class DeleteFreehandDrawingsCommand: public Command {
public:
	DeleteFreehandDrawingsCommand(MainWidget* w) : Command(w) {};

	std::optional<fz_rect> rect_;
	DrawingMode original_drawing_mode = DrawingMode::None;


	void pre_perform() {
		original_drawing_mode = widget->freehand_drawing_mode;
		widget->freehand_drawing_mode = DrawingMode::None;
	}

	std::optional<Requirement> next_requirement(MainWidget* widget) {

		if (!rect_.has_value()) {
			Requirement req = { RequirementType::Rect, "Command Rect"};
			return req;
		}
		return {};
	}

	void set_rect_requirement(fz_rect rect) {
		if (rect.x0 > rect.x1) {
			std::swap(rect.x0, rect.x1);
		}
		if (rect.y0 > rect.y1) {
			std::swap(rect.y0, rect.y1);
		}

		rect_ = rect;
	}

	void perform() {
		widget->delete_freehand_drawings(rect_.value());
		widget->freehand_drawing_mode = original_drawing_mode;
	}

	std::string get_name() {
		return "delete_freehand_drawings";
	}
};

class CustomCommand : public Command {

	std::wstring raw_command;
	std::string name;
	std::optional<fz_rect> command_rect;
	std::optional<AbsoluteDocumentPos> command_point;
	std::optional<std::wstring> command_text;

public:

	CustomCommand(MainWidget* widget_, std::string name_, std::wstring command_) : Command(widget_) {
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

	void set_point_requirement(AbsoluteDocumentPos point) {
		command_point = point;
	}

	void set_text_requirement(std::wstring txt) {
		command_text = txt;
	}

	void perform() {
		widget->execute_command(raw_command, command_text.value_or(L""));
	}

	std::string get_name() {
		return name;
	}

};


//class ConfigCommand : public TextCommand {
//	std::string config_name;
//public:
//	ConfigCommand(std::string config_name_) {
//		config_name = config_name_;
//	}
//
//	void perform(MainWidget* widget) {
//        widget->config_manager->deserialize_config(config_name, text.value());
//	}
//
//	std::string get_name() {
//		return "setconfig_" + config_name;
//	}
//	
//	bool requires_document() { return false; }
//};

class ConfigCommand : public Command {
    std::string config_name;
	std::optional<std::wstring> text = {};
	ConfigManager* config_manager;
public:
    ConfigCommand(MainWidget* widget_, std::string config_name_, ConfigManager* config_manager_) : Command(widget_), config_manager(config_manager_){
        config_name = config_name_;
    }

	void set_text_requirement(std::wstring value) {
		text = value;
	}

	std::wstring get_text_default_value() {
		Config* config = config_manager->get_mut_config_with_name(utf8_decode(config_name));
		std::wstringstream config_stream;
		config->serialize(config->value, config_stream);
		std::wstring default_value = config_stream.str();

		return default_value;
	}

	std::optional<Requirement> next_requirement(MainWidget* widget) {
		if (TOUCH_MODE) {
			Config* config = config_manager->get_mut_config_with_name(utf8_decode(config_name));
			if ((!text.has_value()) && config->config_type == ConfigType::String) {
				Requirement res;
				res.type = RequirementType::Text;
				res.name = "Config Value";
				return res;
			}
			return {};
		}
		else{
			if (text.has_value()) {
				return {};
			}
			else {

				Requirement res;
				res.type = RequirementType::Text;
				res.name = "Config Value";
				return res;
			}
		}
	}

    void perform() {

		if (TOUCH_MODE) {
			Config* config = widget->config_manager->get_mut_config_with_name(utf8_decode(config_name));


			if (config->config_type == ConfigType::String) {
				if (widget->config_manager->deserialize_config(config_name, text.value())) {
					widget->on_config_changed(config_name);
				}
			}
			if (config->config_type == ConfigType::Color3) {
				widget->push_current_widget(new Color3ConfigUI(config_name, widget, (float*)config->value));
				widget->show_current_widget();
			}

			if (config->config_type == ConfigType::Color4) {
				widget->push_current_widget(new Color4ConfigUI(config_name, widget, (float*)config->value));
				widget->show_current_widget();
			}
			if (config->config_type == ConfigType::Bool) {
				widget->push_current_widget(new BoolConfigUI(config_name, widget, (bool*)config->value, QString::fromStdWString(config->name)));
				widget->show_current_widget();
			}
			if (config->config_type == ConfigType::EnableRectangle) {
				widget->push_current_widget(new RectangleConfigUI(config_name, widget, (UIRect*)config->value));
				widget->show_current_widget();
			}
			if (config->config_type == ConfigType::Float) {
				FloatExtras extras = std::get<FloatExtras>(config->extras);
				widget->push_current_widget(new FloatConfigUI(config_name, widget, (float*)config->value, extras.min_val, extras.max_val));
				widget->show_current_widget();

			}
			if (config->config_type == ConfigType::Int) {
				IntExtras extras = std::get<IntExtras>(config->extras);
				widget->push_current_widget(new IntConfigUI(config_name, widget, (int*)config->value, extras.min_val, extras.max_val));
				widget->show_current_widget();
			}

			//        config->serialize
		}
		else {

			if (widget->config_manager->deserialize_config(config_name, text.value())) {
				widget->on_config_changed(config_name);
			}
		}
    }

    std::string get_name() {
        return "setconfig_" + config_name;
    }

    bool requires_document() { return false; }
};


class MacroCommand : public Command {
	std::vector<std::unique_ptr<Command>> commands;
	std::vector<std::string> modes;
	//std::wstring commands;
	std::string name;
	std::wstring raw_commands;
	CommandManager* command_manager;
	bool is_modal = false;

public:
	//MacroCommand(std::string name_, std::vector<std::unique_ptr<NewCommand>> commands_) {

	MacroCommand(MainWidget* widget_, CommandManager* manager, std::string name_, std::wstring commands_) : Command(widget_){
		//commands = std::move(commands_);
		command_manager = manager;
		name = name_;
		raw_commands = commands_;


		auto parts = QString::fromStdWString(commands_).split(';');
		for (int i = 0; i < parts.size(); i++) {
			if (parts.at(i).size() > 0) {

				if (parts.at(i).at(0) == '[') {
					is_modal = true;
				}

				if (!is_modal) {
					commands.push_back(std::make_unique<LazyCommand>(widget_, manager, parts.at(i).toStdWString()));
				}
				else {
					int closed_bracket_index = parts.at(i).indexOf(']');
					if (closed_bracket_index > 0) {
						QString mode_string = parts.at(i).mid(1, closed_bracket_index - 1);
						QString command_string = parts.at(i).mid(closed_bracket_index + 1);
						commands.push_back(std::make_unique<LazyCommand>(widget_, manager, command_string.toStdWString()));
						modes.push_back(mode_string.toStdString());
					}
				}

			}
		}

	}


	void set_text_requirement(std::wstring value) {
		if (is_modal) {
			int current_mode_index = get_current_mode_index();
			if (current_mode_index >= 0) {
				commands[current_mode_index]->set_text_requirement(value);
			}
		}
		else {
			for (int i = 0; i < commands.size(); i++) {
				std::optional<Requirement> req = commands[i]->next_requirement(widget);
				if (req) {
					if (req.value().type == RequirementType::Text) {
						commands[i]->set_text_requirement(value);
					}
					return;
				}
			}
		}
	}

	void set_symbol_requirement(char value) {
		if (is_modal) {
			int current_mode_index = get_current_mode_index();
			if (current_mode_index >= 0) {
				commands[current_mode_index]->set_symbol_requirement(value);
			}
		}
		else {

			for (int i = 0; i < commands.size(); i++) {
				std::optional<Requirement> req = commands[i]->next_requirement(widget);
				if (req) {
					if (req.value().type == RequirementType::Symbol) {
						commands[i]->set_symbol_requirement(value);
					}
					return;
				}
			}
		}
	}

	void set_file_requirement(std::wstring value) {
		if (is_modal) {
			int current_mode_index = get_current_mode_index();
			if (current_mode_index >= 0) {
				commands[current_mode_index]->set_file_requirement(value);
			}
		}
		else {
			for (int i = 0; i < commands.size(); i++) {
				std::optional<Requirement> req = commands[i]->next_requirement(widget);
				if (req) {
					if (req.value().type == RequirementType::File) {
						commands[i]->set_file_requirement(value);
					}
					return;
				}
			}
		}
	}

	void set_rect_requirement(fz_rect value) {
		if (is_modal) {
			int current_mode_index = get_current_mode_index();
			if (current_mode_index >= 0) {
				commands[current_mode_index]->set_rect_requirement(value);
			}
		}
		else {

			for (int i = 0; i < commands.size(); i++) {
				std::optional<Requirement> req = commands[i]->next_requirement(widget);
				if (req) {
					if (req.value().type == RequirementType::Rect) {
						commands[i]->set_rect_requirement(value);
					}
					return;
				}
			}
		}
	}

	std::optional<Requirement> next_requirement(MainWidget* widget) {
		if (is_modal && (modes.size() != commands.size())) {
			std::wcout << L"Invalid modal command : " << raw_commands;
			return {};
		}

		if (is_modal) {
			int current_mode_index = get_current_mode_index();
			if (current_mode_index >= 0) {
				return commands[current_mode_index]->next_requirement(widget);
			}
			return {};
		}
		else {
			for (int i = 0; i < commands.size(); i++) {
				if (commands[i]->next_requirement(widget)) {
					return commands[i]->next_requirement(widget);
				}
			}
			return {};
		}
	}


	bool is_enabled() {
		if (!is_modal) {
			return commands.size() > 0;
		}
		else {
			std::string mode_string = widget->get_current_mode_string();
			for (int i = 0; i < modes.size(); i++) {
				if (mode_matches(mode_string, modes[i])) {
					return true;
				}
			}
			return false;
		}

	}
	void perform() {
		if (!is_modal) {
			for (std::unique_ptr<Command>& subcommand : commands) {
				subcommand->run();
			}
		}
		else {
			if (modes.size() != commands.size()) {
				qDebug() << "Invalid modal command : " << QString::fromStdWString(raw_commands);
				return;
			}
			std::string mode_string = widget->get_current_mode_string();

			for (int i = 0; i < commands.size(); i++) {
				if (mode_matches(mode_string, modes[i])) {
					widget->handle_command_types(std::move(commands[i]), 1);
					//commands[i]->run(widget);
					return;
				}
			}
		}
	}

	int get_current_mode_index() {
		if (is_modal) {
			std::string mode_str = widget->get_current_mode_string();
			for (int i = 0; i < commands.size(); i++) {
				if (mode_matches(mode_str, modes[i])) {
					return i;
				}
			}
			return -1;

		}

		return -1;
	}

	bool mode_matches(std::string current_mode, std::string command_mode) {
		for (auto c : command_mode) {
			if (current_mode.find(c) == std::string::npos) {
				return false;
			}
		}
		return true;
	}

	std::string get_name() {
		return name;
	}
};

CommandManager::CommandManager(ConfigManager* config_manager) {

	new_commands["goto_beginning"] = [](MainWidget* widget) {return std::make_unique< GotoBeginningCommand>(widget); };
	new_commands["goto_end"] = [](MainWidget* widget) {return std::make_unique< GotoEndCommand>(widget); };
	new_commands["goto_definition"] = [](MainWidget* widget) {return std::make_unique< GotoDefinitionCommand>(widget); };
	new_commands["overview_definition"] = [](MainWidget* widget) {return std::make_unique< OverviewDefinitionCommand>(widget); };
	new_commands["portal_to_definition"] = [](MainWidget* widget) {return std::make_unique< PortalToDefinitionCommand>(widget); };
	new_commands["goto_tab"] = [](MainWidget* widget) {return std::make_unique< GotoLoadedDocumentCommand>(widget); };
	new_commands["next_item"] = [](MainWidget* widget) {return std::make_unique< NextItemCommand>(widget); };
	new_commands["previous_item"] = [](MainWidget* widget) {return std::make_unique< PrevItemCommand>(widget); };
	new_commands["toggle_text_mark"] = [](MainWidget* widget) {return std::make_unique< ToggleTextMarkCommand>(widget); };
	new_commands["move_text_mark_forward"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkForwardCommand>(widget); };
	new_commands["move_text_mark_backward"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkBackwardCommand>(widget); };
	new_commands["move_text_mark_forward_word"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkForwardWordCommand>(widget); };
	new_commands["move_text_mark_backward_word"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkBackwardWordCommand>(widget); };
	new_commands["move_text_mark_down"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkDownCommand>(widget); };
	new_commands["move_text_mark_up"] = [](MainWidget* widget) {return std::make_unique< MoveTextMarkUpCommand>(widget); };
	new_commands["set_mark"] = [](MainWidget* widget) {return std::make_unique< SetMark>(widget); };
	new_commands["toggle_drawing_mask"] = [](MainWidget* widget) {return std::make_unique< ToggleDrawingMask>(widget); };
	new_commands["turn_on_all_drawings"] = [](MainWidget* widget) {return std::make_unique< TurnOnAllDrawings>(widget); };
	new_commands["turn_off_all_drawings"] = [](MainWidget* widget) {return std::make_unique< TurnOffAllDrawings>(widget); };
	new_commands["goto_mark"] = [](MainWidget* widget) {return std::make_unique< GotoMark>(widget); };
	new_commands["goto_page_with_page_number"] = [](MainWidget* widget) {return std::make_unique< GotoPageWithPageNumberCommand>(widget); };
	new_commands["edit_selected_bookmark"] = [](MainWidget* widget) {return std::make_unique< EditSelectedBookmarkCommand>(widget); };
	new_commands["search"] = [](MainWidget* widget) {return std::make_unique< SearchCommand>(widget); };
	new_commands["add_annot_to_highlight"] = [](MainWidget* widget) {return std::make_unique< AddAnnotationToSelectedHighlightCommand>(widget); };
	new_commands["set_freehand_thickness"] = [](MainWidget* widget) {return std::make_unique< SetFreehandThickness>(widget); };
	new_commands["goto_page_with_label"] = [](MainWidget* widget) {return std::make_unique< GotoPageWithLabel>(widget); };
	new_commands["regex_search"] = [](MainWidget* widget) {return std::make_unique< RegexSearchCommand>(widget); };
	new_commands["chapter_search"] = [](MainWidget* widget) {return std::make_unique< ChapterSearchCommand>(widget); };
	new_commands["move_down"] = [](MainWidget* widget) {return std::make_unique< MoveDownCommand>(widget); };
	new_commands["move_up"] = [](MainWidget* widget) {return std::make_unique< MoveUpCommand>(widget); };
	new_commands["move_left"] = [](MainWidget* widget) {return std::make_unique< MoveLeftCommand>(widget); };
	new_commands["move_right"] = [](MainWidget* widget) {return std::make_unique< MoveRightCommand>(widget); };
	new_commands["zoom_in"] = [](MainWidget* widget) {return std::make_unique< ZoomInCommand>(widget); };
	new_commands["zoom_out"] = [](MainWidget* widget) {return std::make_unique< ZoomOutCommand>(widget); };
	new_commands["fit_to_page_width"] = [](MainWidget* widget) {return std::make_unique< FitToPageWidthCommand>(widget); };
	new_commands["fit_to_page_height"] = [](MainWidget* widget) {return std::make_unique< FitToPageHeightCommand>(widget); };
	new_commands["fit_to_page_height_smart"] = [](MainWidget* widget) {return std::make_unique< FitToPageHeightSmartCommand>(widget); };
	new_commands["fit_to_page_width_smart"] = [](MainWidget* widget) {return std::make_unique< FitToPageWidthSmartCommand>(widget); };
	new_commands["next_page"] = [](MainWidget* widget) {return std::make_unique< NextPageCommand>(widget); };
	new_commands["previous_page"] = [](MainWidget* widget) {return std::make_unique< PreviousPageCommand>(widget); };
	new_commands["open_document"] = [](MainWidget* widget) {return std::make_unique< OpenDocumentCommand>(widget); };
	new_commands["add_bookmark"] = [](MainWidget* widget) {return std::make_unique< AddBookmarkCommand>(widget); };
	new_commands["add_marked_bookmark"] = [](MainWidget* widget) {return std::make_unique< AddBookmarkMarkedCommand>(widget); };
	new_commands["add_freetext_bookmark"] = [](MainWidget* widget) {return std::make_unique< AddBookmarkFreetextCommand>(widget); };
	new_commands["add_highlight"] = [](MainWidget* widget) {return std::make_unique< AddHighlightCommand>(widget); };
	new_commands["goto_toc"] = [](MainWidget* widget) {return std::make_unique< GotoTableOfContentsCommand>(widget); };
	new_commands["goto_highlight"] = [](MainWidget* widget) {return std::make_unique< GotoHighlightCommand>(widget); };
	new_commands["increase_freetext_font_size"] = [](MainWidget* widget) {return std::make_unique< IncreaseFreetextBookmarkFontSizeCommand>(widget); };
	new_commands["decrease_freetext_font_size"] = [](MainWidget* widget) {return std::make_unique< DecreaseFreetextBookmarkFontSizeCommand>(widget); };
	new_commands["goto_bookmark"] = [](MainWidget* widget) {return std::make_unique< GotoBookmarkCommand>(widget); };
	new_commands["goto_bookmark_g"] = [](MainWidget* widget) {return std::make_unique< GotoBookmarkGlobalCommand>(widget); };
	new_commands["goto_highlight_g"] = [](MainWidget* widget) {return std::make_unique< GotoHighlightGlobalCommand>(widget); };
	new_commands["link"] = [](MainWidget* widget) {return std::make_unique< PortalCommand>(widget); };
	new_commands["portal"] = [](MainWidget* widget) {return std::make_unique< PortalCommand>(widget); };
	new_commands["next_state"] = [](MainWidget* widget) {return std::make_unique< NextStateCommand>(widget); };
	new_commands["prev_state"] = [](MainWidget* widget) {return std::make_unique< PrevStateCommand>(widget); };
	new_commands["delete_link"] = [](MainWidget* widget) {return std::make_unique< DeletePortalCommand>(widget); };
	new_commands["delete_portal"] = [](MainWidget* widget) {return std::make_unique< DeletePortalCommand>(widget); };
	new_commands["delete_bookmark"] = [](MainWidget* widget) {return std::make_unique< DeleteBookmarkCommand>(widget); };
	new_commands["delete_highlight"] = [](MainWidget* widget) {return std::make_unique< DeleteHighlightCommand>(widget); };
	new_commands["goto_link"] = [](MainWidget* widget) {return std::make_unique< GotoPortalCommand>(widget); };
	new_commands["goto_portal"] = [](MainWidget* widget) {return std::make_unique< GotoPortalCommand>(widget); };
	new_commands["edit_link"] = [](MainWidget* widget) {return std::make_unique< EditPortalCommand>(widget); };
	new_commands["edit_portal"] = [](MainWidget* widget) {return std::make_unique< EditPortalCommand>(widget); };
	new_commands["open_prev_doc"] = [](MainWidget* widget) {return std::make_unique< OpenPrevDocCommand>(widget); };
	new_commands["open_document_embedded"] = [](MainWidget* widget) {return std::make_unique< OpenDocumentEmbeddedCommand>(widget); };
	new_commands["open_document_embedded_from_current_path"] = [](MainWidget* widget) {return std::make_unique< OpenDocumentEmbeddedFromCurrentPathCommand>(widget); };
	new_commands["copy"] = [](MainWidget* widget) {return std::make_unique< CopyCommand>(widget); };
	new_commands["toggle_fullscreen"] = [](MainWidget* widget) {return std::make_unique< ToggleFullscreenCommand>(widget); };
	new_commands["toggle_one_window"] = [](MainWidget* widget) {return std::make_unique< ToggleOneWindowCommand>(widget); };
	new_commands["toggle_highlight"] = [](MainWidget* widget) {return std::make_unique< ToggleHighlightCommand>(widget); };
	new_commands["toggle_synctex"] = [](MainWidget* widget) {return std::make_unique< ToggleSynctexCommand>(widget); };
	new_commands["turn_on_synctex"] = [](MainWidget* widget) {return std::make_unique< TurnOnSynctexCommand>(widget); };
	new_commands["toggle_show_last_command"] = [](MainWidget* widget) {return std::make_unique< ToggleShowLastCommand>(widget); };
	new_commands["command"] = [](MainWidget* widget) {return std::make_unique< CommandCommand>(widget); };
	new_commands["external_search"] = [](MainWidget* widget) {return std::make_unique< ExternalSearchCommand>(widget); };
	new_commands["open_selected_url"] = [](MainWidget* widget) {return std::make_unique< OpenSelectedUrlCommand>(widget); };
	new_commands["screen_down"] = [](MainWidget* widget) {return std::make_unique< ScreenDownCommand>(widget); };
	new_commands["screen_up"] = [](MainWidget* widget) {return std::make_unique< ScreenUpCommand>(widget); };
	new_commands["next_chapter"] = [](MainWidget* widget) {return std::make_unique< NextChapterCommand>(widget); };
	new_commands["prev_chapter"] = [](MainWidget* widget) {return std::make_unique< PrevChapterCommand>(widget); };
	new_commands["toggle_dark_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleDarkModeCommand>(widget); };
	new_commands["toggle_presentation_mode"] = [](MainWidget* widget) {return std::make_unique< TogglePresentationModeCommand>(widget); };
	new_commands["turn_on_presentation_mode"] = [](MainWidget* widget) {return std::make_unique< TurnOnPresentationModeCommand>(widget); };
	new_commands["toggle_mouse_drag_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleMouseDragMode>(widget); };
	new_commands["toggle_freehand_drawing_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleFreehandDrawingMode>(widget); };
	new_commands["toggle_pen_drawing_mode"] = [](MainWidget* widget) {return std::make_unique< TogglePenDrawingMode>(widget); };
	new_commands["close_window"] = [](MainWidget* widget) {return std::make_unique< CloseWindowCommand>(widget); };
	new_commands["quit"] = [](MainWidget* widget) {return std::make_unique< QuitCommand>(widget); };
	new_commands["escape"] = [](MainWidget* widget) {return std::make_unique< EscapeCommand>(widget); };
	new_commands["toggle_pdf_annotations"] = [](MainWidget* widget) {return std::make_unique< TogglePDFAnnotationsCommand>(widget); };
	new_commands["q"] = [](MainWidget* widget) {return std::make_unique< CloseWindowCommand>(widget); };
	new_commands["open_link"] = [](MainWidget* widget) {return std::make_unique< OpenLinkCommand>(widget); };
	new_commands["overview_link"] = [](MainWidget* widget) {return std::make_unique< OverviewLinkCommand>(widget); };
	new_commands["portal_to_link"] = [](MainWidget* widget) {return std::make_unique< PortalToLinkCommand>(widget); };
	new_commands["copy_link"] = [](MainWidget* widget) {return std::make_unique< CopyLinkCommand>(widget); };
	new_commands["keyboard_select"] = [](MainWidget* widget) {return std::make_unique< KeyboardSelectCommand>(widget); };
	new_commands["keyboard_smart_jump"] = [](MainWidget* widget) {return std::make_unique< KeyboardSmartjumpCommand>(widget); };
	new_commands["keyboard_overview"] = [](MainWidget* widget) {return std::make_unique< KeyboardOverviewCommand>(widget); };
	new_commands["keys"] = [](MainWidget* widget) {return std::make_unique< KeysCommand>(widget); };
	new_commands["keys_user"] = [](MainWidget* widget) {return std::make_unique< KeysUserCommand>(widget); };
	new_commands["prefs"] = [](MainWidget* widget) {return std::make_unique< PrefsCommand>(widget); };
	new_commands["prefs_user"] = [](MainWidget* widget) {return std::make_unique< PrefsUserCommand>(widget); };
	new_commands["move_visual_mark_down"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkDownCommand>(widget); };
	new_commands["move_visual_mark_up"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkUpCommand>(widget); };
	new_commands["move_visual_mark_next"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkNextCommand>(widget); };
	new_commands["move_visual_mark_prev"] = [](MainWidget* widget) {return std::make_unique< MoveVisualMarkPrevCommand>(widget); };
	new_commands["toggle_custom_color"] = [](MainWidget* widget) {return std::make_unique< ToggleCustomColorMode>(widget); };
	new_commands["set_select_highlight_type"] = [](MainWidget* widget) {return std::make_unique< SetSelectHighlightTypeCommand>(widget); };
	new_commands["set_freehand_type"] = [](MainWidget* widget) {return std::make_unique< SetFreehandType>(widget); };
	new_commands["toggle_window_configuration"] = [](MainWidget* widget) {return std::make_unique< ToggleWindowConfigurationCommand>(widget); };
	new_commands["prefs_user_all"] = [](MainWidget* widget) {return std::make_unique< PrefsUserAllCommand>(widget); };
	new_commands["keys_user_all"] = [](MainWidget* widget) {return std::make_unique< KeysUserAllCommand>(widget); };
	new_commands["fit_to_page_width_ratio"] = [](MainWidget* widget) {return std::make_unique< FitToPageWidthRatioCommand>(widget); };
	new_commands["smart_jump_under_cursor"] = [](MainWidget* widget) {return std::make_unique< SmartJumpUnderCursorCommand>(widget); };
	new_commands["download_paper_under_cursor"] = [](MainWidget* widget) {return std::make_unique< DownloadPaperUnderCursorCommand>(widget); };
	new_commands["overview_under_cursor"] = [](MainWidget* widget) {return std::make_unique< OverviewUnderCursorCommand>(widget); };
	new_commands["close_overview"] = [](MainWidget* widget) {return std::make_unique< CloseOverviewCommand>(widget); };
	new_commands["visual_mark_under_cursor"] = [](MainWidget* widget) {return std::make_unique< VisualMarkUnderCursorCommand>(widget); };
	new_commands["close_visual_mark"] = [](MainWidget* widget) {return std::make_unique< CloseVisualMarkCommand>(widget); };
	new_commands["zoom_in_cursor"] = [](MainWidget* widget) {return std::make_unique< ZoomInCursorCommand>(widget); };
	new_commands["zoom_out_cursor"] = [](MainWidget* widget) {return std::make_unique< ZoomOutCursorCommand>(widget); };
	new_commands["goto_left"] = [](MainWidget* widget) {return std::make_unique< GotoLeftCommand>(widget); };
	new_commands["goto_left_smart"] = [](MainWidget* widget) {return std::make_unique< GotoLeftSmartCommand>(widget); };
	new_commands["goto_right"] = [](MainWidget* widget) {return std::make_unique< GotoRightCommand>(widget); };
	new_commands["goto_right_smart"] = [](MainWidget* widget) {return std::make_unique< GotoRightSmartCommand>(widget); };
	new_commands["rotate_clockwise"] = [](MainWidget* widget) {return std::make_unique< RotateClockwiseCommand>(widget); };
	new_commands["rotate_counterclockwise"] = [](MainWidget* widget) {return std::make_unique< RotateCounterClockwiseCommand>(widget); };
	new_commands["goto_next_highlight"] = [](MainWidget* widget) {return std::make_unique< GotoNextHighlightCommand>(widget); };
	new_commands["goto_prev_highlight"] = [](MainWidget* widget) {return std::make_unique< GotoPrevHighlightCommand>(widget); };
	new_commands["goto_next_highlight_of_type"] = [](MainWidget* widget) {return std::make_unique< GotoNextHighlightOfTypeCommand>(widget); };
	new_commands["goto_prev_highlight_of_type"] = [](MainWidget* widget) {return std::make_unique< GotoPrevHighlightOfTypeCommand>(widget); };
	new_commands["add_highlight_with_current_type"] = [](MainWidget* widget) {return std::make_unique< AddHighlightWithCurrentTypeCommand>(widget); };
	new_commands["undo_drawing"] = [](MainWidget* widget) {return std::make_unique< UndoDrawingCommand>(widget); };
	new_commands["enter_password"] = [](MainWidget* widget) {return std::make_unique< EnterPasswordCommand>(widget); };
	new_commands["toggle_fastread"] = [](MainWidget* widget) {return std::make_unique< ToggleFastreadCommand>(widget); };
	new_commands["goto_top_of_page"] = [](MainWidget* widget) {return std::make_unique< GotoTopOfPageCommand>(widget); };
	new_commands["goto_bottom_of_page"] = [](MainWidget* widget) {return std::make_unique< GotoBottomOfPageCommand>(widget); };
	new_commands["new_window"] = [](MainWidget* widget) {return std::make_unique< NewWindowCommand>(widget); };
	new_commands["reload"] = [](MainWidget* widget) {return std::make_unique< ReloadCommand>(widget); };
	new_commands["reload_no_flicker"] = [](MainWidget* widget) {return std::make_unique< ReloadNoFlickerCommand>(widget); };
	new_commands["reload_config"] = [](MainWidget* widget) {return std::make_unique< ReloadConfigCommand>(widget); };
	new_commands["synctex_under_cursor"] = [](MainWidget* widget) {return std::make_unique< SynctexUnderCursorCommand>(widget); };
	new_commands["set_status_string"] = [](MainWidget* widget) {return std::make_unique< SetStatusStringCommand>(widget); };
	new_commands["clear_status_string"] = [](MainWidget* widget) {return std::make_unique< ClearStatusStringCommand>(widget); };
	new_commands["toggle_titlebar"] = [](MainWidget* widget) {return std::make_unique< ToggleTittlebarCommand>(widget); };
	new_commands["next_preview"] = [](MainWidget* widget) {return std::make_unique< NextPreviewCommand>(widget); };
	new_commands["previous_preview"] = [](MainWidget* widget) {return std::make_unique< PreviousPreviewCommand>(widget); };
	new_commands["goto_overview"] = [](MainWidget* widget) {return std::make_unique< GotoOverviewCommand>(widget); };
	new_commands["portal_to_overview"] = [](MainWidget* widget) {return std::make_unique< PortalToOverviewCommand>(widget); };
	new_commands["goto_selected_text"] = [](MainWidget* widget) {return std::make_unique< GotoSelectedTextCommand>(widget); };
	new_commands["focus_text"] = [](MainWidget* widget) {return std::make_unique< FocusTextCommand>(widget); };
	new_commands["goto_window"] = [](MainWidget* widget) {return std::make_unique< GotoWindowCommand>(widget); };
	new_commands["toggle_smooth_scroll_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleSmoothScrollModeCommand>(widget); };
	new_commands["goto_begining"] = [](MainWidget* widget) {return std::make_unique< GotoBeginningCommand>(widget); };
	new_commands["toggle_scrollbar"] = [](MainWidget* widget) {return std::make_unique< ToggleScrollbarCommand>(widget); };
	new_commands["overview_to_portal"] = [](MainWidget* widget) {return std::make_unique< OverviewToPortalCommand>(widget); };
	new_commands["select_rect"] = [](MainWidget* widget) {return std::make_unique< SelectRectCommand>(widget); };
	new_commands["toggle_typing_mode"] = [](MainWidget* widget) {return std::make_unique< ToggleTypingModeCommand>(widget); };
	new_commands["donate"] = [](MainWidget* widget) {return std::make_unique< DonateCommand>(widget); };
	new_commands["overview_next_item"] = [](MainWidget* widget) {return std::make_unique< OverviewNextItemCommand>(widget); };
	new_commands["overview_prev_item"] = [](MainWidget* widget) {return std::make_unique< OverviewPrevItemCommand>(widget); };
	new_commands["delete_highlight_under_cursor"] = [](MainWidget* widget) {return std::make_unique< DeleteHighlightUnderCursorCommand>(widget); };
	new_commands["noop"] = [](MainWidget* widget) {return std::make_unique< NoopCommand>(widget); };
	new_commands["import"] = [](MainWidget* widget) {return std::make_unique< ImportCommand>(widget); };
	new_commands["export"] = [](MainWidget* widget) {return std::make_unique< ExportCommand>(widget); };
	new_commands["enter_visual_mark_mode"] = [](MainWidget* widget) {return std::make_unique< EnterVisualMarkModeCommand>(widget); };
	new_commands["set_page_offset"] = [](MainWidget* widget) {return std::make_unique< SetPageOffsetCommand>(widget); };
	new_commands["toggle_visual_scroll"] = [](MainWidget* widget) {return std::make_unique< ToggleVisualScrollCommand>(widget); };
	new_commands["toggle_horizontal_scroll_lock"] = [](MainWidget* widget) {return std::make_unique< ToggleHorizontalLockCommand>(widget); };
	new_commands["execute"] = [](MainWidget* widget) {return std::make_unique< ExecuteCommand>(widget); };
	new_commands["embed_annotations"] = [](MainWidget* widget) {return std::make_unique< EmbedAnnotationsCommand>(widget); };
	new_commands["import_annotations"] = [](MainWidget* widget) {return std::make_unique< ImportAnnotationsCommand>(widget); };
	new_commands["copy_window_size_config"] = [](MainWidget* widget) {return std::make_unique< CopyWindowSizeConfigCommand>(widget); };
	new_commands["toggle_select_highlight"] = [](MainWidget* widget) {return std::make_unique< ToggleSelectHighlightCommand>(widget); };
	new_commands["open_last_document"] = [](MainWidget* widget) {return std::make_unique< OpenLastDocumentCommand>(widget); };
	new_commands["toggle_statusbar"] = [](MainWidget* widget) {return std::make_unique< ToggleStatusbarCommand>(widget); };
	new_commands["start_reading"] = [](MainWidget* widget) {return std::make_unique< StartReadingCommand>(widget); };
	new_commands["stop_reading"] = [](MainWidget* widget) {return std::make_unique< StopReadingCommand>(widget); };
	new_commands["debug"] = [](MainWidget* widget) {return std::make_unique< DebugCommand>(widget); };
	new_commands["add_marked_data"] = [](MainWidget* widget) {return std::make_unique< AddMarkedDataCommand>(widget); };
	new_commands["remove_marked_data"] = [](MainWidget* widget) {return std::make_unique< RemoveMarkedDataCommand>(widget); };
	new_commands["export_marked_data"] = [](MainWidget* widget) {return std::make_unique< ExportMarkedDataCommand>(widget); };
	new_commands["undo_marked_data"] = [](MainWidget* widget) {return std::make_unique< UndoMarkedDataCommand>(widget); };
	new_commands["goto_random_page"] = [](MainWidget* widget) {return std::make_unique< GotoRandomPageCommand>(widget); };
	new_commands["delete_freehand_drawings"] = [](MainWidget* widget) {return std::make_unique< DeleteFreehandDrawingsCommand>(widget); };


	for (auto [command_name_, command_value] : ADDITIONAL_COMMANDS) {
		std::string command_name = utf8_encode(command_name_);
		std::wstring local_command_value = command_value;
		new_commands[command_name] = [command_name, local_command_value, this](MainWidget* w) {return  std::make_unique<CustomCommand>(w, command_name, local_command_value); };
	}

	for (auto [command_name_, macro_value] : ADDITIONAL_MACROS) {
		std::string command_name = utf8_encode(command_name_);
		std::wstring local_macro_value = macro_value;
		new_commands[command_name] = [command_name, local_macro_value, this](MainWidget* w) {return std::make_unique<MacroCommand>(w, this, command_name, local_macro_value); };
	}

	std::vector<Config> configs = config_manager->get_configs();

	for (auto conf : configs) {
		std::string confname = utf8_encode(conf.name);
		std::string config_set_command_name = "setconfig_" + confname;
		//commands.push_back({ config_set_command_name, true, false , false, false, true, {} });
		new_commands[config_set_command_name] = [confname, config_manager](MainWidget* w) {return std::make_unique<ConfigCommand>(w, confname, config_manager); };

	}

}


std::unique_ptr<Command> CommandManager::get_command_with_name(MainWidget* w, std::string name) {

	if (new_commands.find(name) != new_commands.end()) {
		return new_commands[name](w);
	}
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
	CommandManager* command_manager,
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
                        LOG(std::wcout
                            << L"Warning: key defined in " << command_file_names[j]
                            << L":" << command_line_numbers[j]
                            << L" for " << utf8_decode(command_names[j][0])
                            << L" is unreachable, shadowed by final key sequence defined in "
                            << parent_node->defining_file_path
                            << L":" << parent_node->defining_file_line << L"\n");
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
				if ((parent_node->name_.size() == 0) || parent_node->name_[0].compare(command_names[j][0]) != 0) {

					LOG(std::wcout << L"Warning: key defined in " << parent_node->defining_file_path
						<< L":" << parent_node->defining_file_line
						<< L" overwritten by " << command_file_names[j]
						<< L":" << command_line_numbers[j]);
					if (parent_node->name_.size() > 0) {
						LOG(std::wcout << L". Overriding command: " << line 
							<< L": replacing " << utf8_decode(parent_node->name_[0])
							<< L" with " << utf8_decode(command_names[j][0]));
					}
					LOG(std::wcout << L"\n");
				}
			}
			if ((size_t) i == (tokens.size() - 1)) {
				parent_node->is_final = true;
				parent_node->name_.clear();
                parent_node->defining_file_line = command_line_numbers[j];
                parent_node->defining_file_path = command_file_names[j];
				for (size_t k = 0; k < command_names[j].size(); k++) {
					parent_node->name_.push_back(command_names[j][k]);
				}
				if (command_names[j].size() == 1) {
					parent_node->generator = command_manager->new_commands[command_names[j][0]];
				}
				else {
					QStringList command_parts;
					for (int k = 0; k < command_names[j].size(); k++) {
						command_parts.append(QString::fromStdString(command_names[j][k]));
					}
					std::wstring joined_command = command_parts.join(";").toStdWString();
					parent_node->generator = [joined_command, command_manager](MainWidget* w) {return std::make_unique<MacroCommand>(w, command_manager, "", joined_command); };
				}
				//if (command_names[j].size())
			}
			else {
				if (parent_node->is_final && (parent_node->name_.size() > 0)) {
					LOG(std::wcout << L"Warning: unmapping " << utf8_decode(parent_node->name_[0]) << L" because of " << utf8_decode(command_names[j][0]) << L" which uses " << line << L"\n");
				}
				parent_node->is_final = false;
			}

		}
	}

	return root;
}

InputParseTreeNode* parse_lines(
	CommandManager* command_manager,
	const std::vector<std::wstring>& lines,
	const std::vector<std::vector<std::string>>& command_names,
	const std::vector<std::wstring>& command_file_names,
	const std::vector<int>& command_line_numbers
	) {
	// parse key configs into a trie where leaves are annotated with the name of the command

	InputParseTreeNode* root = new InputParseTreeNode;
	root->is_root = true;

	parse_lines(root, command_manager, lines, command_names, command_file_names, command_line_numbers);

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

InputParseTreeNode* parse_key_config_files(CommandManager* command_manager, const Path& default_path,
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

	return parse_lines(command_manager, command_keys, command_names, command_files, command_line_numbers);
}


InputHandler::InputHandler(const Path& default_path, const std::vector<Path>& user_paths, CommandManager* cm) {
	command_manager = cm;
	user_key_paths = user_paths;
	reload_config_files(default_path, user_paths);
}

void InputHandler::reload_config_files(const Path& default_config_path, const std::vector<Path>& user_config_paths)
{
	delete_current_parse_tree(root);

	root = parse_key_config_files(command_manager, default_config_path, user_config_paths);
	current_node = root;
}


bool is_digit(int key) {
	return key >= Qt::Key::Key_0 && key <= Qt::Key::Key_9;
}

std::unique_ptr<Command> InputHandler::handle_key(MainWidget* w, QKeyEvent* key_event, bool shift_pressed, bool control_pressed, bool alt_pressed, int* num_repeats) {
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
			return nullptr;
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
				if (child->generator) {
					return (child->generator.value())(w);
				}
				return nullptr;
				//for (size_t i = 0; i < child->name.size(); i++) {
				//	res.push_back(command_manager->get_command_with_name(child->name[i]));
				//}
				//return res;
			}
			else{
				current_node = child;
				return nullptr;
			}
		}
	}
	LOG(std::wcout << "Warning: invalid command (key:" << (char)key << "); resetting to root" << std::endl);
	number_stack.clear();
	current_node = root;
	return nullptr;
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
		if (thisroot->name_.size() == 1) {
			map[thisroot->name_[0]].push_back(get_key_string_from_tree_node_sequence(prefix));
		}
		else if (thisroot->name_.size() > 1) {
			for (const auto& name : thisroot->name_) {
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
void Command::set_point_requirement(AbsoluteDocumentPos value) {}

std::wstring Command::get_text_default_value() {
	return L"";
}

bool Command::pushes_state() {
	return false;
}

bool Command::requires_document() {
	return true;
}

void Command::set_num_repeats(int nr) {
	num_repeats = nr;
}

void Command::pre_perform() {

}

void Command::on_cancel() {

}

void Command::run() {
	if (this->requires_document() && !(widget->main_document_view_has_document())) {
		return;
	}
	perform();
}

std::vector<char> Command::special_symbols() {
	std::vector<char> res;
	return res;
}


std::string Command::get_name() {
	return "";
}

bool is_macro_command_enabled(Command* command) {
	MacroCommand* macro_command = dynamic_cast<MacroCommand*>(command);
	if (macro_command) {
		return macro_command->is_enabled();
	}

	return false;
}

std::unique_ptr<Command> CommandManager::create_macro_command(MainWidget* w, std::string name, std::wstring macro_string) {
	return std::make_unique<MacroCommand>(w, this, name, macro_string);
}
