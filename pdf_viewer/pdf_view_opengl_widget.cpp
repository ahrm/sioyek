#include "pdf_view_opengl_widget.h"
#include "path.h"
#include <qcolor.h>
#include <cmath>

extern Path shader_path;
extern float GAMMA;
extern float BACKGROUND_COLOR[3];
extern float DARK_MODE_BACKGROUND_COLOR[3];
extern float CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[3];
extern float DARK_MODE_CONTRAST;
extern float ZOOM_INC_FACTOR;
extern float VERTICAL_MOVE_AMOUNT;
extern float HIGHLIGHT_COLORS[26 * 3];
extern bool SHOULD_DRAW_UNRENDERED_PAGES;
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
extern bool RERENDER_OVERVIEW;
extern bool RULER_MODE;
extern float PAGE_SEPARATOR_WIDTH;
extern float PAGE_SEPARATOR_COLOR[3];
extern float RULER_PADDING;
extern float OVERVIEW_SIZE[2];
extern float OVERVIEW_OFFSET[2];
extern float FASTREAD_OPACITY;
extern bool PRERENDER_NEXT_PAGE;
extern int PRERENDERED_PAGE_COUNT;
extern bool SHOULD_HIGHLIGHT_LINKS;
extern bool SHOULD_HIGHLIGHT_UNSELECTED_SEARCH;
extern float UNSELECTED_SEARCH_HIGHLIGHT_COLOR[3];
extern int KEYBOARD_SELECT_FONT_SIZE;
extern float CUSTOM_COLOR_CONTRAST;
extern float DISPLAY_RESOLUTION_SCALE;
extern float KEYBOARD_SELECT_BACKGROUND_COLOR[4];
extern float KEYBOARD_SELECT_TEXT_COLOR[4];
extern bool ALPHABETIC_LINK_TAGS;

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

GLfloat g_quad_uvs_rotated[] = {
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f
};

GLfloat rotation_uvs[4][8] = {
	{
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
	},
	{
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f
	},
	{
	1.0f, 1.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f
	},
	{
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	0.0f, 1.0f
	},
};

OpenGLSharedResources PdfViewOpenGLWidget::shared_gl_objects;

GLuint PdfViewOpenGLWidget::LoadShaders(Path vertex_file_path, Path fragment_file_path) {

	//const wchar_t* vertex_file_path = vertex_file_path_.c_str();
	//const wchar_t* fragment_file_path = fragment_file_path_.c_str();
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::wstring VertexShaderCode;
	std::string vertex_shader_code_utf8;

	std::wifstream VertexShaderStream = open_wifstream(vertex_file_path.get_path());
	if (VertexShaderStream.is_open()) {
		std::wstringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::wstring FragmentShaderCode;
	std::string fragment_shader_code_utf8;

	std::wifstream FragmentShaderStream = open_wifstream(fragment_file_path.get_path());
	if (FragmentShaderStream.is_open()) {
		std::wstringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}
	else {
		return 0;
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	vertex_shader_code_utf8 = utf8_encode(VertexShaderCode);
	char const* VertexSourcePointer = vertex_shader_code_utf8.c_str();
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
	fragment_shader_code_utf8 = utf8_encode(FragmentShaderCode);
	char const* FragmentSourcePointer = fragment_shader_code_utf8.c_str();
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
		shared_gl_objects.custom_color_program = LoadShaders(shader_path.slash(L"simple.vertex"),  shader_path.slash(L"custom_colors.fragment"));
		shared_gl_objects.separator_program = LoadShaders(shader_path.slash(L"simple.vertex"),  shader_path.slash(L"separator.fragment"));
		shared_gl_objects.stencil_program = LoadShaders(shader_path.slash(L"stencil.vertex"),  shader_path.slash(L"stencil.fragment"));

		shared_gl_objects.dark_mode_contrast_uniform_location = glGetUniformLocation(shared_gl_objects.rendered_dark_program, "contrast");
		shared_gl_objects.gamma_uniform_location = glGetUniformLocation(shared_gl_objects.rendered_program, "gamma");

		shared_gl_objects.highlight_color_uniform_location = glGetUniformLocation(shared_gl_objects.highlight_program, "highlight_color");

		shared_gl_objects.line_color_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "line_color");
		shared_gl_objects.line_time_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "time");

		shared_gl_objects.custom_color_transform_uniform_location = glGetUniformLocation(shared_gl_objects.custom_color_program, "transform_matrix");

		shared_gl_objects.separator_background_color_uniform_location = glGetUniformLocation(shared_gl_objects.separator_program, "background_color");

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

void PdfViewOpenGLWidget::render_line_window(GLuint program, float gl_vertical_pos, std::optional<fz_rect> ruler_rect) {


	float bar_height = 4.0f;

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

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(line_data), line_data, GL_DYNAMIC_DRAW);
	//glDrawArrays(GL_LINES, 0, 2);

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bar_data), bar_data, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (RULER_MODE && ruler_rect.has_value()) {
		float gl_vertical_begin_pos = ruler_rect.value().y0;
		float ruler_left_pos = ruler_rect.value().x0;
		float ruler_right_pos = ruler_rect.value().x1;
		float top_bar_data[] = {
			-1, gl_vertical_begin_pos + bar_height,
			1, gl_vertical_begin_pos + bar_height,
			-1, gl_vertical_begin_pos,
			1, gl_vertical_begin_pos
		};

		float left_bar_data[] = {
			-1, gl_vertical_begin_pos,
			ruler_left_pos, gl_vertical_begin_pos,
			-1, gl_vertical_pos,
			ruler_left_pos, gl_vertical_pos
		};
		float right_bar_data[] = {
			ruler_right_pos, gl_vertical_begin_pos,
			1, gl_vertical_begin_pos,
			ruler_right_pos, gl_vertical_pos,
			1, gl_vertical_pos
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(top_bar_data), top_bar_data, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBufferData(GL_ARRAY_BUFFER, sizeof(left_bar_data), left_bar_data, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBufferData(GL_ARRAY_BUFFER, sizeof(right_bar_data), right_bar_data, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisable(GL_BLEND);

}
void PdfViewOpenGLWidget::render_highlight_window(GLuint program, fz_rect window_rect, bool draw_border) {

	if (is_rotated()) {
		return;
	}

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
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
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

	render(&painter);

	//painter.drawText(-100, -100, "1234567890");
}

PdfViewOpenGLWidget::PdfViewOpenGLWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, ConfigManager* config_manager, bool is_helper, QWidget* parent) :
	QOpenGLWidget(parent),
	document_view(document_view),
	pdf_renderer(pdf_renderer),
	config_manager(config_manager),
	is_helper(is_helper)
{
	creation_time = QDateTime::currentDateTime();

	QSurfaceFormat format;
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	this->setFormat(format);

	overview_half_width = OVERVIEW_SIZE[0];
	overview_half_height = OVERVIEW_SIZE[1];

	overview_offset_x = OVERVIEW_OFFSET[0];
	overview_offset_y = OVERVIEW_OFFSET[1];
}

void PdfViewOpenGLWidget::cancel_search() {
	search_results.clear();
	current_search_result_index =-1;
	is_searching = false;
	is_search_cancelled = true;
}

void PdfViewOpenGLWidget::handle_escape() {
	cancel_search();
	synctex_highlights.clear();
	if (!SHOULD_HIGHLIGHT_LINKS) {
		should_highlight_links = false;
	}
	should_highlight_words = false;
	should_show_numbers = false;
	character_highlight_rect = {};
	wrong_character_rect = {};
}

void PdfViewOpenGLWidget::toggle_highlight_links() {
	this->should_highlight_links = !this->should_highlight_links;
}

void PdfViewOpenGLWidget::set_highlight_links(bool should_highlight, bool should_show_numbers) {
	this->should_highlight_links = should_highlight;
	this->should_show_numbers = should_show_numbers;
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


std::optional<SearchResult> PdfViewOpenGLWidget::get_current_search_result() {
	if (!valid_document()) return {};
	if (current_search_result_index == -1) return {};
	search_results_mutex.lock();
	if (search_results.size() == 0) {
		search_results_mutex.unlock();
		return {};
	}
	SearchResult res = search_results[current_search_result_index];
	search_results_mutex.unlock();
	return res;
}

std::optional<SearchResult> PdfViewOpenGLWidget::set_search_result_offset(int offset) {
	if (!valid_document()) return {};
	search_results_mutex.lock();
	if (search_results.size() == 0) {
		search_results_mutex.unlock();
		return {};
	}
	int target_index = mod(current_search_result_index + offset, search_results.size());
	current_search_result_index = target_index;

	SearchResult res = search_results[target_index];
	search_results_mutex.unlock();
	return res;

}


void PdfViewOpenGLWidget::goto_search_result(int offset, bool overview) {
	if (!valid_document()) return;

	std::optional<SearchResult> result_ = set_search_result_offset(offset);
	if (result_) {
		SearchResult result = result_.value();
		float new_offset_y = result.rects.front().y0 + document_view->get_document()->get_accum_page_height(result.page);
		if (overview) {
			OverviewState state = { new_offset_y, nullptr };
			set_overview_page(state);
		}
		else {
			document_view->set_offset_y(new_offset_y);
		}
	}
}


void PdfViewOpenGLWidget::render_overview(OverviewState overview) {
	if (!valid_document()) return;
	Document* target_doc = document_view->get_document();

	if (overview.doc) {
		target_doc = overview.doc;
	}

	DocumentPos docpos = target_doc->absolute_to_page_pos({ 0, overview.absolute_offset_y });

	float view_width = static_cast<int>(document_view->get_view_width() * overview_half_width);
	float view_height = static_cast<int>(document_view->get_view_height() * overview_half_height);
	float page_width = target_doc->get_page_width(docpos.page);
	float page_height = target_doc->get_page_height(docpos.page);
	float zoom_level = view_width / page_width;

	GLuint texture = pdf_renderer->find_rendered_page(target_doc->get_path(),
		docpos.page,
		zoom_level,
		nullptr,
		nullptr);

	fz_rect window_rect = get_overview_rect_pixel_perfect(
		document_view->get_view_width(),
		document_view->get_view_height(),
		view_width,
		view_height);

	window_rect.y0 = -window_rect.y0;
	window_rect.y1 = -window_rect.y1;

	float page_vertices[4 * 2];
	float page_uvs[4 * 2];
	float border_vertices[4 * 2];

	float offset_diff = 2 * (target_doc->get_accum_page_height(docpos.page) + target_doc->get_page_height(docpos.page) - overview.absolute_offset_y)
		* zoom_level / document_view->get_view_height();

	float page_min_x = window_rect.x0;
	float page_max_x = window_rect.x1;
	float page_max_y = (window_rect.y0 + window_rect.y1) / 2 - offset_diff;
	float page_min_y = (window_rect.y0 + window_rect.y1) / 2 - offset_diff +  2 * page_height * zoom_level / document_view->get_view_height();

	page_vertices[0] = page_min_x;
	page_vertices[1] = page_min_y;
	page_vertices[2] = page_max_x;
	page_vertices[3] = page_min_y;
	page_vertices[4] = page_min_x;
	page_vertices[5] = page_max_y;
	page_vertices[6] = page_max_x;
	page_vertices[7] = page_max_y;

	page_uvs[0] = 0.0f;
	page_uvs[1] = 0.0f;
	page_uvs[2] = 1.0f;
	page_uvs[3] = 0.0f;
	page_uvs[4] = 0.0f;
	page_uvs[5] = 1.0f;
	page_uvs[6] = 1.0f;
	page_uvs[7] = 1.0f;

	get_overview_window_vertices(border_vertices);

	enable_stencil();
	write_to_stencil();
	draw_stencil_rects({window_rect}, true);
	use_stencil_to_write(true);

	fz_rect page_rect;
	page_rect.x0 = window_rect.x0;
	page_rect.x1 = window_rect.x1;
	page_rect.y0 = window_rect.y0;
	page_rect.y1 = window_rect.y1;

	float gray_color[] = { 0.5f, 0.5f, 0.5f };
	float bg_color[] = { 1.0f, 1.0f, 1.0f };
	get_background_color(bg_color);
	glDisable(GL_BLEND);

	{ // draw background
		glUseProgram(shared_gl_objects.highlight_program);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(border_vertices), border_vertices, GL_DYNAMIC_DRAW);
		glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, bg_color);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}

	bind_program();
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	if (texture) {
		glBindTexture(GL_TEXTURE_2D, texture);

		//draw the overview
		glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(page_uvs), page_uvs, GL_DYNAMIC_DRAW);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		std::optional<SearchResult> highlighted_result_ = get_current_search_result();
		if (highlighted_result_) {
			SearchResult highlighted_result = highlighted_result_.value();
			glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
			glUseProgram(shared_gl_objects.highlight_program);
			glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"search_highlight_color"));
			//glUniform3fv(g_shared_resources.highlight_color_uniform_location, 1, highlight_color_temp);
			for (auto rect : highlighted_result.rects) {
				fz_rect target = document_to_overview_rect(highlighted_result.page, rect);
				render_highlight_window(shared_gl_objects.highlight_program, target);
			}
		}
	}

	disable_stencil();

	//draw the border
	glUseProgram(shared_gl_objects.highlight_program);
	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(border_vertices), border_vertices, GL_DYNAMIC_DRAW);
	glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, gray_color);
	glDrawArrays(GL_LINE_LOOP, 0, 4);

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), g_quad_uvs, GL_DYNAMIC_DRAW);

}

void PdfViewOpenGLWidget::render_page(int page_number) {

	if (!valid_document()) return;

	int rendered_width = -1;
	int rendered_height = -1;

	GLuint texture = pdf_renderer->find_rendered_page(document_view->get_document()->get_path(),
		page_number,
		document_view->get_zoom_level(),
		&rendered_width,
		&rendered_height);


	if (rotation_index % 2 == 1) {
		std::swap(rendered_width, rendered_height);
	}

	float page_vertices[4 * 2];
	fz_rect page_rect = { 0,
		0,
		document_view->get_document()->get_page_width(page_number),
		document_view->get_document()->get_page_height(page_number) };

#ifdef SIOYEK_QT6
	float device_pixel_ratio = static_cast<float>(QGuiApplication::primaryScreen()->devicePixelRatio());
#else
	float device_pixel_ratio = QApplication::desktop()->devicePixelRatioF();
#endif

	if (DISPLAY_RESOLUTION_SCALE > 0) {
		device_pixel_ratio *= DISPLAY_RESOLUTION_SCALE;
	}

	fz_rect window_rect = document_view->document_to_window_rect_pixel_perfect(page_number,
		page_rect,
		static_cast<int>(rendered_width / device_pixel_ratio),
		static_cast<int>(rendered_height / device_pixel_ratio)
	);
	rect_to_quad(window_rect, page_vertices);

	if (texture != 0) {

		//if (is_dark_mode) {
		//	glUseProgram(shared_gl_objects.rendered_dark_program);
		//	glUniform1f(shared_gl_objects.dark_mode_contrast_uniform_location, DARK_MODE_CONTRAST);
		//}
		//else {
		//	glUseProgram(shared_gl_objects.rendered_program);
		//}
		bind_program();

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

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), rotation_uvs[rotation_index], GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if (!is_presentation_mode()) {

		// render page separator
		glUseProgram(shared_gl_objects.separator_program);

		fz_rect separator_rect = { 0,
			document_view->get_document()->get_page_height(page_number) - PAGE_SEPARATOR_WIDTH / 2,
			document_view->get_document()->get_page_width(page_number),
			document_view->get_document()->get_page_height(page_number) + PAGE_SEPARATOR_WIDTH / 2};

//fz_rect DocumentView::document_to_window_rect_pixel_perfect(int page, fz_rect doc_rect, int pixel_width, int pixel_height) {
		if (PAGE_SEPARATOR_WIDTH > 0) {

			fz_rect separator_window_rect = document_view->document_to_window_rect(page_number, separator_rect);
			rect_to_quad(separator_window_rect, page_vertices);

			glUniform3fv(shared_gl_objects.separator_background_color_uniform_location, 1, PAGE_SEPARATOR_COLOR);

			glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
			glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
	}

}

void PdfViewOpenGLWidget::render(QPainter* painter) {

	painter->beginNativePainting();
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glBindVertexArray(vertex_array_object);


	if (!valid_document()) {

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		if (is_helper) {
			//painter->endNativePainting();
			draw_empty_helper_message(painter);
		}
		return;
	}

	std::vector<int> visible_pages;
	document_view->get_visible_pages(document_view->get_view_height(), visible_pages);

	if (color_mode == ColorPalette::Dark) {
		glClearColor(DARK_MODE_BACKGROUND_COLOR[0], DARK_MODE_BACKGROUND_COLOR[1], DARK_MODE_BACKGROUND_COLOR[2], 1.0f);
	}
	else if (color_mode == ColorPalette::Custom) {
		glClearColor(CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[0], CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[1], CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[2], 1.0f);
	}
	else {
		glClearColor(BACKGROUND_COLOR[0], BACKGROUND_COLOR[1], BACKGROUND_COLOR[2], 1.0f);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


	std::vector<std::pair<int, fz_link*>> all_visible_links;

	if (is_presentation_mode()) {
		if (PRERENDER_NEXT_PAGE) {
			GLuint texture = pdf_renderer->find_rendered_page(document_view->get_document()->get_path(),
				visible_page_number.value() + 1,
				document_view->get_zoom_level(),
				nullptr,
				nullptr);
		}
		render_page(visible_page_number.value());
	}
	else {
		for (int page : visible_pages) {
			render_page(page);

			if (should_highlight_links) {
				glUseProgram(shared_gl_objects.highlight_program);
				glUniform3fv(shared_gl_objects.highlight_color_uniform_location,
					1,
					config_manager->get_config<float>(L"link_highlight_color"));
				//fz_link* links = document_view->get_document()->get_page_links(page);
				//while (links != nullptr) {
				//	render_highlight_document(shared_gl_objects.highlight_program, page, links->rect);
				//	all_visible_links.push_back(std::make_pair(page, links));
				//	links = links->next;
				//}
			}
		}
		// prerender pages
		if (visible_pages.size() > 0) {
			int num_pages = document_view->get_document()->num_pages();
			int max_page = visible_pages[visible_pages.size() - 1];
			for (int i = 1; i < (PRERENDERED_PAGE_COUNT + 1); i++) {
				if (max_page + i < num_pages) {
					pdf_renderer->find_rendered_page(document_view->get_document()->get_path(),
						max_page + i,
						document_view->get_zoom_level(),
						nullptr,
						nullptr);
				}
			}
		}
	}

	if (fastread_mode) {

		auto rects = document_view->get_document()->get_highlighted_character_masks(document_view->get_center_page_number());

		if (rects.size() > 0) {
			enable_stencil();
			write_to_stencil();
			draw_stencil_rects(rects, false, document_view->get_center_page_number());
			use_stencil_to_write(false);
			render_transparent_background();
			disable_stencil();

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


	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);

	search_results_mutex.lock();
	if (search_results.size() > 0) {

		int index = current_search_result_index;
		if (index == -1) index = 0;

		SearchResult current_search_result = search_results[index];
		glUseProgram(shared_gl_objects.highlight_program);
		//glUniform3fv(g_shared_resources.highlight_color_uniform_location, 1, highlight_color_temp);

		if (SHOULD_HIGHLIGHT_UNSELECTED_SEARCH) {

			std::vector<int> visible_search_indices = get_visible_search_results(visible_pages);
			glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, UNSELECTED_SEARCH_HIGHLIGHT_COLOR);
			for (int visible_search_index : visible_search_indices) {
				if (visible_search_index != current_search_result_index) {
					SearchResult res = search_results[visible_search_index];
					for (auto rect : res.rects) {
						render_highlight_document(shared_gl_objects.highlight_program, res.page, rect);
					}
				}
			}
		}

		glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"search_highlight_color"));
		for (auto rect : current_search_result.rects) {
			render_highlight_document(shared_gl_objects.highlight_program, current_search_result.page, rect);
		}
	}
	search_results_mutex.unlock();

	glUseProgram(shared_gl_objects.highlight_program);
	glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"text_highlight_color"));
	std::vector<fz_rect> bounding_rects;
	merge_selected_character_rects(*document_view->get_selected_character_rects(), bounding_rects);
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
		for (size_t i = 0; i < highlights.size(); i++) {
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

			if (selection_begin_window_y > selection_end_window_y) {
				std::swap(selection_begin_window_y, selection_end_window_y);
			}

			bool is_selection_in_window = range_intersects(selection_begin_window_y, selection_end_window_y, -1.0f, 1.0f);

			if (is_selection_in_window) {
				for (size_t j = 0; j < highlights[i].highlight_rects.size(); j++) {
					glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &HIGHLIGHT_COLORS[(highlights[i].type - 'a') * 3]);
					render_highlight_absolute(shared_gl_objects.highlight_program, highlights[i].highlight_rects[j], false);
				}
			}
		}
	}

	if (should_draw_vertical_line) {
		//render_line_window(shared_gl_objects.vertical_line_program ,vertical_line_location);

		float vertical_line_end = document_view->get_ruler_window_y();
		std::optional<fz_rect> ruler_rect = document_view->get_ruler_window_rect();

		if (color_mode == ColorPalette::Dark) {
			render_line_window(shared_gl_objects.vertical_line_dark_program , vertical_line_end, ruler_rect);
		}
		else {
			render_line_window(shared_gl_objects.vertical_line_program , vertical_line_end, ruler_rect);
		}
	}
	if (overview_page) {
		render_overview(overview_page.value());
	}


	painter->endNativePainting();

	if (should_highlight_words && (!overview_page)) {
		setup_text_painter(painter);

		std::vector<std::string> tags = get_tags(word_rects.size());

		for (size_t i = 0; i < word_rects.size(); i++) {
			auto [rect, page] = word_rects[i];


			fz_rect window_rect = document_view->document_to_window_rect(page, rect);

			int view_width = static_cast<float>(document_view->get_view_width());
			int view_height = static_cast<float>(document_view->get_view_height());
			
			int window_x0 = static_cast<int>(window_rect.x0 * view_width / 2 + view_width / 2);
			int window_y0 = static_cast<int>(-window_rect.y0 * view_height / 2 + view_height / 2);

			if (i > 0) {
				if (std::abs(word_rects[i - 1].first.x0 - rect.x0) < 5) {
					window_y0 = static_cast<int>(-window_rect.y1 * view_height / 2 + view_height / 2);
				}
			}

			int window_y1 = static_cast<int>(-window_rect.y1 * view_height / 2 + view_height / 2);

			painter->drawText(window_x0, (window_y0 + window_y1) / 2, tags[i].c_str());
		}
	}

	if (should_highlight_links && should_show_numbers && (!overview_page)) {

		document_view->get_visible_links(all_visible_links);
		setup_text_painter(painter);
		for (size_t i = 0; i < all_visible_links.size(); i++) {
			std::stringstream ss;
			ss << i;
			std::string index_string = ss.str();

			if (ALPHABETIC_LINK_TAGS) {
				index_string = get_aplph_tag(i, all_visible_links.size());
			}

			auto [page, link] = all_visible_links[i];

			bool should_draw = true;

			// some malformed doucments have multiple overlapping links which makes reading
			// the link labels difficult. Here we only draw the link text if there are no
			// other close links. This has quadratic runtime but it should not matter since
			// there are not many links in a single PDF page.
			for (int j = i+1; j < all_visible_links.size(); j++) {
				auto [other_page, other_link] = all_visible_links[j];
				float distance = std::abs(other_link->rect.x0 - link->rect.x0) + std::abs(other_link->rect.y0 - link->rect.y0);
				if (distance < 10) {
					should_draw = false;
				}
			}

			//document_view->document_to_window_pos_in_pixels(page, link->rect.x0, link->rect.x1, &window_x, &window_y);
			fz_rect window_rect = document_view->document_to_window_rect(page, link->rect);

			int view_width = static_cast<float>(document_view->get_view_width());
			int view_height = static_cast<float>(document_view->get_view_height());
			
			int window_x = static_cast<int>(window_rect.x0 * view_width / 2 + view_width / 2);
			int window_y = static_cast<int>(-window_rect.y0 * view_height / 2 + view_height / 2);

			if (should_draw) {
				painter->drawText(window_x, window_y, index_string.c_str());
			}
		}
	}
	if (character_highlight_rect) {
		float rectangle_color[] = {0.0f, 1.0f, 1.0f};
		glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, rectangle_color);
		render_highlight_absolute(shared_gl_objects.highlight_program, character_highlight_rect.value());

		if (wrong_character_rect) {
			float wrong_color[] = {1.0f, 0.0f, 0.0f};
			glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, wrong_color);
			render_highlight_absolute(shared_gl_objects.highlight_program, wrong_character_rect.value());
		}
	}
	if (selected_rectangle) {
		enable_stencil();
		write_to_stencil();
		float rectangle_color[] = {0.0f, 0.0f, 0.0f};
		glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, rectangle_color);
		render_highlight_absolute(shared_gl_objects.highlight_program, selected_rectangle.value());

		use_stencil_to_write(false);
		fz_rect window_rect = {-1, -1, 1, 1};
		render_highlight_window(shared_gl_objects.highlight_program, window_rect, true);

		disable_stencil();
	}

}

bool PdfViewOpenGLWidget::get_is_searching(float* prog) {
	if (is_search_cancelled) {
		return false;
	}

	search_results_mutex.lock();
	if (is_searching) {
		if (prog) {
			*prog = percent_done;
		}
	}
	search_results_mutex.unlock();
	return true;
}

void PdfViewOpenGLWidget::search_text(const std::wstring& text, bool case_sensitive, bool regex, std::optional<std::pair<int,int>> range) {

	if (!document_view) return;

	search_results_mutex.lock();
	search_results.clear();
	current_search_result_index = -1;
	search_results_mutex.unlock();

	int min_page = -1;
	int max_page = 2147483647;
	if (range.has_value()) {
		min_page = range.value().first;
		max_page = range.value().second;
	}

	if (document_view->get_document()->is_super_fast_index_ready()) {
		if (!text.empty()) {
			int current_page = document_view->get_center_page_number();
			std::vector<SearchResult> results;
			if (regex) {
				results = document_view->get_document()->search_regex(text, case_sensitive, current_page, min_page, max_page);
			}
			else {
				results = document_view->get_document()->search_text(text, case_sensitive, current_page, min_page, max_page);
			}
			search_results = std::move(results);
			is_searching = false;
			is_search_cancelled = false;
			percent_done = 1.0f;
		}
	}
	else {

		is_searching = true;
		is_search_cancelled = false;

		int current_page = document_view->get_center_page_number();
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
}

void PdfViewOpenGLWidget::set_dark_mode(bool mode) {
	if (mode == true) {
		this->color_mode = ColorPalette::Dark;
	}
	else {
		this->color_mode = ColorPalette::Normal;
	}
}

void PdfViewOpenGLWidget::toggle_dark_mode() {
	set_dark_mode(!(this->color_mode == ColorPalette::Dark));
}

void PdfViewOpenGLWidget::set_synctex_highlights(std::vector<std::pair<int, fz_rect>> highlights) {
	synctex_highlights = std::move(highlights);
}

void PdfViewOpenGLWidget::on_document_view_reset() {
	this->synctex_highlights.clear();
}

PdfViewOpenGLWidget::~PdfViewOpenGLWidget() {
	if (is_opengl_initialized) {
		//glDeleteVertexArrays(1, &vertex_array_object);
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

void PdfViewOpenGLWidget::set_should_draw_vertical_line(bool val) {
	should_draw_vertical_line = val;
}

bool PdfViewOpenGLWidget::get_should_draw_vertical_line() {
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

		bool is_control_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)
			|| QApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier);

		if (is_control_pressed) {
			if (wevent->angleDelta().y() > 0) {
				float pev_zoom_level = document_view->get_zoom_level();
				float new_zoom_level = pev_zoom_level * ZOOM_INC_FACTOR;
				document_view->set_zoom_level(new_zoom_level, true);
			}

			if (wevent->angleDelta().y() < 0) {
				float pev_zoom_level = document_view->get_zoom_level();
				float new_zoom_level = pev_zoom_level / ZOOM_INC_FACTOR;
				document_view->set_zoom_level(new_zoom_level, true);
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
void PdfViewOpenGLWidget::set_overview_page(std::optional<OverviewState> overview) {
	if (overview.has_value()) {
		Document* target = document_view->get_document();
		if (overview.value().doc != nullptr) {
			target = overview.value().doc;
		}

		float offset = overview.value().absolute_offset_y;
		if (offset < 0) {
			overview.value().absolute_offset_y = 0;
		}
		if (offset > target->max_y_offset()) {
			overview.value().absolute_offset_y = target->max_y_offset();
		}
	}
	
	this->overview_page = overview;
}

std::optional<OverviewState> PdfViewOpenGLWidget::get_overview_page() {
	return overview_page;
}

void PdfViewOpenGLWidget::draw_empty_helper_message(QPainter* painter) {
	// should be called with native painting disabled

	QString message = "No portals yet";
	QFontMetrics fm(QApplication::font());
#ifdef SIOYEK_QT6
	int message_width = fm.boundingRect(message).width();
#else
	int message_width = fm.width(message);
#endif
	int message_height = fm.height();

	int view_width = document_view->get_view_width();
	int view_height = document_view->get_view_height();

	painter->drawText(view_width / 2 - message_width / 2, view_height / 2 - message_height / 2, message);
}

void PdfViewOpenGLWidget::set_visible_page_number(std::optional<int> val) {
	this->visible_page_number = val;
}

bool PdfViewOpenGLWidget::is_presentation_mode() {
	if (visible_page_number) {
		return true;
	}
	return false;
}

fz_rect	PdfViewOpenGLWidget::get_overview_rect() {
	fz_rect res;
	res.x0 = overview_offset_x - overview_half_width;
	res.y0 = -overview_offset_y - overview_half_height;
	res.x1 = overview_offset_x + overview_half_width;
	res.y1 = -overview_offset_y + overview_half_height;
	return res;
}

fz_rect	PdfViewOpenGLWidget::get_overview_rect_pixel_perfect(int widget_width, int widget_height, int view_width, int view_height) {
	fz_rect res;
	int x0_pixel = static_cast<int>((((overview_offset_x - overview_half_width) + 1.0f) / 2.0f) * widget_width);
	int x1_pixel = x0_pixel + view_width;
	int y0_pixel = static_cast<int>((((-overview_offset_y - overview_half_height) + 1.0f) / 2.0f) * widget_height);
	int y1_pixel = y0_pixel + view_height;

	res.x0 = (static_cast<float>(x0_pixel) / static_cast<float>(widget_width)) * 2.0f - 1.0f;
	res.x1 = (static_cast<float>(x1_pixel) / static_cast<float>(widget_width)) * 2.0f - 1.0f;
	res.y0 = (static_cast<float>(y0_pixel) / static_cast<float>(widget_height)) * 2.0f - 1.0f;
	res.y1 = (static_cast<float>(y1_pixel) / static_cast<float>(widget_height)) * 2.0f - 1.0f;

	return res;
}

std::vector<fz_rect> PdfViewOpenGLWidget::get_overview_border_rects() {
	std::vector<fz_rect> res;

	fz_rect bottom_rect;
	fz_rect top_rect;
	fz_rect left_rect;
	fz_rect right_rect;

	bottom_rect.x0 = overview_offset_x - overview_half_width;
	bottom_rect.y0 = -overview_offset_y - overview_half_height - 0.05f;
	bottom_rect.x1 = overview_offset_x + overview_half_width;
	bottom_rect.y1 = -overview_offset_y - overview_half_height + 0.05f;

	top_rect.x0 = overview_offset_x - overview_half_width;
	top_rect.y0 = -overview_offset_y + overview_half_height - 0.05f;
	top_rect.x1 = overview_offset_x + overview_half_width;
	top_rect.y1 = -overview_offset_y + overview_half_height + 0.05f;

	left_rect.x0 = overview_offset_x - overview_half_width - 0.05f;
	left_rect.y0 = -overview_offset_y - overview_half_height;
	left_rect.x1 = overview_offset_x - overview_half_width + 0.05f;
	left_rect.y1 = -overview_offset_y + overview_half_height;

	right_rect.x0 = overview_offset_x + overview_half_width - 0.05f;
	right_rect.y0 = -overview_offset_y - overview_half_height;
	right_rect.x1 = overview_offset_x + overview_half_width + 0.05f;
	right_rect.y1 = -overview_offset_y + overview_half_height;

	res.push_back(bottom_rect);
	res.push_back(top_rect);
	res.push_back(left_rect);
	res.push_back(right_rect);

	return res;
}

bool PdfViewOpenGLWidget::is_window_point_in_overview(fvec2 window_point) {
	if (get_overview_page()) {
		fz_point point{ window_point.x(), window_point.y()};
		fz_rect rect = get_overview_rect();
		bool res = fz_is_point_inside_rect(point, rect);
		return res;
	}
	return false;
}

bool PdfViewOpenGLWidget::is_window_point_in_overview_border(float window_x, float window_y, OverviewSide* which_border) {

	if (!get_overview_page().has_value()) {
		return false;
	}

	fz_point point{ window_x, window_y };
	std::vector<fz_rect> rects = get_overview_border_rects();
	for (size_t i = 0; i < rects.size(); i++) {
		if (fz_is_point_inside_rect(point, rects[i])) {
			*which_border = static_cast<OverviewSide>(i);
			return true;
		}
	}
	return false;
}

void PdfViewOpenGLWidget::get_overview_offsets(float* offset_x, float* offset_y) {
	*offset_x = overview_offset_x;
	*offset_y = overview_offset_y;
}
void PdfViewOpenGLWidget::set_overview_offsets(float offset_x, float offset_y) {
	overview_offset_x = offset_x;
	overview_offset_y = offset_y;
}

void PdfViewOpenGLWidget::set_overview_offsets(fvec2 offsets) {
	set_overview_offsets(offsets.x(), offsets.y());
}

float PdfViewOpenGLWidget::get_overview_side_pos(int index) {
	if (index == OverviewSide::bottom) {
		return overview_offset_y - overview_half_height;
	}
	if (index == OverviewSide::top) {
		return overview_offset_y + overview_half_height;
	}
	if (index == OverviewSide::left) {
		return overview_offset_x - overview_half_width;
	}
	if (index == OverviewSide::right) {
		return overview_offset_x + overview_half_width;
	}
}

void PdfViewOpenGLWidget::set_overview_side_pos(int index, fz_rect original_rect, fvec2 diff) {

	fz_rect new_rect = original_rect;

	if (index == OverviewSide::bottom) {
		float new_bottom_pos = original_rect.y0 + diff.y();
		if (new_bottom_pos < original_rect.y1) {
			new_rect.y0 = new_bottom_pos;
		}
	}
	if (index == OverviewSide::top) {
		float new_top_pos = original_rect.y1 + diff.y();
		if (new_top_pos > original_rect.y0) {
			new_rect.y1 = new_top_pos;
		}
	}
	if (index == OverviewSide::left) {
		float new_left_pos = original_rect.x0 + diff.x();
		if (new_left_pos < original_rect.x1) {
			new_rect.x0 = new_left_pos;
		}
	}
	if (index == OverviewSide::right) {
		float new_right_pos = original_rect.x1 + diff.x();
		if (new_right_pos > original_rect.x0) {
			new_rect.x1 = new_right_pos;
		}
	}
	set_overview_rect(new_rect);

}

void PdfViewOpenGLWidget::set_overview_rect(fz_rect rect) {
	float halfwidth = (rect.x1 - rect.x0) / 2;
	float halfheight = (rect.y1 - rect.y0) / 2;
	float offset_x = rect.x0 + halfwidth;
	float offset_y = rect.y0 + halfheight;

	overview_offset_x = offset_x;
	overview_offset_y = -offset_y;
	overview_half_width = halfwidth;
	overview_half_height = halfheight;
}

void PdfViewOpenGLWidget::set_custom_color_mode(bool mode) {
	if (mode) {
		this->color_mode = ColorPalette::Custom;
	}
	else {
		this->color_mode = ColorPalette::Normal;
	}
}

void PdfViewOpenGLWidget::toggle_custom_color_mode() {
	set_custom_color_mode(!(this->color_mode == ColorPalette::Custom));
}

void PdfViewOpenGLWidget::bind_program() {
	if (color_mode == ColorPalette::Dark) {
		glUseProgram(shared_gl_objects.rendered_dark_program);
		glUniform1f(shared_gl_objects.dark_mode_contrast_uniform_location, DARK_MODE_CONTRAST);
	}
	else if (color_mode == ColorPalette::Custom) {
		glUseProgram(shared_gl_objects.custom_color_program);
		float transform_matrix[16];
		get_custom_color_transform_matrix(transform_matrix);
		glUniformMatrix4fv(shared_gl_objects.custom_color_transform_uniform_location, 1, GL_TRUE, transform_matrix);

	}
	else {
		glUseProgram(shared_gl_objects.rendered_program);
	}
}

DocumentPos PdfViewOpenGLWidget::window_pos_to_overview_pos(NormalizedWindowPos window_pos) {
	Document* target = document_view->get_document();
	if (overview_page) {
		if (overview_page.value().doc != nullptr) {
			target = overview_page.value().doc;
		}
	}

	float window_width = static_cast<float>(size().width());
	float window_height = static_cast<float>(size().height());
	int window_x = static_cast<int>((1.0f + window_pos.x) / 2 * window_width);
	int window_y = static_cast<int>((1.0f + window_pos.y) / 2 * window_height);
	DocumentPos docpos = target->absolute_to_page_pos({ 0, get_overview_page().value().absolute_offset_y });
	float overview_width = document_view->get_view_width() * overview_half_width;
	float page_width = target->get_page_width(docpos.page);
	float zoom_level = overview_width / page_width;

	int overview_left = (-overview_half_width + overview_offset_x) * window_width / 2 + window_width / 2;
	int overview_mid = ( - overview_offset_y) * window_height / 2 + window_height / 2;

	int relative_window_x = static_cast<int>(static_cast<float>(window_x - overview_left) / zoom_level);
	int relative_window_y = static_cast<int>(static_cast<float>(window_y - overview_mid) / zoom_level);

	float doc_offset_x = relative_window_x;
	float doc_offset_y = docpos.y + relative_window_y;
	int doc_page = docpos.page;
	return {doc_page, doc_offset_x, doc_offset_y};
}

void PdfViewOpenGLWidget::toggle_highlight_words() {
	this->should_highlight_words = !this->should_highlight_words;
}

void PdfViewOpenGLWidget::set_highlight_words(std::vector<std::pair<fz_rect, int>>& rects) {
	word_rects = std::move(rects);
}

void PdfViewOpenGLWidget::set_should_highlight_words(bool should_highlight) {
	this->should_highlight_words = should_highlight;
}

void PdfViewOpenGLWidget::rotate_clockwise() {
	rotation_index = (rotation_index + 1) % 4;
}

void PdfViewOpenGLWidget::rotate_counterclockwise() {
	rotation_index = (rotation_index - 1) % 4;
	if (rotation_index < 0) {
		rotation_index += 4;
	}
}

bool PdfViewOpenGLWidget::is_rotated() {
	return rotation_index != 0;
}

void PdfViewOpenGLWidget::enable_stencil() {
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
}

void PdfViewOpenGLWidget::write_to_stencil() {
	glStencilFunc(GL_NEVER, 1, 0xFF);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
}

void PdfViewOpenGLWidget::use_stencil_to_write(bool eq) {
	//glStencilFunc(GL_EQUAL, 1, 0xFF);
	if (eq) {
		glStencilFunc(GL_EQUAL, 1, 0xFF);
	}
	else {
		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	}
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void PdfViewOpenGLWidget::disable_stencil() {
	glDisable(GL_STENCIL_TEST);
}

void PdfViewOpenGLWidget::render_transparent_background() {

	float bar_data[] = {
		-1, -1,
		1, -1,
		-1, 1,
		1, 1
	};

	glDisable(GL_CULL_FACE);
	glUseProgram(shared_gl_objects.vertical_line_program);

	float background_color[4] = { 1.0f, 1.0f, 1.0f, 1-FASTREAD_OPACITY };

	if (this->color_mode == ColorPalette::Normal) {
	}
	else if (this->color_mode == ColorPalette::Dark) {
		background_color[0] = background_color[1] = background_color[2] = 0;
	}
	else {
		background_color[0] = CUSTOM_BACKGROUND_COLOR[0];
		background_color[1] = CUSTOM_BACKGROUND_COLOR[1];
		background_color[2] = CUSTOM_BACKGROUND_COLOR[2];
	}

	glUniform4fv(shared_gl_objects.line_color_uniform_location,
		1,
		background_color);

	float time = -QDateTime::currentDateTime().msecsTo(creation_time);
	glUniform1f(shared_gl_objects.line_time_uniform_location, time);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(bar_data), bar_data, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisable(GL_BLEND);
}

void PdfViewOpenGLWidget::draw_stencil_rects(const std::vector<fz_rect>& rects, bool is_window_rect, int page) {

	std::vector<float> window_rects;
	for (auto rect : rects) {
		fz_rect window_rect;
		if (is_window_rect) {
			window_rect = rect;
		}
		else {

			window_rect = document_view->document_to_window_rect(page, rect);
		}
		float triangle1[6] = {
			window_rect.x0, window_rect.y0,
			window_rect.x0, window_rect.y1,
			window_rect.x1, window_rect.y0
		};
		float triangle2[6] = {
			window_rect.x1, window_rect.y0,
			window_rect.x0, window_rect.y1,
			window_rect.x1, window_rect.y1
		};
		for (int i = 0; i < 6; i++) window_rects.push_back(triangle1[i]);
		for (int i = 0; i < 6; i++) window_rects.push_back(triangle2[i]);
	}

	glUseProgram(shared_gl_objects.stencil_program);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, window_rects.size() * sizeof(float), window_rects.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, rects.size() * 6);
	glDisableVertexAttribArray(0);

}

void PdfViewOpenGLWidget::toggle_fastread_mode() {
	fastread_mode = !fastread_mode;
}

void PdfViewOpenGLWidget::get_overview_size(float* width, float* height) {
	*width = overview_half_width;
	*height = overview_half_height;
}

void PdfViewOpenGLWidget::setup_text_painter(QPainter* painter) {

	int bgcolor[4];
	int textcolor[4];

	convert_color4(KEYBOARD_SELECT_BACKGROUND_COLOR, bgcolor);
	convert_color4(KEYBOARD_SELECT_TEXT_COLOR, textcolor);

	QBrush background_brush = QBrush(QColor(bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3]));
	QFont font;
	font.setPixelSize(KEYBOARD_SELECT_FONT_SIZE);
	painter->setBackgroundMode(Qt::BGMode::OpaqueMode);
	painter->setBackground(background_brush);
	painter->setPen(QColor(textcolor[0], textcolor[1], textcolor[2], textcolor[3]));
	painter->setFont(font);

}

void PdfViewOpenGLWidget::get_overview_window_vertices(float out_vertices[2*4]) {

	out_vertices[0] = overview_offset_x - overview_half_width;
	out_vertices[1] = overview_offset_y + overview_half_height;
	out_vertices[2] = overview_offset_x - overview_half_width;
	out_vertices[3] = overview_offset_y -overview_half_height;
	out_vertices[4] = overview_offset_x + overview_half_width;
	out_vertices[5] = overview_offset_y - overview_half_height;
	out_vertices[6] = overview_offset_x + overview_half_width;
	out_vertices[7] = overview_offset_y + overview_half_height;
}

void PdfViewOpenGLWidget::set_selected_rectangle(fz_rect selected) {
	selected_rectangle = selected;
}

void PdfViewOpenGLWidget::clear_selected_rectangle() {
	selected_rectangle = {};
}
std::optional<fz_rect> PdfViewOpenGLWidget::get_selected_rectangle() {
	return selected_rectangle;
}

void PdfViewOpenGLWidget::set_typing_rect(int page, fz_rect highlight_rect, std::optional<fz_rect> wrong_rect) {
	fz_rect absrect = document_view->get_document()->document_to_absolute_rect(page, highlight_rect, true);
	character_highlight_rect = absrect;

	if (wrong_rect) {
		fz_rect abswrong = document_view->get_document()->document_to_absolute_rect(page, wrong_rect.value(), true);
		wrong_character_rect = abswrong;
	}
	else {
		wrong_character_rect = {};
	}

}

Document* PdfViewOpenGLWidget::get_current_overview_document() {
	if (overview_page) {
		if (overview_page.value().doc) {
			return overview_page->doc;
		}
		else {
			return document_view->get_document();
		}
	}
	else {
		return document_view->get_document();
	}

}

NormalizedWindowPos PdfViewOpenGLWidget::document_to_overview_pos(DocumentPos pos) {
	NormalizedWindowPos res;

	if (overview_page) {
		OverviewState overview = overview_page.value();
		Document* target_doc = get_current_overview_document();
		DocumentPos docpos = target_doc->absolute_to_page_pos({ 0, overview.absolute_offset_y });

		AbsoluteDocumentPos abspos = target_doc->document_to_absolute_pos(pos);

		float overview_zoom_level = (2 * overview_half_width) / target_doc->get_page_width(docpos.page);

		float relative_x = abspos.x * overview_zoom_level;
		float aspect = static_cast<float>(width()) / static_cast<float>(height());
		float relative_y = (abspos.y - overview.absolute_offset_y) * overview_zoom_level * aspect;
		float left = overview_offset_x - overview_half_width;
		float top = overview_offset_y;
		return {left + relative_x, top - relative_y};

		return res;
	}
	else {
		return res;
	}
}

fz_rect PdfViewOpenGLWidget::document_to_overview_rect(int page, fz_rect document_rect) {
	fz_rect res;
	DocumentPos top_left = { page, document_rect.x0, document_rect.y0 };
	DocumentPos bottom_right = { page, document_rect.x1, document_rect.y1 };
	NormalizedWindowPos top_left_pos = document_to_overview_pos(top_left);
	NormalizedWindowPos bottom_right_pos = document_to_overview_pos(bottom_right);
	res.x0 = top_left_pos.x;
	res.y0 = top_left_pos.y;
	res.x1 = bottom_right_pos.x;
	res.y1 = bottom_right_pos.y;
	return res;
}

std::vector<int> PdfViewOpenGLWidget::get_visible_search_results(std::vector<int>& visible_pages) {
	std::vector<int> res;

	int index = find_search_index_for_visible_pages(visible_pages);
	if (index == -1) return res;

	auto next_index = [&](int ind) {
		return (ind + 1) % search_results.size();
	};

	auto prev_index = [&](int ind) {
		int res = ind - 1;
		if (res == -1) {
			return static_cast<int>(search_results.size()-1);
		}
		return res;
	};

	auto is_page_visible = [&](int page) {
		return std::find(visible_pages.begin(), visible_pages.end(), search_results[page].page) != visible_pages.end();
	};

	res.push_back(index);
	if (search_results.size() > 0) {
		int next = next_index(index);
		while ((next != index) && is_page_visible(next)){
			res.push_back(next);
			next = next_index(next);
		}
		if (next != index) {
			int prev = prev_index(index);
			while (is_page_visible(prev)){
				res.push_back(prev);
				prev = prev_index(prev);
			}

		}
	}

	return res;
}

int PdfViewOpenGLWidget::find_search_index_for_visible_pages(std::vector<int>& visible_pages) {
	// finds some search index located in the visible pages

	int breakpoint = find_search_results_breakpoint();
	for (int page : visible_pages) {
		int index = find_search_index_for_visible_page(page, breakpoint);
		if (index != -1) {
			return index;
		}
	}
	return -1;
}

int PdfViewOpenGLWidget::find_search_index_for_visible_page(int page, int breakpoint) {
	// array is sorted, only one binary search
	if ((breakpoint == search_results.size()-1) || (search_results.size() == 1)) {
		return find_search_result_for_page_range(page, 0, breakpoint);
	}
	else {
		int index = find_search_result_for_page_range(page, 0, breakpoint);
		if (index != -1) return index;
		return find_search_result_for_page_range(page, breakpoint + 1, search_results.size() - 1);
	}
}

int PdfViewOpenGLWidget::find_search_results_breakpoint() {
	if (search_results.size() > 0) {
		int begin_index = 0;
		int end_index = search_results.size() - 1;
		return find_search_results_breakpoint_helper(begin_index, end_index);
	}
	else {
		return -1;
	}
}

int PdfViewOpenGLWidget::find_search_result_for_page_range(int page, int range_begin, int range_end) {
	if (range_begin > range_end) {
		return find_search_result_for_page_range(page, range_end, range_begin);
	}

	int midpoint = (range_begin + range_end) / 2;
	if (midpoint == range_begin) {
		if (search_results[range_begin].page == page) {
			return range_begin;
		}
		if (search_results[range_end].page == page) {
			return range_end;
		}
		return -1;
	}
	else {
		if (search_results[midpoint].page >= page) {
			return find_search_result_for_page_range(page, range_begin, midpoint);
		}
		else {
			return find_search_result_for_page_range(page, midpoint, range_end);
		}
	}

}

int PdfViewOpenGLWidget::find_search_results_breakpoint_helper(int begin_index, int end_index) {
	int midpoint = (begin_index + end_index) / 2;
	if (midpoint == begin_index) {
		if (search_results[end_index].page > search_results[begin_index].page) {
			return end_index;
		}
		else{
			return begin_index;
		}
	}
	else {
		if (search_results[midpoint].page >= search_results[begin_index].page) {
			return find_search_results_breakpoint_helper(midpoint, end_index);
		}
		else{
			return find_search_results_breakpoint_helper(begin_index, midpoint);
		}
	}
}

std::vector<std::pair<fz_rect, int>> PdfViewOpenGLWidget::get_highlight_word_rects() {
	return word_rects;
}

void PdfViewOpenGLWidget::get_custom_color_transform_matrix(float matrix_data[16]) {
	float inputs_inverse[16] = { 0, 1, 0, 0, -1, 1, -1, 1, 1, -1, 0, 0, 0, -1, 1, 0 };
	float outputs[16] = {
		CUSTOM_BACKGROUND_COLOR[0], CUSTOM_TEXT_COLOR[0], 1, 0,
		CUSTOM_BACKGROUND_COLOR[1], CUSTOM_TEXT_COLOR[1], CUSTOM_COLOR_CONTRAST * (1 - CUSTOM_BACKGROUND_COLOR[1]), CUSTOM_COLOR_CONTRAST * (1 - CUSTOM_BACKGROUND_COLOR[1]),
		CUSTOM_BACKGROUND_COLOR[2], CUSTOM_TEXT_COLOR[2], 0, 1,
		CUSTOM_BACKGROUND_COLOR[3], CUSTOM_TEXT_COLOR[3], 1, 1,
	};

	matmul<4,4,4>(outputs, inputs_inverse, matrix_data);
}

void PdfViewOpenGLWidget::get_background_color(float out_background[3]) {

	if (this->color_mode == ColorPalette::Normal) {
		out_background[0] = out_background[1] = out_background[2] = 1;
	}
	else if (this->color_mode == ColorPalette::Dark) {
		out_background[0] = out_background[1] = out_background[2] = 0;
	}
	else {
		out_background[0] = CUSTOM_BACKGROUND_COLOR[0];
		out_background[1] = CUSTOM_BACKGROUND_COLOR[1];
		out_background[2] = CUSTOM_BACKGROUND_COLOR[2];
	}
}

void PdfViewOpenGLWidget::clear_all_selections() {
	cancel_search();
	document_view->selected_character_rects.clear();
}
