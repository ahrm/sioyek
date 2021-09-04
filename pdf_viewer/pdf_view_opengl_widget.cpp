#include "pdf_view_opengl_widget.h"
#include "path.h"
#include <qcolor.h>

extern Path shader_path;
extern float BACKGROUND_COLOR[3];
extern float DARK_MODE_BACKGROUND_COLOR[3];
extern float DARK_MODE_CONTRAST;
extern float VERTICAL_LINE_WIDTH;
extern float VERTICAL_LINE_FREQ;
extern float ZOOM_INC_FACTOR;
extern float VERTICAL_MOVE_AMOUNT;
extern float HIGHLIGHT_COLORS[26 * 3];
extern bool SHOULD_DRAW_UNRENDERED_PAGES;

GLfloat g_quad_vertex[] = {
	-1.0f, -1.0f,
	1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, 1.0f
};

GLfloat g_quad_uvs[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
};

OpenGLSharedResources PdfViewOpenGLWidget::shared_gl_objects;

GLuint PdfViewOpenGLWidget::LoadShaders(Path vertex_file_path, Path fragment_file_path) {

	//const wchar_t* vertex_file_path = vertex_file_path_.c_str();
	//const wchar_t* fragment_file_path = fragment_file_path_.c_str();
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path.get_path_utf8(), std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path.get_path_utf8().c_str());
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path.get_path_utf8(), std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path.get_path_utf8().c_str());
		return 0;
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path.get_path_utf8().c_str());
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path.get_path_utf8().c_str());
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

void PdfViewOpenGLWidget::initializeGL() {
	is_opengl_initialized = true;

	initializeOpenGLFunctions();

	if (!shared_gl_objects.is_initialized) {
		// we initialize the shared opengl objects here. Ideally we should have initialized them before any object
		// of this type was created but we could not use any OpenGL function before initalizeGL is called for the
		// first time.

		shared_gl_objects.is_initialized = true;

		//shared_gl_objects.rendered_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path, L"simple.fragment"));
		//shared_gl_objects.rendered_dark_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path, L"dark_mode.fragment"));
		//shared_gl_objects.unrendered_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path, L"unrendered_page.fragment"));
		//shared_gl_objects.highlight_program = LoadShaders( concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path , L"highlight.fragment"));
		//shared_gl_objects.vertical_line_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path , L"vertical_bar.fragment"));
		//shared_gl_objects.vertical_line_dark_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path , L"vertical_bar_dark.fragment"));

		shared_gl_objects.rendered_program = LoadShaders(shader_path.slash(L"simple.vertex"),  shader_path.slash(L"simple.fragment"));
		shared_gl_objects.rendered_dark_program = LoadShaders(shader_path.slash(L"simple.vertex"),  shader_path.slash(L"dark_mode.fragment"));
		shared_gl_objects.unrendered_program = LoadShaders(shader_path.slash(L"simple.vertex"),  shader_path.slash(L"unrendered_page.fragment"));
		shared_gl_objects.highlight_program = LoadShaders( shader_path.slash(L"simple.vertex"),  shader_path .slash(L"highlight.fragment"));
		shared_gl_objects.vertical_line_program = LoadShaders(shader_path.slash(L"simple.vertex"),  shader_path .slash(L"vertical_bar.fragment"));
		shared_gl_objects.vertical_line_dark_program = LoadShaders(shader_path.slash(L"simple.vertex"),  shader_path .slash(L"vertical_bar_dark.fragment"));

		shared_gl_objects.dark_mode_contrast_uniform_location = glGetUniformLocation(shared_gl_objects.rendered_dark_program, "contrast");

		shared_gl_objects.highlight_color_uniform_location = glGetUniformLocation(shared_gl_objects.highlight_program, "highlight_color");

		shared_gl_objects.line_color_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "line_color");
		shared_gl_objects.line_time_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "time");
		shared_gl_objects.line_freq_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "freq");

		glGenBuffers(1, &shared_gl_objects.vertex_buffer_object);
		glGenBuffers(1, &shared_gl_objects.uv_buffer_object);

		glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex), g_quad_vertex, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), g_quad_uvs, GL_DYNAMIC_DRAW);

	}

	//vertex array objects can not be shared for some reason!
	glGenVertexArrays(1, &vertex_array_object);
	glBindVertexArray(vertex_array_object);

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void PdfViewOpenGLWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);

	if (document_view) {
		document_view->on_view_size_change(w, h);
	}
}

void PdfViewOpenGLWidget::render_line_window(GLuint program, float gl_vertical_pos) {


	float bar_height = VERTICAL_LINE_WIDTH;

	float bar_data[] = {
		-1, gl_vertical_pos,
		1, gl_vertical_pos,
		-1, gl_vertical_pos - bar_height,
		1, gl_vertical_pos - bar_height
	};

	glDisable(GL_CULL_FACE);
	glUseProgram(program);

	const float* vertical_line_color = config_manager->get_config<float>(L"vertical_line_color");
	if (vertical_line_color != nullptr) {
		glUniform4fv(shared_gl_objects.line_color_uniform_location,
			1,
			vertical_line_color);
	}
	float time = -QDateTime::currentDateTime().msecsTo(creation_time);
	glUniform1f(shared_gl_objects.line_time_uniform_location, time);
	glUniform1f(shared_gl_objects.line_freq_uniform_location, VERTICAL_LINE_FREQ);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(line_data), line_data, GL_DYNAMIC_DRAW);
	//glDrawArrays(GL_LINES, 0, 2);

	glBufferData(GL_ARRAY_BUFFER, sizeof(bar_data), bar_data, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}
void PdfViewOpenGLWidget::render_highlight_window(GLuint program, fz_rect window_rect, bool draw_border) {

	float quad_vertex_data[] = {
		window_rect.x0, window_rect.y1,
		window_rect.x1, window_rect.y1,
		window_rect.x0, window_rect.y0,
		window_rect.x1, window_rect.y0
	};
	float line_data[] = {
		window_rect.x0, window_rect.y0,
		window_rect.x1, window_rect.y0,
		window_rect.x1, window_rect.y1,
		window_rect.x0, window_rect.y1
	};


	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);

	glUseProgram(program);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (draw_border) {
		glDisable(GL_BLEND);
		glBufferData(GL_ARRAY_BUFFER, sizeof(line_data), line_data, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINE_LOOP, 0, 4);
	}


}

void PdfViewOpenGLWidget::render_highlight_absolute(GLuint program, fz_rect absolute_document_rect, bool draw_border) {
	fz_rect window_rect = document_view->absolute_to_window_rect(absolute_document_rect);
	render_highlight_window(program, window_rect, draw_border);
}

void PdfViewOpenGLWidget::render_highlight_document(GLuint program, int page, fz_rect doc_rect) {
	fz_rect window_rect = document_view->document_to_window_rect(page, doc_rect);
	render_highlight_window(program, window_rect);
}

void PdfViewOpenGLWidget::paintGL() {

	QPainter painter(this);
	QTextOption option;

	QColor red_color = QColor::fromRgb(255, 0, 0);
	painter.setPen(red_color);

	painter.beginNativePainting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glBindVertexArray(vertex_array_object);
	render(&painter);
	painter.endNativePainting();
	painter.drawText(-100, -100, "1234567890");
}

PdfViewOpenGLWidget::PdfViewOpenGLWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, ConfigManager* config_manager, bool is_helper, QWidget* parent) :
	QOpenGLWidget(parent),
	document_view(document_view),
	config_manager(config_manager),
	pdf_renderer(pdf_renderer),
	is_helper(is_helper)
{
	creation_time = QDateTime::currentDateTime();

	QSurfaceFormat format;
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	this->setFormat(format);
}

void PdfViewOpenGLWidget::handle_escape() {
	search_results.clear();
	synctex_highlights.clear();
	current_search_result_index =-1;
	is_searching = false;
	is_search_cancelled = true;
	should_highlight_links = false;
}

void PdfViewOpenGLWidget::toggle_highlight_links() {
	this->should_highlight_links = !this->should_highlight_links;
}

void PdfViewOpenGLWidget::set_highlight_links(bool should_highlight) {
	this->should_highlight_links = should_highlight;
}

int PdfViewOpenGLWidget::get_num_search_results() {
	search_results_mutex.lock();
	int num = search_results.size();
	search_results_mutex.unlock();
	return num;
}

int PdfViewOpenGLWidget::get_current_search_result_index() {
	return current_search_result_index;
}

bool PdfViewOpenGLWidget::valid_document() {
	if (document_view) {
		if (document_view->get_document()) {
			return true;
		}
	}
	return false;
}

void PdfViewOpenGLWidget::goto_search_result(int offset) {
	if (!valid_document()) return;

	search_results_mutex.lock();
	if (search_results.size() == 0) {
		search_results_mutex.unlock();
		return;
	}

	int target_index = mod(current_search_result_index + offset, search_results.size());
	current_search_result_index = target_index;

	auto [rect, target_page] = search_results[target_index];
	search_results_mutex.unlock();

	float new_offset_y = rect.y0 + document_view->get_document()->get_accum_page_height(target_page);

	document_view->set_offset_y(new_offset_y);
}

void PdfViewOpenGLWidget::render_page(int page_number) {

	if (!valid_document()) return;

	GLuint texture = pdf_renderer->find_rendered_page(document_view->get_document()->get_path(),
		page_number,
		document_view->get_zoom_level(),
		nullptr,
		nullptr);

	float page_vertices[4 * 2];
	fz_rect page_rect = { 0,
		0,
		document_view->get_document()->get_page_width(page_number),
		document_view->get_document()->get_page_height(page_number) };

	fz_rect window_rect = document_view->document_to_window_rect(page_number, page_rect);
	rect_to_quad(window_rect, page_vertices);

	if (texture != 0) {

		if (is_dark_mode) {
			glUseProgram(shared_gl_objects.rendered_dark_program);
			glUniform1f(shared_gl_objects.dark_mode_contrast_uniform_location, DARK_MODE_CONTRAST);
		}
		else {
			glUseProgram(shared_gl_objects.rendered_program);
		}

		glBindTexture(GL_TEXTURE_2D, texture);
	}
	else {
		if (!SHOULD_DRAW_UNRENDERED_PAGES) {
			return;
		}
		glUseProgram(shared_gl_objects.unrendered_program);
	}
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void PdfViewOpenGLWidget::render(QPainter* painter) {

	if (!valid_document()) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	std::vector<int> visible_pages;
	document_view->get_visible_pages(document_view->get_view_height(), visible_pages);

	if (is_dark_mode) {
		glClearColor(DARK_MODE_BACKGROUND_COLOR[0], DARK_MODE_BACKGROUND_COLOR[1], DARK_MODE_BACKGROUND_COLOR[2], 1.0f);
	}
	else {
		glClearColor(BACKGROUND_COLOR[0], BACKGROUND_COLOR[1], BACKGROUND_COLOR[2], 1.0f);
	}
	glClear(GL_COLOR_BUFFER_BIT);


	std::vector<std::pair<int, fz_link*>> all_visible_links;

	for (int page : visible_pages) {
		render_page(page);

		if (should_highlight_links) {
			glUseProgram(shared_gl_objects.highlight_program);
			glUniform3fv(shared_gl_objects.highlight_color_uniform_location,
				1,
				config_manager->get_config<float>(L"link_highlight_color"));
			fz_link* links = document_view->get_document()->get_page_links(page);
			while (links != nullptr) {
				render_highlight_document(shared_gl_objects.highlight_program, page, links->rect);
				all_visible_links.push_back(std::make_pair(page, links));
				links = links->next;
			}
		}
	}

#ifndef NDEBUG
	if (last_selected_block) {
		glUseProgram(shared_gl_objects.highlight_program);
		glUniform3fv(shared_gl_objects.highlight_color_uniform_location,
			1,
			config_manager->get_config<float>(L"link_highlight_color"));

		int page = last_selected_block_page.value();
		fz_rect rect = last_selected_block.value();
		render_highlight_document(shared_gl_objects.highlight_program, page, rect);
	}
#endif


	search_results_mutex.lock();
	if (search_results.size() > 0) {
		int index = current_search_result_index;
		if (index == -1) index = 0;

		SearchResult current_search_result = search_results[index];
		glUseProgram(shared_gl_objects.highlight_program);
		glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"search_highlight_color"));
		//glUniform3fv(g_shared_resources.highlight_color_uniform_location, 1, highlight_color_temp);
		render_highlight_document(shared_gl_objects.highlight_program, current_search_result.page, current_search_result.rect);
	}
	search_results_mutex.unlock();

	glUseProgram(shared_gl_objects.highlight_program);
	glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"text_highlight_color"));
	std::vector<fz_rect> bounding_rects;
	merge_selected_character_rects(selected_character_rects, bounding_rects);
	//for (auto rect : selected_character_rects) {
	//	render_highlight_absolute(shared_gl_objects.highlight_program, rect);
	//}
	for (auto rect : bounding_rects) {
		render_highlight_absolute(shared_gl_objects.highlight_program, rect);
	}

	glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"synctex_highlight_color"));
	for (auto [page, rect] : synctex_highlights) {
		render_highlight_document(shared_gl_objects.highlight_program, page, rect);
	}

	if (document_view->get_document()->can_use_highlights()) {
		const std::vector<Highlight>& highlights = document_view->get_document()->get_highlights();
		for (int i = 0; i < highlights.size(); i++) {
			float selection_begin_window_x, selection_begin_window_y;
			float selection_end_window_x, selection_end_window_y;

			document_view->absolute_to_window_pos(
				highlights[i].selection_begin.x,
				highlights[i].selection_begin.y,
				&selection_begin_window_x,
				&selection_begin_window_y);

			document_view->absolute_to_window_pos(
				highlights[i].selection_end.x,
				highlights[i].selection_end.y,
				&selection_end_window_x,
				&selection_end_window_y);

			bool is_selection_in_window = range_intersects(selection_begin_window_y, selection_end_window_y, -1.0f, 1.0f);

			if (is_selection_in_window) {
				for (int j = 0; j < highlights[i].highlight_rects.size(); j++) {
					glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &HIGHLIGHT_COLORS[(highlights[i].type - 'a') * 3]);
					render_highlight_absolute(shared_gl_objects.highlight_program, highlights[i].highlight_rects[j], false);
				}
			}
		}
	}

	if (should_draw_vertical_line) {
		//render_line_window(shared_gl_objects.vertical_line_program ,vertical_line_location);

		if (is_dark_mode) {
			render_line_window(shared_gl_objects.vertical_line_dark_program , document_view->get_vertical_line_window_y());
		}
		else {
			render_line_window(shared_gl_objects.vertical_line_program , document_view->get_vertical_line_window_y());
		}
	}

	if (should_highlight_links) {
		painter->endNativePainting();
		for (int i = 0; i < all_visible_links.size(); i++) {
			std::stringstream ss;
			ss << i;
			std::string index_string = ss.str();

			auto [page, link] = all_visible_links[i];

			//document_view->document_to_window_pos_in_pixels(page, link->rect.x0, link->rect.x1, &window_x, &window_y);
			fz_rect window_rect = document_view->document_to_window_rect(page, link->rect);

			int view_width = static_cast<float>(document_view->get_view_width());
			int view_height = static_cast<float>(document_view->get_view_height());
			
			int window_x = static_cast<int>(window_rect.x0 * view_width / 2 + view_width / 2);
			int window_y = static_cast<int>(-window_rect.y0 * view_height / 2 + view_height / 2);

			painter->drawText(window_x, window_y, index_string.c_str());
		}
	}
}

bool PdfViewOpenGLWidget::get_is_searching(float* prog)
{
	if (is_search_cancelled) {
		return false;
	}

	search_results_mutex.lock();
	bool res = is_searching;
	if (is_searching) {
		*prog = percent_done;
	}
	search_results_mutex.unlock();
	return true;
}

void PdfViewOpenGLWidget::search_text(const std::wstring& text, std::optional<std::pair<int,int>> range) {

	if (!document_view) return;

	search_results_mutex.lock();
	search_results.clear();
	current_search_result_index = -1;
	search_results_mutex.unlock();

	is_searching = true;
	is_search_cancelled = false;

	int current_page = document_view->get_current_page_number();
	if (current_page >= 0) {
		pdf_renderer->add_request(
			document_view->get_document()->get_path(),
			current_page,
			text,
			&search_results,
			&percent_done,
			&is_searching,
			&search_results_mutex,
			range);

	}
}

void PdfViewOpenGLWidget::set_dark_mode(bool mode)
{
	this->is_dark_mode = mode;
}

void PdfViewOpenGLWidget::set_synctex_highlights(std::vector<std::pair<int, fz_rect>> highlights)
{
	synctex_highlights = std::move(highlights);
}

void PdfViewOpenGLWidget::on_document_view_reset()
{
	this->synctex_highlights.clear();
}

PdfViewOpenGLWidget::~PdfViewOpenGLWidget() {
	if (is_opengl_initialized) {
		glDeleteVertexArrays(1, &vertex_array_object);
	}
}

//void PdfViewOpenGLWidget::set_vertical_line_pos(float pos)
//{
//	vertical_line_location = pos;
//}
//
//float PdfViewOpenGLWidget::get_vertical_line_pos()
//{
//	return vertical_line_location;
//}

void PdfViewOpenGLWidget::set_should_draw_vertical_line(bool val)
{
	should_draw_vertical_line = val;
}

bool PdfViewOpenGLWidget::get_should_draw_vertical_line()
{
	return should_draw_vertical_line;
}

void PdfViewOpenGLWidget::mouseMoveEvent(QMouseEvent* mouse_event) {

	if (is_helper && (document_view != nullptr)) {

		int x = mouse_event->pos().x();
		int y = mouse_event->pos().y();

		int x_diff = x - last_mouse_down_window_x;
		int y_diff = y - last_mouse_down_window_y;

		float x_diff_doc = x_diff / document_view->get_zoom_level();
		float y_diff_doc = y_diff / document_view->get_zoom_level();

		document_view->set_offsets(last_mouse_down_document_offset_x + x_diff_doc, last_mouse_down_document_offset_y - y_diff_doc);
		update();
	}
}

void PdfViewOpenGLWidget::mousePressEvent(QMouseEvent* mevent) {
	if (is_helper && (document_view != nullptr)) {
		int window_x = mevent->pos().x();
		int window_y = mevent->pos().y();

		if (mevent->button() == Qt::MouseButton::LeftButton) {
			last_mouse_down_window_x = window_x;
			last_mouse_down_window_y = window_y;

			last_mouse_down_document_offset_x = document_view->get_offset_x();
			last_mouse_down_document_offset_y = document_view->get_offset_y();
		}
	}
}

void PdfViewOpenGLWidget::mouseReleaseEvent(QMouseEvent* mouse_event) {

	if (is_helper && (document_view != nullptr)) {

		int x = mouse_event->pos().x();
		int y = mouse_event->pos().y();

		int x_diff = x - last_mouse_down_window_x;
		int y_diff = y - last_mouse_down_window_y;

		float x_diff_doc = x_diff / document_view->get_zoom_level();
		float y_diff_doc = y_diff / document_view->get_zoom_level();

		document_view->set_offsets(last_mouse_down_document_offset_x + x_diff_doc, last_mouse_down_document_offset_y - y_diff_doc);

		OpenedBookState new_book_state = document_view->get_state().book_state;
		if (this->on_link_edit) {
			(this->on_link_edit.value())(new_book_state);
		}

		update();
	}
}

void PdfViewOpenGLWidget::wheelEvent(QWheelEvent* wevent) {
	if (is_helper && (document_view != nullptr)) {

		bool is_control_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier);

		if (is_control_pressed) {
			if (wevent->angleDelta().y() > 0) {
				float pev_zoom_level = document_view->get_zoom_level();
				float new_zoom_level = pev_zoom_level * ZOOM_INC_FACTOR;
				document_view->set_zoom_level(new_zoom_level);
			}

			if (wevent->angleDelta().y() < 0) {
				float pev_zoom_level = document_view->get_zoom_level();
				float new_zoom_level = pev_zoom_level / ZOOM_INC_FACTOR;
				document_view->set_zoom_level(new_zoom_level);
			}
		}
		else {
			float prev_doc_x = document_view->get_offset_x();
			float prev_doc_y = document_view->get_offset_y();
			float prev_zoom_level = document_view->get_zoom_level();

			float delta_y = wevent->angleDelta().y() * VERTICAL_MOVE_AMOUNT / prev_zoom_level;
			float delta_x = wevent->angleDelta().x() * VERTICAL_MOVE_AMOUNT / prev_zoom_level;

			document_view->set_offsets(prev_doc_x + delta_x, prev_doc_y - delta_y);
		}

		OpenedBookState new_book_state = document_view->get_state().book_state;
		if (this->on_link_edit) {
			(this->on_link_edit.value())(new_book_state);
		}
		update();
	}

}

void PdfViewOpenGLWidget::register_on_link_edit_listener(std::function<void(const OpenedBookState&)> listener) {
	this->on_link_edit = listener;
}
