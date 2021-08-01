#include "document_view.h"

extern float MOVE_SCREEN_PERCENTAGE;

DocumentView::DocumentView( fz_context* mupdf_context,
	sqlite3* db,
	DocumentManager* document_manager,
	ConfigManager* config_manager) :
	mupdf_context(mupdf_context),
	database(db),
	document_manager(document_manager),
	config_manager(config_manager)
{

}
DocumentView::~DocumentView() {
}

DocumentView::DocumentView(fz_context* mupdf_context,
	sqlite3* db,
	DocumentManager* document_manager,
	ConfigManager* config_manager,
	bool* invalid_flag,
	std::wstring path,
	int view_width,
	int view_height,
	float offset_x,
	float offset_y) :
	DocumentView(mupdf_context,
		db,
		document_manager,
		config_manager)

{
	on_view_size_change(view_width, view_height);
	open_document(path, invalid_flag);
	set_offsets(offset_x, offset_y);
}

float DocumentView::get_zoom_level() {
	return zoom_level;
}

DocumentViewState DocumentView::get_state() {
	DocumentViewState res;

	if (current_document) {
		res.document_path = current_document->get_path();
		res.book_state.offset_x = get_offset_x();
		res.book_state.offset_y = get_offset_y();
		res.book_state.zoom_level = get_zoom_level();
	}
	return res;
}

void DocumentView::set_opened_book_state(const OpenedBookState& state)
{
	set_offsets(state.offset_x, state.offset_y);
	set_zoom_level(state.zoom_level);
}


void DocumentView::handle_escape() {
}

void DocumentView::set_book_state(OpenedBookState state) {
	set_offsets(state.offset_x, state.offset_y);
	set_zoom_level(state.zoom_level);
}
void DocumentView::set_offsets(float new_offset_x, float new_offset_y) {
	if (current_document == nullptr) return;

	int num_pages = current_document->num_pages();
	float max_y_offset = current_document->get_accum_page_height(num_pages-1) + current_document->get_page_height(num_pages-1);
	float min_y_offset = 0;

	if (new_offset_y > max_y_offset) new_offset_y = max_y_offset;
	if (new_offset_y < min_y_offset) new_offset_y = min_y_offset;

	offset_x = new_offset_x;
	offset_y = new_offset_y;
}

Document* DocumentView::get_document() {
	return current_document;
}

//int DocumentView::get_num_search_results() {
//	search_results_mutex.lock();
//	int num = search_results.size();
//	search_results_mutex.unlock();
//	return num;
//}
//
//int DocumentView::get_current_search_result_index() {
//	return current_search_result_index;
//}

std::optional<Link> DocumentView::find_closest_link() {
	if (current_document) {
		return current_document->find_closest_link(offset_y);
	}
	return {};
}

void DocumentView::goto_link(Link* link)
{
	if (link) {
		if (get_document() &&
			get_document()->get_path() == link->dst.document_path) {
			set_opened_book_state(link->dst.book_state);
		}
		else {
			open_document(link->dst.document_path, nullptr);
			set_opened_book_state(link->dst.book_state);
		}
	}
}

void DocumentView::delete_closest_link() {
	if (current_document) {
		current_document->delete_closest_link(offset_y);
	}
}

void DocumentView::delete_closest_bookmark() {
	if (current_document) {
		current_document->delete_closest_bookmark(offset_y);
	}
}

float DocumentView::get_offset_x() {
	return offset_x;
}

float DocumentView::get_offset_y() {
	return offset_y;
}

int DocumentView::get_view_height()
{
	return view_height;
}

int DocumentView::get_view_width()
{
	return view_width;
}

void DocumentView::set_null_document()
{
	current_document = nullptr;
}

void DocumentView::set_offset_x(float new_offset_x) {
	set_offsets(new_offset_x, offset_y);
}

void DocumentView::set_offset_y(float new_offset_y) {
	set_offsets(offset_x, new_offset_y);
}

//void DocumentView::render_highlight_window( GLuint program, fz_rect window_rect) {
//	float quad_vertex_data[] = {
//		window_rect.x0, window_rect.y1,
//		window_rect.x1, window_rect.y1,
//		window_rect.x0, window_rect.y0,
//		window_rect.x1, window_rect.y0
//	};
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//	glUseProgram(program);
//	glEnableVertexAttribArray(0);
//	glEnableVertexAttribArray(1);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//	glDisable(GL_BLEND);
//}

//void DocumentView::render_highlight_absolute( GLuint program, fz_rect absolute_document_rect) {
//	fz_rect window_rect = absolute_to_window_rect(absolute_document_rect);
//	render_highlight_window(program, window_rect);
//}
//
//void DocumentView::render_highlight_document( GLuint program, int page, fz_rect doc_rect) {
//	fz_rect window_rect = document_to_window_rect(page, doc_rect);
//	render_highlight_window(program, window_rect);
//}

std::optional<PdfLink> DocumentView::get_link_in_pos(int view_x, int view_y) {
	if (!current_document) return {};

	float doc_x, doc_y;
	int page;
	window_to_document_pos(view_x, view_y, &doc_x, &doc_y, &page);

	fz_link* links = current_document->get_page_links(page);
	fz_point point = { doc_x, doc_y };
	std::optional<PdfLink> res = {};

	bool found = false;
	while (links != nullptr) {
		if (fz_is_point_inside_rect(point, links->rect)) {
			res = { links->rect, links->uri };
			return res;
		}
		links = links->next;
	}
	return {};
}
void DocumentView::add_mark(char symbol) {
	//assert(current_document);
	if (current_document) {
		current_document->add_mark(symbol, offset_y);
	}
}
void DocumentView::add_bookmark(std::wstring desc) {
	//assert(current_document);
	if (current_document) {
		current_document->add_bookmark(desc, offset_y);
	}
}
void DocumentView::on_view_size_change(int new_width, int new_height) {
	view_width = new_width;
	view_height = new_height;
}

//void DocumentView::absolute_to_window_pos_pixels(float absolute_x, float absolute_y, float* window_x, float* window_y) {
//
//}

void DocumentView::absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y) {
	float half_width = static_cast<float>(view_width) / zoom_level / 2;
	float half_height = static_cast<float>(view_height) / zoom_level / 2;

	*window_x = (absolute_x + offset_x) / half_width;
	*window_y = (-absolute_y + offset_y) / half_height;
}
fz_rect DocumentView::absolute_to_window_rect(fz_rect doc_rect) {
	fz_rect res;
	absolute_to_window_pos(doc_rect.x0, doc_rect.y0, &res.x0, &res.y0);
	absolute_to_window_pos(doc_rect.x1, doc_rect.y1, &res.x1, &res.y1);

	return res;
}
void DocumentView::document_to_window_pos(int page, float doc_x, float doc_y, float* window_x, float* window_y) {

	if (current_document) {
		double doc_rect_y_offset = -current_document->get_accum_page_height(page);

		float half_width = static_cast<float>(view_width) / zoom_level / 2;
		float half_height = static_cast<float>(view_height) / zoom_level / 2;

		*window_y = (doc_rect_y_offset + offset_y - doc_y) / half_height;
		*window_x = (doc_x + offset_x - current_document->get_page_width(page) / 2) / half_width;
	}
}
void DocumentView::document_to_window_pos_in_pixels(int page, float doc_x, float doc_y, int* window_x, int* window_y)
{
	float opengl_window_pos_x, opengl_window_pos_y;
	document_to_window_pos(page, doc_x, doc_y, &opengl_window_pos_x, &opengl_window_pos_y);
	*window_x = static_cast<int>(opengl_window_pos_x * view_width / 2 + view_width / 2);
	*window_y = static_cast<int>(-opengl_window_pos_y * view_height / 2 + view_height / 2);
}
fz_rect DocumentView::document_to_window_rect(int page, fz_rect doc_rect) {
	fz_rect res;
	document_to_window_pos(page, doc_rect.x0, doc_rect.y0, &res.x0, &res.y0);
	document_to_window_pos(page, doc_rect.x1, doc_rect.y1, &res.x1, &res.y1);

	return res;
}
void DocumentView::window_to_document_pos(float window_x, float window_y, float* doc_x, float* doc_y, int* doc_page) {
	if (current_document) {
		current_document->absolute_to_page_pos(
			(window_x - view_width / 2) / zoom_level - offset_x,
			(window_y - view_height / 2) / zoom_level + offset_y, doc_x, doc_y, doc_page);
	}
}
void DocumentView::window_to_absolute_document_pos(float window_x, float window_y, float* doc_x, float* doc_y) {
	*doc_x = (window_x - view_width / 2) / zoom_level - offset_x;
	*doc_y = (window_y - view_height / 2) / zoom_level + offset_y;
}


void DocumentView::goto_mark(char symbol) {
	if (current_document) {
		float new_y_offset = 0.0f;
		if (current_document->get_mark_location_if_exists(symbol, &new_y_offset)) {
			set_offset_y(new_y_offset);
		}
	}
}
void DocumentView::goto_end() {
	if (current_document) {
		int last_page_index = current_document->num_pages() - 1;
		set_offset_y(current_document->get_accum_page_height(last_page_index));
	}
}
float DocumentView::set_zoom_level(float zl) {
	zoom_level = zl;
	return zoom_level;
}
float DocumentView::zoom_in() {
	const float max_zoom_level = 10.0f;
	float new_zoom_level = zoom_level * ZOOM_INC_FACTOR;

	if (new_zoom_level > max_zoom_level) {
		new_zoom_level = max_zoom_level;
	}
	
	return set_zoom_level(new_zoom_level);
}
float DocumentView::zoom_out() {
	return set_zoom_level(zoom_level / ZOOM_INC_FACTOR);
}
void DocumentView::move_absolute(float dx, float dy) {
	set_offsets(offset_x + dx, offset_y + dy);
}
void DocumentView::move(float dx, float dy) {
	int abs_dx = (dx / zoom_level);
	int abs_dy = (dy / zoom_level);
	move_absolute(abs_dx, abs_dy);
}
void DocumentView::get_absolute_delta_from_doc_delta(float dx, float dy, float* abs_dx, float* abs_dy)
{
	*abs_dx = (dx / zoom_level);
	*abs_dy = (dy / zoom_level);
}
int DocumentView::get_current_page_number() {
	return current_document->get_offset_page_number(get_offset_y());
}

//void DocumentView::search_text(const wchar_t* text) {
//	if (!current_document) return;
//
//	search_results_mutex.lock();
//	search_results.clear();
//	current_search_result_index = 0;
//	search_results_mutex.unlock();
//
//	is_searching = true;
//	pdf_renderer->add_request(current_document->get_path(), 
//		get_current_page_number(), text, &search_results, &percent_done, &is_searching, &search_results_mutex);
//}

//void DocumentView::goto_search_result(int offset) {
//	if (!current_document) return;
//
//	search_results_mutex.lock();
//	if (search_results.size() == 0) {
//		search_results_mutex.unlock();
//		return;
//	}
//
//	int target_index = mod(current_search_result_index + offset, search_results.size());
//	current_search_result_index = target_index;
//
//	int target_page = search_results[target_index].page;
//
//	fz_rect rect = search_results[target_index].rect;
//
//	float new_offset_y = rect.y0 + current_document->get_accum_page_height(target_page);
//
//	set_offset_y(new_offset_y);
//	search_results_mutex.unlock();
//}
void DocumentView::get_visible_pages(int window_height, std::vector<int>& visible_pages) {
	if (!current_document) return;

	float window_y_range_begin = offset_y - window_height / (1.5 * zoom_level);
	float window_y_range_end = offset_y + window_height / (1.5 * zoom_level);
	window_y_range_begin -= 1;
	window_y_range_end += 1;

	current_document->get_visible_pages(window_y_range_begin, window_y_range_end, visible_pages);

	//float page_begin = 0.0f;

	//const vector<float>& page_heights = current_document->get_page_heights();
	//for (int i = 0; i < page_heights.size(); i++) {
	//	float page_end = page_begin + page_heights[i];

	//	if (intersects(window_y_range_begin, window_y_range_end, page_begin, page_end)) {
	//		visible_pages.push_back(i);
	//	}
	//	page_begin = page_end;
	//}
	//cout << "num visible pages:" << visible_pages.size() << endl;
}
void DocumentView::move_pages(int num_pages) {
	if (!current_document) return;
	int current_page = get_current_page_number();
	if (current_page == -1) {
		current_page = 0;
	}
	move_absolute(0, num_pages * (current_document->get_page_height(current_page) + PAGE_PADDINGS));
}

void DocumentView::move_screens(int num_screens)
{
	float screen_height_in_doc_space = view_height / zoom_level;
	set_offset_y(get_offset_y() + num_screens * screen_height_in_doc_space * MOVE_SCREEN_PERCENTAGE);
	//return move_amount;
}

void DocumentView::reset_doc_state() {
	zoom_level = 1.0f;
	set_offsets(0.0f, 0.0f);
}
void DocumentView::open_document(const std::wstring& doc_path,
	bool* invalid_flag,
	bool load_prev_state,
	std::optional<OpenedBookState> prev_state,
	bool force_load_dimensions) {

	std::wstring canonical_path = get_canonical_path(doc_path);

	if (canonical_path == L"") {
		current_document = nullptr;
		return;
	}

	//if (error_code) {
	//	current_document = nullptr;
	//	return;
	//}

	//document_path = cannonical_path;


	//current_document = new Document(mupdf_context, doc_path, database);
	//current_document = document_manager->get_document(doc_path);
	current_document = document_manager->get_document(canonical_path);
	//current_document->open();
	if (!current_document->open(invalid_flag, force_load_dimensions)) {
		current_document = nullptr;
	}

	reset_doc_state();

	if (prev_state) {
		zoom_level = prev_state.value().zoom_level;
		offset_x = prev_state.value().offset_x;
		offset_y = prev_state.value().offset_y;
		set_offsets(offset_x, offset_y);
	}
	else if (load_prev_state) {

		std::vector<OpenedBookState> prev_state;
		if (select_opened_book(database, canonical_path, prev_state)) {
			if (prev_state.size() > 1) {
				std::cerr << "more than one file with one path, this should not happen!" << std::endl;
			}
		}
		if (prev_state.size() > 0) {
			OpenedBookState previous_state = prev_state[0];
			zoom_level = previous_state.zoom_level;
			offset_x = previous_state.offset_x;
			offset_y = previous_state.offset_y;
			set_offsets(previous_state.offset_x, previous_state.offset_y);
		}
		else {
			if (current_document) {
				// automatically adjust width
				fit_to_page_width();
			}
		}
	}
}
float DocumentView::get_page_offset(int page) {

	if (!current_document) return 0.0f;

	int max_page = current_document->num_pages()-1;
	if (page > max_page) {
		page = max_page;
	}
	return current_document->get_accum_page_height(page);
}

void DocumentView::goto_offset_within_page(int page, float offset_x, float offset_y) {
	set_offsets(offset_x, get_page_offset(page) + offset_y);
}

void DocumentView::goto_page(int page) {
	set_offset_y(get_page_offset(page) + view_height_in_document_space()/2);
}

//void DocumentView::goto_toc_link(std::variant<PageTocLink, ChapterTocLink> toc_link) {
//	int page = -1;
//
//	if (std::holds_alternative<PageTocLink>(toc_link)) {
//		PageTocLink l = std::get<PageTocLink>(toc_link);
//		page = l.page;
//	}
//	else{
//		ChapterTocLink l = std::get<ChapterTocLink>(toc_link);
//		std::vector<int> accum_chapter_page_counts;
//		current_document->count_chapter_pages_accum(accum_chapter_page_counts);
//		page = accum_chapter_page_counts[l.chapter] + l.page;
//	}
//	set_offset_y(get_page_offset(page) + view_height_in_document_space()/2);
//}

void DocumentView::fit_to_page_width(bool smart)
{
	int cp = get_current_page_number();
	//int page_width = current_document->get_page_width(cp);
	if (smart) {

		float left_ratio, right_ratio;
		int normal_page_width;
		int page_width = current_document->get_page_width_smart(cp, &left_ratio, &right_ratio, &normal_page_width);
		float right_leftover = 1.0f - right_ratio;
		float imbalance = left_ratio - right_leftover;

		set_offset_x(-imbalance * normal_page_width / 2.0f);
		set_zoom_level(static_cast<float>(view_width) / page_width);
	}
	else {
		int page_width = current_document->get_page_width(cp);
		set_offset_x(0);
		set_zoom_level(static_cast<float>(view_width) / page_width);
	}

}

void DocumentView::persist() {
	if (!current_document) return;
	update_book(database, current_document->get_path(), zoom_level, offset_x, offset_y);
}

int DocumentView::get_current_chapter_index()
{
	const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();

	if (chapter_pages.size() == 0) {
		return -1;
	}

	int cp = get_current_page_number();

	int current_chapter_index = 0;

	int index = 0;
	for (int p : chapter_pages) {
		if (p <= cp) {
			current_chapter_index = index;
		}
		index++;
	}

	return current_chapter_index;
}

std::wstring DocumentView::get_current_chapter_name()
{
	const std::vector<std::wstring>& chapter_names = current_document->get_flat_toc_names();
	int current_chapter_index = get_current_chapter_index();
	if (current_chapter_index > 0) {
		return chapter_names[current_chapter_index];
	}
	return L"";
}

std::optional<std::pair<int, int>> DocumentView::get_current_page_range()
{
	int ci = get_current_chapter_index();
	if (ci < 0) {
		return {};
	}
	const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();
	int range_begin = chapter_pages[ci];
	int range_end = current_document->num_pages()-1;

	if (ci < chapter_pages.size() - 1) {
		range_end = chapter_pages[ci + 1];
	}

	return std::make_pair(range_begin, range_end);
}

void DocumentView::goto_chapter(int diff)
{
	const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();
	int curr_page = get_current_page_number();

	int index = 0;

	while (index < chapter_pages.size() && chapter_pages[index] < curr_page) {
		index++;
	}

	int new_index = index + diff;
	if (new_index < 0) {
		goto_page(0);
	}
	else if (new_index >= chapter_pages.size()) {
		goto_end();
	}
	else {
		goto_page(chapter_pages[new_index]);
	}
}

float DocumentView::view_height_in_document_space()
{
	return static_cast<float>(view_height) / zoom_level;
}

void DocumentView::set_vertical_line_pos(float pos)
{
	vertical_line_pos = pos;
}

float DocumentView::get_vertical_line_pos()
{
	return vertical_line_pos;
}

float DocumentView::get_vertical_line_window_y()
{
	float absol_y = get_vertical_line_pos();
	float window_x, window_y;
	absolute_to_window_pos(0.0, absol_y, &window_x, &window_y);
	return window_y;
}

void DocumentView::goto_vertical_line_pos()
{
	if (current_document) {
		float new_y_offset = vertical_line_pos;
		set_offset_y(new_y_offset);
	}
}

void DocumentView::get_text_selection(fz_point selection_begin,
	fz_point selection_end,
	bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
	std::vector<fz_rect>& selected_characters,
	std::wstring& selected_text) {

	// selected_characters are in absolute document space
	if (!current_document) return;
	int page_begin, page_end;
	fz_rect page_rect;

	selected_characters.clear();
	selected_text.clear();

	fz_point page_point1;
	fz_point page_point2;

	current_document->absolute_to_page_pos(selection_begin.x, selection_begin.y, &page_point1.x, &page_point1.y, &page_begin);
	current_document->absolute_to_page_pos(selection_end.x, selection_end.y, &page_point2.x, &page_point2.y, &page_end);

	if (page_end < page_begin) {
		std::swap(page_begin, page_end);
		std::swap(page_point1, page_point2);
	}

	selected_text.clear();

	bool word_selecting = false;
	bool selecting = false;
	if (is_word_selection) {
		selecting = true;
	}
	
	for (int i = page_begin; i <= page_end; i++) {

		// for now, let's assume there is only one page
		fz_stext_page* stext_page = current_document->get_stext_with_page_number(i);
		std::vector<fz_stext_char*> flat_chars;


		get_flat_chars_from_stext_page(stext_page, flat_chars);

		if (!stext_page) continue;

		int location_index1, location_index2;
		fz_stext_char* char_begin = nullptr;
		fz_stext_char* char_end = nullptr;
		if (i == page_begin) {
			char_begin = find_closest_char_to_document_point(flat_chars, page_point1, &location_index1);
		}
		if (i == page_end) {
			char_end = find_closest_char_to_document_point(flat_chars, page_point2, &location_index2);
		}
		if (char_begin && char_end) {
			// swap the locations if end happends before begin
			if (page_begin == page_end && location_index1 > location_index2) {
				std::swap(char_begin, char_end);
			}
		}


		for (auto current_char : flat_chars) {
			if (!is_word_selection) {
				if (current_char == char_begin) {
					selecting = true;
				}
				//if (current_char == char_end) {
				//	selecting = false;
				//}
			}
			else {
				if (word_selecting == false && is_separator(char_begin, current_char)) {
					selected_text.clear();
					selected_characters.clear();
					continue;
				}
				if (current_char == char_begin) {
					word_selecting = true;
				}
				if (current_char == char_end) {
					selecting = false;
				}
				if (word_selecting == true && is_separator(char_end, current_char) && selecting == false) {
					word_selecting = false;
					return;
				}
			}

			if (selecting || word_selecting) {
				if (!(current_char->c == ' ' && selected_text.size() == 0)) {
					selected_text.push_back(current_char->c);
					fz_rect charrect = current_document->page_rect_to_absolute_rect(i, fz_rect_from_quad(current_char->quad));
					selected_characters.push_back(charrect);
				}
				if ((current_char->next == nullptr)) {
					if (current_char->c != '-')
					{
						selected_text.push_back(' ');
					}
					else {
						selected_text.pop_back();
					}
				}

			}
			if (!is_word_selection) {
				if (current_char == char_end) {
					selecting = false;
				}
			}
		}
	}

}
