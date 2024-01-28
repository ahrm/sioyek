#include <cmath>

#include <qcolor.h>
#include <qmouseevent.h>
#include <qapplication.h>
#include <qdatetime.h>
#include <qfile.h>

#include "pdf_view_opengl_widget.h"
#include "path.h"
#include "book.h"
#include "document.h"
#include "document_view.h"
#include "pdf_renderer.h"
#include "config.h"
#include "utils.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#ifndef GL_PRIMITIVE_RESTART_FIXED_INDEX
#define GL_PRIMITIVE_RESTART_FIXED_INDEX  0x8D69
#endif

extern bool DEBUG_DISPLAY_FREEHAND_POINTS;
extern bool DEBUG_SMOOTH_FREEHAND_DRAWINGS;
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
extern int NUM_H_SLICES;
extern int NUM_V_SLICES;
extern bool SLICED_RENDERING;
//extern float BOOKMARK_RECT_SIZE;
extern bool RENDER_FREETEXT_BORDERS;
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern float STRIKE_LINE_WIDTH;
extern std::wstring RULER_DISPLAY_MODE;
extern float RULER_COLOR[3];
extern float RULER_MARKER_COLOR[3];
extern float HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT;
extern bool ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE;
extern bool HIDE_OVERLAPPING_LINK_LABELS;
extern bool PRESERVE_IMAGE_COLORS;

extern int NUM_PRERENDERED_NEXT_SLIDES;
extern int NUM_PRERENDERED_PREV_SLIDES;

extern float DEFAULT_SEARCH_HIGHLIGHT_COLOR[3];
extern float DEFAULT_LINK_HIGHLIGHT_COLOR[3];
extern float DEFAULT_SYNCTEX_HIGHLIGHT_COLOR[3];
extern float DEFAULT_TEXT_HIGHLIGHT_COLOR[3];
extern float DEFAULT_VERTICAL_LINE_COLOR[4];

extern int RULER_UNDERLINE_PIXEL_WIDTH;
extern UIRect PORTRAIT_EDIT_PORTAL_UI_RECT;
extern UIRect LANDSCAPE_EDIT_PORTAL_UI_RECT;

extern UIRect PORTRAIT_BACK_UI_RECT;
extern UIRect PORTRAIT_FORWARD_UI_RECT;
extern UIRect LANDSCAPE_BACK_UI_RECT;
extern UIRect LANDSCAPE_FORWARD_UI_RECT;
extern UIRect PORTRAIT_VISUAL_MARK_PREV;
extern UIRect PORTRAIT_VISUAL_MARK_NEXT;
extern UIRect LANDSCAPE_VISUAL_MARK_PREV;
extern UIRect LANDSCAPE_VISUAL_MARK_NEXT;
extern UIRect PORTRAIT_MIDDLE_LEFT_UI_RECT;
extern UIRect PORTRAIT_MIDDLE_RIGHT_UI_RECT;
extern UIRect LANDSCAPE_MIDDLE_LEFT_UI_RECT;
extern UIRect LANDSCAPE_MIDDLE_RIGHT_UI_RECT;
extern UIRect PORTRAIT_EDIT_PORTAL_UI_RECT;
extern UIRect LANDSCAPE_EDIT_PORTAL_UI_RECT;

extern std::wstring BACK_RECT_TAP_COMMAND;
extern std::wstring BACK_RECT_HOLD_COMMAND;
extern std::wstring FORWARD_RECT_TAP_COMMAND;
extern std::wstring FORWARD_RECT_HOLD_COMMAND;
extern std::wstring EDIT_PORTAL_TAP_COMMAND;
extern std::wstring EDIT_PORTAL_HOLD_COMMAND;
extern std::wstring VISUAL_MARK_NEXT_TAP_COMMAND;
extern std::wstring VISUAL_MARK_NEXT_HOLD_COMMAND;
extern std::wstring VISUAL_MARK_PREV_TAP_COMMAND;
extern std::wstring VISUAL_MARK_PREV_HOLD_COMMAND;
extern std::wstring MIDDLE_LEFT_RECT_TAP_COMMAND;
extern std::wstring MIDDLE_LEFT_RECT_HOLD_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_TAP_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_HOLD_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_HOLD_COMMAND;
extern std::wstring EDIT_PORTAL_TAP_COMMAND;
extern std::wstring EDIT_PORTAL_HOLD_COMMAND;

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

void generate_bezier_with_endpoints_and_velocity(
    Vec<float, 2> p0, 
    Vec<float, 2> p1, 
    Vec<float, 2> v0, 
    Vec<float, 2> v1, 
    int n_points,
    float thickness,
    std::vector<FreehandDrawingPoint>& output
    ) {
    float alpha = 1.0f / n_points;
    auto q0 = p0;
    auto q1 = v0;
    auto q2 = (p1 - p0) * 3 - v0 * 2 - v1;
    auto q3 = (p0 - p1) * 2 + v0 + v1;

    for (int i = 0; i < n_points; i++) {
        float t = alpha * i;
        float tt = t * t;
        float ttt = tt * t;
        Vec<float, 2> point = q0 + q1 *t + q2 * tt + q3 * ttt;
        output.push_back(FreehandDrawingPoint{AbsoluteDocumentPos{point.x(), point.y()}, thickness});
    }

}

bool num_slices_for_page_rect(PagelessDocumentRect page_rect, int* h_slices, int* v_slices) {
    /*
    determines the number of vertical/horizontal slices when rendering
    normally we don't use slicing when SLICED_RENDERING is false, unless
    there is a giant page which would crash the application if we tried rendering
    it as a single page
    returns true if we are using sliced rendering
    */
    if (page_rect.y1 > 2000.0) {
        *v_slices = static_cast<int>(page_rect.y1 / 500.0f);
        if (SLICED_RENDERING) {
            *h_slices = NUM_H_SLICES;
            return true;
        }
        else {
            *h_slices = 1;
            return true;
        }
    }
    else {
        if (SLICED_RENDERING) {
            *h_slices = NUM_H_SLICES;
            *v_slices = NUM_V_SLICES;
            return true;
        }
        else {
            *h_slices = 1;
            *v_slices = 1;
            return false;
        }
    }
}

std::string read_file_contents(const Path& path) {
#ifdef SIOYEK_ANDROID
    std::wstring actual_path = path.get_path();
    QFile qfile(QString::fromStdWString(path.get_path()));
    qfile.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly);
    std::string content = qfile.readAll().toStdString();
    qfile.close();
    return content;
#else
    std::wifstream stream = open_wifstream(path.get_path());
    std::wstring content;

    if (stream.is_open()) {
        std::wstringstream sstr;
        sstr << stream.rdbuf();
        content = sstr.str();
        stream.close();
        return utf8_encode(content);
    }
    else {
        return "";
    }
#endif
}

GLuint PdfViewOpenGLWidget::LoadShaders(Path vertex_file_path, Path fragment_file_path) {
    //const wchar_t* vertex_file_path = vertex_file_path_.c_str();
    //const wchar_t* fragment_file_path = fragment_file_path_.c_str();
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);


#ifdef SIOYEK_ANDROID
    std::string header = "#version 310 es\n";
#else
    std::string header = "#version 330 core\n";
#endif

    std::string vertex_shader_code_utf8 = read_file_contents(vertex_file_path);
    if (vertex_shader_code_utf8.size() == 0) {
        return 0;
    }
    vertex_shader_code_utf8 = header + vertex_shader_code_utf8;

    std::string fragment_shader_code_utf8 = read_file_contents(fragment_file_path);
    if (fragment_shader_code_utf8.size() == 0) {
        return 0;
    }

    fragment_shader_code_utf8 = header + fragment_shader_code_utf8;

    GLint Result = GL_FALSE;
    int InfoLogLength;

    char const* VertexSourcePointer = vertex_shader_code_utf8.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

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

#ifdef SIOYEK_ANDROID
        shared_gl_objects.rendered_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/simple.fragment"));
        shared_gl_objects.rendered_dark_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/dark_mode.fragment"));
        shared_gl_objects.unrendered_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/unrendered_page.fragment"));
        shared_gl_objects.highlight_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/highlight.fragment"));
        shared_gl_objects.vertical_line_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/vertical_bar.fragment"));
        shared_gl_objects.vertical_line_dark_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/vertical_bar_dark.fragment"));
        shared_gl_objects.custom_color_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/custom_colors.fragment"));
        shared_gl_objects.separator_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/separator.fragment"));
        shared_gl_objects.stencil_program = LoadShaders(Path(L":/pdf_viewer/shaders/stencil.vertex"), Path(L":/pdf_viewer/shaders/stencil.fragment"));
        shared_gl_objects.line_program = LoadShaders(Path(L":/pdf_viewer/shaders/line.vertex"), Path(L":/pdf_viewer/shaders/line.fragment"));
        shared_gl_objects.compiled_drawing_program = LoadShaders(Path(L":/pdf_viewer/shaders/compiled_drawing.vertex"), Path(L":/pdf_viewer/shaders/compiled_line.fragment"));
        shared_gl_objects.compiled_dots_program = LoadShaders(Path(L":/pdf_viewer/shaders/dot.vertex"), Path(L":/pdf_viewer/shaders/dot.fragment"));
#else
        shared_gl_objects.rendered_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"simple.fragment"));
        shared_gl_objects.rendered_dark_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"dark_mode.fragment"));
        shared_gl_objects.unrendered_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"unrendered_page.fragment"));
        shared_gl_objects.highlight_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"highlight.fragment"));
        shared_gl_objects.vertical_line_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"vertical_bar.fragment"));
        shared_gl_objects.vertical_line_dark_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"vertical_bar_dark.fragment"));
        shared_gl_objects.custom_color_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"custom_colors.fragment"));
        shared_gl_objects.separator_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"separator.fragment"));
        shared_gl_objects.stencil_program = LoadShaders(shader_path.slash(L"stencil.vertex"), shader_path.slash(L"stencil.fragment"));
        shared_gl_objects.line_program = LoadShaders(shader_path.slash(L"line.vertex"), shader_path.slash(L"line.fragment"));
        shared_gl_objects.compiled_drawing_program = LoadShaders(shader_path.slash(L"compiled_drawing.vertex"), shader_path.slash(L"compiled_line.fragment"));
        shared_gl_objects.compiled_dots_program = LoadShaders(shader_path.slash(L"dot.vertex"), shader_path.slash(L"dot.fragment"));
#endif

        shared_gl_objects.dark_mode_contrast_uniform_location = glGetUniformLocation(shared_gl_objects.rendered_dark_program, "contrast");
        //shared_gl_objects.gamma_uniform_location = glGetUniformLocation(shared_gl_objects.rendered_program, "gamma");

        shared_gl_objects.highlight_color_uniform_location = glGetUniformLocation(shared_gl_objects.highlight_program, "highlight_color");
        shared_gl_objects.highlight_opacity_uniform_location = glGetUniformLocation(shared_gl_objects.highlight_program, "opacity");

        shared_gl_objects.line_color_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "line_color");
        shared_gl_objects.line_time_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "time");

        shared_gl_objects.custom_color_transform_uniform_location = glGetUniformLocation(shared_gl_objects.custom_color_program, "transform_matrix");

        shared_gl_objects.separator_background_color_uniform_location = glGetUniformLocation(shared_gl_objects.separator_program, "background_color");
        shared_gl_objects.freehand_line_color_uniform_location = glGetUniformLocation(shared_gl_objects.line_program, "line_color");

        shared_gl_objects.compiled_drawing_offset_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_drawing_program, "offset");
        shared_gl_objects.compiled_drawing_scale_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_drawing_program, "scale");
        shared_gl_objects.compiled_drawing_colors_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_drawing_program, "type_colors");

        shared_gl_objects.compiled_dot_color_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_dots_program, "type_colors");
        shared_gl_objects.compiled_dot_offset_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_dots_program, "offset");
        shared_gl_objects.compiled_dot_scale_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_dots_program, "scale");

        //{ // test
        //    float vertex_data[] = {
        //        -0.5f, -0.1f,
        //        -0.5f, 0.1f,
        //        0.5f, -0.1f,
        //        0.5f, 0.1f, // end of strip 1
        //        -0.5f, -0.4f,
        //        -0.5f, -0.2f,
        //        0.5f, -0.4f,
        //        0.5f, -0.2f
        //    };

        //    unsigned int index_data[] = {0, 1, 2, 3, 0xFFFFFFFF, 4, 5, 6, 7};

        //    glGenVertexArrays(1, &shared_gl_objects.test_vao);
        //    glGenBuffers(1, &shared_gl_objects.test_vbo);
        //    glGenBuffers(1, &shared_gl_objects.test_ibo);

        //    glBindVertexArray(shared_gl_objects.test_vao);
        //    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.test_vbo);
        //    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
        //    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        //    glEnableVertexAttribArray(0);

        //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shared_gl_objects.test_ibo);
        //    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_data), index_data, GL_STATIC_DRAW);
        //    glBindVertexArray(0);
        //}

        glGenBuffers(1, &shared_gl_objects.vertex_buffer_object);
        glGenBuffers(1, &shared_gl_objects.uv_buffer_object);
        glGenBuffers(1, &shared_gl_objects.line_points_buffer_object);

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

void PdfViewOpenGLWidget::render_line_window(GLuint program, float gl_vertical_pos, std::optional<NormalizedWindowRect> ruler_rect) {


    float bar_height = 4.0f;

    float bar_data[] = {
        -1, gl_vertical_pos,
        1, gl_vertical_pos,
        -1, gl_vertical_pos - bar_height,
        1, gl_vertical_pos - bar_height
    };


    glDisable(GL_CULL_FACE);
    glUseProgram(program);

    //const float* vertical_line_color = config_manager->get_config<float>(L"vertical_line_color");
    std::array<float, 4> vertical_line_color = cc4(DEFAULT_VERTICAL_LINE_COLOR);

    glUniform4fv(shared_gl_objects.line_color_uniform_location,
        1,
        &vertical_line_color[0]);

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
        float gl_vertical_begin_pos = ruler_rect->y0;
        float ruler_left_pos = ruler_rect->x0;
        float ruler_right_pos = ruler_rect->x1;
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
void PdfViewOpenGLWidget::render_highlight_window(GLuint program, NormalizedWindowRect window_rect, int flags, int line_width_in_pixels) {

    if (is_rotated()) {
        return;
    }

    float scale_factor = document_view->get_zoom_level() / document_view->get_view_height();

    float line_width_window = STRIKE_LINE_WIDTH * scale_factor;

    if (line_width_in_pixels > 0){
        line_width_window = static_cast<float>(line_width_in_pixels) / document_view->get_view_height();
    }

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    glDisable(GL_CULL_FACE);

    glUseProgram(program);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    if (flags & HRF_UNDERLINE) {
        float underline_data[] = {
            window_rect.x0, window_rect.y1 + line_width_window,
            window_rect.x1, window_rect.y1 + line_width_window,
            window_rect.x0, window_rect.y1 - line_width_window,
            window_rect.x1, window_rect.y1 - line_width_window
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(underline_data), underline_data, GL_DYNAMIC_DRAW);
    }
    else if (flags & HRF_STRIKE) {
        float strike_data[] = {
            window_rect.x0, (window_rect.y1 + window_rect.y0) / 2 ,
            window_rect.x1, (window_rect.y1 + window_rect.y0) / 2 ,
            window_rect.x0, (window_rect.y1 + window_rect.y0) / 2 - 2 * line_width_window,
            window_rect.x1, (window_rect.y1 + window_rect.y0) / 2 - 2 * line_width_window
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(strike_data), strike_data, GL_DYNAMIC_DRAW);
    }
    else {
        float quad_vertex_data[] = {
            window_rect.x0, window_rect.y1,
            window_rect.x1, window_rect.y1,
            window_rect.x0, window_rect.y0,
            window_rect.x1, window_rect.y0
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
    }


    // no need to draw the fill color if we are in underline/strike mode
    if (flags & HRF_FILL) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    if (flags & HRF_BORDER) {
        float line_data[] = {
            window_rect.x0, window_rect.y0,
            window_rect.x1, window_rect.y0,
            window_rect.x1, window_rect.y1,
            window_rect.x0, window_rect.y1
        };

        glDisable(GL_BLEND);
        glBufferData(GL_ARRAY_BUFFER, sizeof(line_data), line_data, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
    if (flags & HRF_UNDERLINE) {
        glDisable(GL_BLEND);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    if (flags & HRF_STRIKE) {
        glDisable(GL_BLEND);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

void PdfViewOpenGLWidget::render_highlight_absolute(GLuint program, AbsoluteRect absolute_document_rect, int flags) {
    NormalizedWindowRect window_rect;
    if (scratchpad) {
        window_rect = scratchpad->absolute_to_window_rect(absolute_document_rect);
    }
    else {
        window_rect = absolute_document_rect.to_window_normalized(document_view);
    }
    render_highlight_window(program, window_rect, flags);
}

void PdfViewOpenGLWidget::render_highlight_document(GLuint program, DocumentRect doc_rect, int flags) {
    NormalizedWindowRect window_rect = doc_rect.to_window_normalized(document_view);
    render_highlight_window(program, window_rect, flags);
}

void PdfViewOpenGLWidget::render_scratchpad(QPainter* painter) {

    /* bool use_cached_framebuffer = can_use_cached_scratchpad_framebuffer(); */

    painter->beginNativePainting();
    clear_background_color();
    painter->endNativePainting();

    for (auto [pixmap, rect] : scratchpad->pixmaps) {
        WindowRect window_rect = rect.to_window(scratchpad);

        QRect window_qrect = QRect(
            window_rect.x0,
            window_rect.y0,
            window_rect.width(),
            window_rect.height()
        );
        painter->drawPixmap(window_qrect, pixmap);
    }

    for (auto [pixmap, rect] : moving_pixmaps) { // highlight moving pixmaps
        WindowRect window_rect = rect.to_window(scratchpad);
        QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());
        painter->fillRect(window_qrect.adjusted(-2, -2, 2, 2), QColor(255, 255, 0));
        painter->drawPixmap(window_qrect, pixmap);
    }

    /* if (use_cached_framebuffer) { */
    /*     painter->drawImage(rect(), cached_framebuffer.value()); */
    /* } */

    painter->beginNativePainting();

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindVertexArray(vertex_array_object);
    bind_default();


    std::vector<FreehandDrawing> pending_drawing;
    if (current_drawing.points.size() > 1) {
        pending_drawing.push_back(current_drawing);
    }


     //std::vector<FreehandDrawing> debug_darwings;
     //std::vector<FreehandDrawingPoint> debug_points;

     //debug_points.push_back(FreehandDrawingPoint{ AbsoluteDocumentPos{-100, 0}, 1 });
     //debug_points.push_back(FreehandDrawingPoint{ AbsoluteDocumentPos{0, 100}, 1 });
     //debug_points.push_back(FreehandDrawingPoint{ AbsoluteDocumentPos{100, 0}, 1 });
     //debug_points.push_back(FreehandDrawingPoint{ AbsoluteDocumentPos{200, -100}, 1 });
     //debug_points.push_back(FreehandDrawingPoint{ AbsoluteDocumentPos{300, -100}, 1 });
     //debug_points.push_back(FreehandDrawingPoint{ AbsoluteDocumentPos{300, -200}, 1 });

     //FreehandDrawing debug_drawing;
     //debug_drawing.type = 'r';
     //debug_drawing.points = debug_points;
     //debug_darwings.push_back(debug_drawing);
     //debug_darwings.push_back(generate_debug_drawing());
     //render_drawings(scratchpad, debug_darwings);



     if (scratchpad->get_non_compiled_drawings().size() > 50 || scratchpad->is_compile_invalid()) {
         compile_drawings(scratchpad, scratchpad->get_all_drawings());
     }

     glEnable(GL_MULTISAMPLE);

     render_compiled_drawings();
     glEnableVertexAttribArray(0);
     glUseProgram(shared_gl_objects.line_program);
     render_drawings(scratchpad, scratchpad->get_non_compiled_drawings());
     render_drawings(scratchpad, moving_drawings, true);
     render_drawings(scratchpad, moving_drawings, false);
     render_drawings(scratchpad, pending_drawing);

    glDisable(GL_MULTISAMPLE);

    bind_default();
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glUseProgram(shared_gl_objects.highlight_program);

    render_selected_rectangle();

    painter->endNativePainting();

}

//void PdfViewOpenGLWidget::update_framebuffer_cache(){
//    last_cache_num_drawings = scratchpad->drawings.size();
//    cached_framebuffer = grabFramebuffer();
//}

void PdfViewOpenGLWidget::paintGL() {

    QPainter painter(this);

    QColor red_color = QColor::fromRgb(255, 0, 0);
    painter.setPen(red_color);

    if (scratchpad == nullptr) {
        my_render(&painter);
    }
    else {
        render_scratchpad(&painter);
    }

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
#ifdef SIOYEK_ANDROID
    format.setVersion(3, 1);
#else
    format.setVersion(3, 3);
#endif
    //    format.setSwapBehavior(QSurfaceFormat::SwapBehavior::SingleBuffer);
    //    format.setSwapInterval(0);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    this->setFormat(format);

    overview_half_width = OVERVIEW_SIZE[0];
    overview_half_height = OVERVIEW_SIZE[1];

    overview_offset_x = OVERVIEW_OFFSET[0];
    overview_offset_y = OVERVIEW_OFFSET[1];
    for (int i = 0; i < 26; i++) {
        visible_drawing_mask[i] = true;
    }

    //bookmark_pixmap = QPixmap(":/icons/B.svg");
    //portal_pixmap = QPixmap(":/end.png");
    bookmark_icon = QIcon(":/icons/B.svg");
    portal_icon = QIcon(":/icons/P.svg");
    bookmark_icon_white = QIcon(":/icons/B_white.svg");
    portal_icon_white = QIcon(":/icons/P_white.svg");
    hourglass_icon = QIcon(":/icons/hourglass.svg");
}

void PdfViewOpenGLWidget::cancel_search() {
    search_results.clear();
    current_search_result_index = -1;
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
            OverviewState state = { new_offset_y, 0, -1, nullptr };
            set_overview_page(state);
        }
        else {
            document_view->set_offset_y(new_offset_y);
        }
    }
}


void PdfViewOpenGLWidget::render_overview(OverviewState overview) {
    if (!valid_document()) return;

    float view_width = static_cast<int>(document_view->get_view_width() * overview_half_width);
    float view_height = static_cast<int>(document_view->get_view_height() * overview_half_height);

    NormalizedWindowRect window_rect = get_overview_rect_pixel_perfect(
        document_view->get_view_width(),
        document_view->get_view_height(),
        view_width,
        view_height);
    window_rect.y0 = -window_rect.y0;
    window_rect.y1 = -window_rect.y1;

    enable_stencil();
    glClear(GL_STENCIL_BUFFER_BIT);
    write_to_stencil();
    draw_stencil_rects({ window_rect });
    use_stencil_to_write(true);

    int page = AbsoluteDocumentPos{0, overview_page->absolute_offset_y}.to_document(doc(true)).page;

    draw_overview_background();


    render_page(page, true);
    render_page(page-1, true);
    render_page(page+1, true);

    std::optional<SearchResult> highlighted_result = get_current_search_result();
    // highlight the overview search result
    if (highlighted_result) {
        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
        glUseProgram(shared_gl_objects.highlight_program);
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"search_highlight_color"));
        glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);

        for (auto rect : highlighted_result->rects) {
            NormalizedWindowRect target = document_to_overview_rect(DocumentRect{ rect, highlighted_result->page });
            render_highlight_window(shared_gl_objects.highlight_program, target, HRF_FILL | HRF_BORDER);
        }
    }
    if (overview_highlights.size() > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
        glUseProgram(shared_gl_objects.highlight_program);
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"search_highlight_color"));
        //glUniform3fv(g_shared_resources.highlight_color_uniform_location, 1, highlight_color_temp);
        for (auto rect : overview_highlights) {
            NormalizedWindowRect target = document_to_overview_rect(rect);
            render_highlight_window(shared_gl_objects.highlight_program, target, HRF_FILL | HRF_BORDER);
        }
    }

    disable_stencil();
    draw_overview_border();

    return;
}

void PdfViewOpenGLWidget::draw_overview_background(){
    float border_vertices[4 * 2];
    get_overview_window_vertices(border_vertices);

    float bg_color[] = { 1.0f, 1.0f, 1.0f };
    get_background_color(bg_color);
    glDisable(GL_BLEND);

    glUseProgram(shared_gl_objects.highlight_program);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(border_vertices), border_vertices, GL_DYNAMIC_DRAW);
    glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, bg_color);
    glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void PdfViewOpenGLWidget::draw_overview_border(){
    float border_color[3] = {0.5f, 0.5f, 0.5f};
    glUseProgram(shared_gl_objects.highlight_program);
    glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, border_color);
    render_highlight_window(shared_gl_objects.highlight_program, get_overview_rect(), HRF_BORDER);
}

Document* PdfViewOpenGLWidget::doc(bool overview){
    if (overview){
        if (overview_page){
            if (overview_page->doc){
                return overview_page->doc;
            }
        }
    }

    return document_view->get_document();
}

void PdfViewOpenGLWidget::render_page(int page_number, bool in_overview, bool force_light_mode) {

    if (!valid_document()) return;

    int nh, nv;

    float page_width = doc(in_overview)->get_page_width(page_number);
    float page_height = doc(in_overview)->get_page_height(page_number);
    PagelessDocumentRect page_rect({ 0, 0, page_width, page_height });
    if ((page_width < 0) || (page_height < 0)) return;

    float zoom_level = in_overview ? get_overview_zoom_level() : document_view->get_zoom_level();

    bool is_sliced = num_slices_for_page_rect(page_rect, &nh, &nv);

    for (int i = 0; i < nh * nv; i++) {
        int v_index = i / nh;
        int h_index = i % nh;

        int rendered_width = -1;
        int rendered_height = -1;

        int index = i;

        if (!is_sliced) {
            index = -1;
        }

        // todo: just replace this with page_width and page_height from above
        float slice_document_width = doc(in_overview)->get_page_width(page_number);
        float slice_document_height = doc(in_overview)->get_page_height(page_number);
        PagelessDocumentRect slice_document_rect;
        slice_document_rect.x0 = 0;
        slice_document_rect.x1 = slice_document_width;
        slice_document_rect.y0 = 0;
        slice_document_rect.y1 = slice_document_height;
        slice_document_rect = get_index_rect(slice_document_rect, i, nh, nv);

        NormalizedWindowRect slice_window_rect = in_overview ? 
            document_to_overview_rect(DocumentRect(slice_document_rect, page_number)) :
            DocumentRect(slice_document_rect, page_number).to_window_normalized(document_view);

        // we add some slack so we pre-render nearby slices
        NormalizedWindowRect full_window_rect;
        full_window_rect.x0 = -1;
        full_window_rect.x1 = 1;
        full_window_rect.y0 = -1.5f;
        full_window_rect.y1 = 1.5f;

        // don't render invisible slices
        if (is_sliced && (!rects_intersect(slice_window_rect, full_window_rect))) {
            continue;
        }

        GLuint texture = pdf_renderer->find_rendered_page(doc(in_overview)->get_path(),
            page_number,
            doc(in_overview)->should_render_pdf_annotations(),
            index,
            nh,
            nv,
            zoom_level,
            devicePixelRatioF(),
            &rendered_width,
            &rendered_height);


        // when rotating, we swap nv and nh 
        int nh_ = nh;
        int nv_ = nv;

        if (rotation_index % 2 == 1) {
            std::swap(rendered_width, rendered_height);
            std::swap(nh_, nv_);
        }

        float page_vertices[4 * 2];
        float slice_height = doc(in_overview)->get_page_height(page_number) / nv_;
        float slice_width = doc(in_overview)->get_page_width(page_number) / nh_;

        PagelessDocumentRect page_rect;
        PagelessDocumentRect full_page_rect({ 0,
                0,
                 doc(in_overview)->get_page_width(page_number),
                 doc(in_overview)->get_page_height(page_number)
        });

        WindowRect full_page_irect = fz_round_rect(fz_transform_rect(full_page_rect,
            fz_scale(zoom_level, zoom_level)));

        if (is_sliced) {

            if (rotation_index == 1) {
                std::swap(h_index, v_index);
                h_index = nv - h_index - 1;
            }
            else if (rotation_index == 2) {
                v_index = nv - v_index - 1;
            }
            else if (rotation_index == 3) {
                std::swap(h_index, v_index);
            }


            page_rect = { h_index * slice_width,
                v_index * slice_height,
                (h_index + 1) * slice_width,
                (v_index + 1) * slice_height
            };
            WindowRect page_irect;
            page_irect.x0 = ((full_page_irect.x1 - full_page_irect.x0) / nh_) * h_index;
            page_irect.x1 = ((full_page_irect.x1 - full_page_irect.x0) / nh_) * (h_index + 1);
            if (h_index == (nh_ - 1)) {
                page_irect.x1 = full_page_irect.x1;
            }

            page_irect.y0 = ((full_page_irect.y1 - full_page_irect.y0) / nv_) * v_index;
            page_irect.y1 = ((full_page_irect.y1 - full_page_irect.y0) / nv_) * (v_index + 1);
            if (v_index == (nv_ - 1)) {
                //page_irect.y0 += 1;
                page_irect.y1 = full_page_irect.y1;
            }

            //fz_irect page_irect = fz_round_rect(fz_transform_rect(page_rect,
            //	fz_scale(document_view->get_zoom_level(), document_view->get_zoom_level())));
            //if (v_index > 0) {
            //	page_irect.y0 += 1;
            //}

            float w = full_page_rect.x1 - full_page_rect.x0;
            float h = full_page_rect.y1 - full_page_rect.y0;

            page_rect.x0 = static_cast<float>(page_irect.x0) / static_cast<float>(full_page_irect.x1 - full_page_irect.x0) * w;
            page_rect.x1 = static_cast<float>(page_irect.x1) / static_cast<float>(full_page_irect.x1 - full_page_irect.x0) * w;
            page_rect.y0 = static_cast<float>(page_irect.y0) / static_cast<float>(full_page_irect.y1 - full_page_irect.y0) * h;
            page_rect.y1 = static_cast<float>(page_irect.y1) / static_cast<float>(full_page_irect.y1 - full_page_irect.y0) * h;
        }
        else {

            page_rect = full_page_rect;
        }


#ifdef SIOYEK_QT6
        float device_pixel_ratio = static_cast<float>(devicePixelRatio());
#else
        float device_pixel_ratio = QApplication::desktop()->devicePixelRatioF();
#endif

        if (DISPLAY_RESOLUTION_SCALE > 0) {
            device_pixel_ratio *= DISPLAY_RESOLUTION_SCALE;
        }

        bool is_not_exact = true;
        if ((full_page_irect.y1 - full_page_irect.y0) % nv == 0) {
            is_not_exact = false;
        }

        NormalizedWindowRect window_rect = in_overview ? 
            document_to_overview_rect(DocumentRect(page_rect, page_number)) :
            document_view->document_to_window_rect_pixel_perfect(DocumentRect(page_rect, page_number),
            static_cast<int>(rendered_width / device_pixel_ratio),
            static_cast<int>(rendered_height / device_pixel_ratio), is_sliced);

        rect_to_quad(window_rect, page_vertices);

        if (texture != 0) {
            bind_program(force_light_mode);
            glBindTexture(GL_TEXTURE_2D, texture);
        }
        else {
            if (!SHOULD_DRAW_UNRENDERED_PAGES) {
                continue;
            }
            float white[3] = {1, 1, 1};
            std::array<float, 3> bgcolor = cc3(white);
            glUseProgram(shared_gl_objects.highlight_program);
            glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &bgcolor[0]);
        }

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), rotation_uvs[rotation_index], GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if ((get_current_color_mode() != Normal) && (PRESERVE_IMAGE_COLORS) && (!in_overview) && (!force_light_mode)) {
            // render images in light mode
            fz_stext_page * stext_page = document_view->get_document()->get_stext_with_page_number(page_number);
            std::vector<PagelessDocumentRect> image_rects;
            for (fz_stext_block* blk = stext_page->first_block; blk != nullptr; blk = blk->next) {
                if (blk->type == FZ_STEXT_BLOCK_IMAGE) {
                        float im_x = blk->u.i.transform.e;
                        float im_y = blk->u.i.transform.f;
                        float im_w = blk->u.i.transform.a;
                        float im_h = blk->u.i.transform.d;
                        PagelessDocumentRect image_rect;
                        image_rect.x0 = im_x;
                        image_rect.x1 = im_x + im_w;
                        image_rect.y0 = im_y;
                        image_rect.y1 = im_y + im_h;
                        image_rects.push_back(image_rect);
                }
            }

            enable_stencil();
            write_to_stencil();
            draw_stencil_rects(page_number, image_rects);
            use_stencil_to_write(true);
            render_page(page_number, in_overview, true);
            disable_stencil();
        }

        if (!document_view->is_presentation_mode() && (!in_overview)){

            // render page separator
            glUseProgram(shared_gl_objects.separator_program);

            PagelessDocumentRect separator_rect({
                0,
                doc(in_overview)->get_page_height(page_number) - PAGE_SEPARATOR_WIDTH / 2,
                doc(in_overview)->get_page_width(page_number),
                doc(in_overview)->get_page_height(page_number) + PAGE_SEPARATOR_WIDTH / 2
                });


            if (PAGE_SEPARATOR_WIDTH > 0) {

                NormalizedWindowRect separator_window_rect = DocumentRect(separator_rect, page_number).to_window_normalized(document_view);
                rect_to_quad(separator_window_rect, page_vertices);

                glUniform3fv(shared_gl_objects.separator_background_color_uniform_location, 1, PAGE_SEPARATOR_COLOR);

                glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
                glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
    }

}

void PdfViewOpenGLWidget::my_render(QPainter* painter) {

    //if (true) {
    //    painter->beginNativePainting();
    //    float offset[] = {0.0f, 0.0f};
    //    float scale[] = { 1.0f, 1.0f };
    //    float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };

    //    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    //    glClear(GL_COLOR_BUFFER_BIT);

    //    glDisable(GL_CULL_FACE);
    //    glDisable(GL_BLEND);
    //    glBindVertexArray(shared_gl_objects.test_vao);

    //    glUseProgram(shared_gl_objects.compiled_drawing_program);

    //    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.test_vbo);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shared_gl_objects.test_ibo);
    //    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    //    glUniform2fv(shared_gl_objects.compiled_drawing_offset_uniform_location, 1, offset);
    //    glUniform2fv(shared_gl_objects.compiled_drawing_scale_uniform_location, 1, scale);
    //    glUniform4fv(shared_gl_objects.compiled_drawing_color_uniform_location, 1, color);
    //    glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    //    glEnableVertexAttribArray(0);
    //    glDrawElements(GL_TRIANGLE_STRIP, 9, GL_UNSIGNED_INT, 0);
    //    bind_default();
    //    return;
    //}

    painter->beginNativePainting();
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindVertexArray(vertex_array_object);
    bind_default();

    if (!valid_document()) {

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if (is_helper) {
            //painter->endNativePainting();
            draw_empty_helper_message(painter, "No portals yet");
        }
        else {
            draw_empty_helper_message(painter, "No document");
        }
        return;
    }

    std::vector<int> visible_pages;
    document_view->get_visible_pages(document_view->get_view_height(), visible_pages);

    clear_background_color();

    std::vector<PdfLink> all_visible_links;

    if (document_view->is_presentation_mode()) {
        int presentation_page_number = document_view->get_presentation_page_number().value();
        if (PRERENDER_NEXT_PAGE) {
            // request the next page so it is scheduled for rendering in the background thread

            for (int i = 0; i < NUM_PRERENDERED_NEXT_SLIDES; i++) {

                if ((presentation_page_number + i + 1) < doc()->num_pages()) {
                    pdf_renderer->find_rendered_page(document_view->get_document()->get_path(),
                        presentation_page_number + i + 1,
                        document_view->get_document()->should_render_pdf_annotations(),
                        -1,
                        1,
                        1,
                        document_view->get_zoom_level(),
                        devicePixelRatioF(),
                        nullptr,
                        nullptr);
                }
            }
            for (int i = 0; i < NUM_PRERENDERED_PREV_SLIDES; i++) {

                if ((presentation_page_number - i - 1) >= 0) {
                    pdf_renderer->find_rendered_page(document_view->get_document()->get_path(),
                        presentation_page_number - i - 1,
                        document_view->get_document()->should_render_pdf_annotations(),
                        -1,
                        1,
                        1,
                        document_view->get_zoom_level(),
                        devicePixelRatioF(),
                        nullptr,
                        nullptr);
                }
            }
        }
        render_page(presentation_page_number);
    }
    else {
        std::array<float, 3> link_highlight_color = cc3(DEFAULT_LINK_HIGHLIGHT_COLOR);

        for (int page : visible_pages) {
            render_page(page);

            if (should_highlight_links) {
                glUseProgram(shared_gl_objects.highlight_program);
                glUniform3fv(shared_gl_objects.highlight_color_uniform_location,
                    1,
                    &link_highlight_color[0]);
                glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
                //fz_link* links = document_view->get_document()->get_page_links(page);
                const std::vector<PdfLink>& links = document_view->get_document()->get_page_merged_pdf_links(page);
                for (auto link : links) {
                    for (auto link_rect : link.rects) {
                        render_highlight_document(shared_gl_objects.highlight_program, { link_rect, page });
                        //all_visible_links.push_back(link);
                    }
                }
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

                    float page_width = document_view->get_document()->get_page_width(max_page + i);
                    float page_height = document_view->get_document()->get_page_width(max_page + i);
                    PagelessDocumentRect page_rect({ 0, 0, page_width, page_height });
                    int nh, nv;
                    num_slices_for_page_rect(page_rect, &nh, &nv);

                    pdf_renderer->find_rendered_page(document_view->get_document()->get_path(),
                        document_view->get_document()->should_render_pdf_annotations(),
                        max_page + i,
                        0,
                        nh,
                        nv,
                        document_view->get_zoom_level(),
                        devicePixelRatioF(),
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
            draw_stencil_rects(document_view->get_center_page_number(), rects);
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
        glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);

        int page = last_selected_block_page.value();
        PagelessDocumentRect rect = last_selected_block.value();
        render_highlight_document(shared_gl_objects.highlight_program, DocumentRect { rect, page });
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

        std::array<float, 3> unselected_search_highlight_color = cc3(UNSELECTED_SEARCH_HIGHLIGHT_COLOR);

        if (SHOULD_HIGHLIGHT_UNSELECTED_SEARCH) {

            std::vector<int> visible_search_indices = get_visible_search_results(visible_pages);
            glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &unselected_search_highlight_color[0]);
            glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
            for (int visible_search_index : visible_search_indices) {
                if (visible_search_index != current_search_result_index) {
                    SearchResult res = search_results[visible_search_index];
                    for (auto rect : res.rects) {
                        render_highlight_document(shared_gl_objects.highlight_program, DocumentRect { rect, res.page });
                    }
                }
            }
        }

        std::array<float, 3> search_highlight_color = cc3(DEFAULT_SEARCH_HIGHLIGHT_COLOR);
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &search_highlight_color[0]);
        glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
        for (auto rect : current_search_result.rects) {
            render_highlight_document(shared_gl_objects.highlight_program, DocumentRect { rect, current_search_result.page });
        }
    }
    search_results_mutex.unlock();

    glUseProgram(shared_gl_objects.highlight_program);

    if (should_show_synxtex_highlights()) {

        std::array<float, 3> synctex_highlight_color = cc3(DEFAULT_SYNCTEX_HIGHLIGHT_COLOR);
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &synctex_highlight_color[0]);
        glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
        for (auto synctex_hl_rect : synctex_highlights) {
            render_highlight_document(shared_gl_objects.highlight_program, synctex_hl_rect, HRF_FILL);
        }
    }



    if (document_view->should_show_text_selection_marker) {
        std::optional<AbsoluteRect> control_character_rect = document_view->get_control_rect();
        if (control_character_rect) {
            float rectangle_color[] = { 0.0f, 1.0f, 1.0f };
            glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, rectangle_color);
            glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
            render_highlight_absolute(shared_gl_objects.highlight_program, control_character_rect.value(), HRF_FILL | HRF_BORDER);
        }
    }

    if (character_highlight_rect) {
        float rectangle_color[] = { 0.0f, 1.0f, 1.0f };
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, rectangle_color);
        glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
        render_highlight_absolute(shared_gl_objects.highlight_program, character_highlight_rect.value(), HRF_FILL | HRF_BORDER);

        if (wrong_character_rect) {
            float wrong_color[] = { 1.0f, 0.0f, 0.0f };
            glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, wrong_color);
            glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
            render_highlight_absolute(shared_gl_objects.highlight_program, wrong_character_rect.value(), HRF_FILL | HRF_BORDER);
        }
    }
    
    render_selected_rectangle();

    if (document_view->is_ruler_mode()) {
        //render_line_window(shared_gl_objects.vertical_line_program ,vertical_line_location);

        float vertical_line_end = document_view->get_ruler_window_y();
        /* std::optional<NormalizedWindowRect> ruler_rect = document_view->get_ruler_window_rect(); */
        // NormalizedWindowRect DocumentView::document_to_window_rect_pixel_perfect(DocumentRect doc_rect, int pixel_width, int pixel_height, bool banded) {
        std::optional<NormalizedWindowRect> ruler_rect = {};

        if (document_view->get_ruler_rect()){
            DocumentRect ruler_document_rect = document_view->get_ruler_rect()->to_document(doc());
            int ruler_pixel_width = static_cast<int>(ruler_document_rect.rect.width() * document_view->get_zoom_level());
            int ruler_pixel_height = static_cast<int>(ruler_document_rect.rect.height() * document_view->get_zoom_level());
            ruler_rect = document_view->document_to_window_rect_pixel_perfect(ruler_document_rect, ruler_pixel_width, ruler_pixel_height, false);
        }

        if ((!ruler_rect.has_value()) || (RULER_DISPLAY_MODE == L"slit")) {
            render_line_window(shared_gl_objects.vertical_line_program, vertical_line_end, ruler_rect);
        }
        else {
            int flags = 0;

            if (RULER_DISPLAY_MODE == L"underline") {
                flags |= HRF_UNDERLINE;
            }

            else if (RULER_DISPLAY_MODE == L"box") {
                flags |= HRF_BORDER;
            }


            glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, RULER_COLOR);
            glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
            render_highlight_window(shared_gl_objects.highlight_program, ruler_rect.value(), flags, RULER_UNDERLINE_PIXEL_WIDTH);
        }
        if (underline) {
            glUseProgram(shared_gl_objects.highlight_program);
            glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, RULER_MARKER_COLOR);
            glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 1.0f);

            AbsoluteRect underline_rect;
            underline_rect.x0 = underline->x - 3.0f;
            underline_rect.x1 = underline->x + 3.0f;

            underline_rect.y0 = underline->y - 1.0f;
            underline_rect.y1 = underline->y + 1.0f;
            NormalizedWindowRect underline_window_rect = underline_rect.to_window_normalized(document_view);
            float mid_y = ruler_rect->y1;
            /* float underline_height = underline_window_rect.width() / 2.0f; */
            /* float underline_height = ruler_rect->height(); */
            float underline_height = 4 * static_cast<float>(RULER_UNDERLINE_PIXEL_WIDTH) / static_cast<float>(document_view->get_view_height());
            underline_window_rect.y0 = mid_y - underline_height / 2;
            underline_window_rect.y1 = mid_y + underline_height / 2;


            render_highlight_window(shared_gl_objects.highlight_program, underline_window_rect, HRF_FILL);
        }
    }
    for (auto [type, marked_data_of_type] : get_marked_data_rect_map()) {

        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &HIGHLIGHT_COLORS[type * 3]);
        glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
        for (auto marked_data : marked_data_of_type) {
            render_highlight_document(shared_gl_objects.highlight_program, marked_data.rect);
        }
    }



    {
        glUseProgram(shared_gl_objects.line_program);
        glEnableVertexAttribArray(0);
        std::vector<FreehandDrawing> pending_drawing;
        if (current_drawing.points.size() > 1) {
            pending_drawing.push_back(current_drawing);
        }
        glEnable(GL_MULTISAMPLE);
        for (auto page : visible_pages) {

            render_drawings(document_view, document_view->get_document()->get_page_drawings(page));
        }
        render_drawings(document_view, moving_drawings, true);
        render_drawings(document_view, moving_drawings, false);
        render_drawings(document_view, pending_drawing);
        glDisable(GL_MULTISAMPLE);
        bind_default();
    }

    painter->endNativePainting();


    if (document_view->get_document()->can_use_highlights()) {
        const std::vector<BookMark>& bookmarks = document_view->get_document()->get_bookmarks();
        const std::vector<Portal>& portals = document_view->get_document()->get_portals();

        for (int i = 0; i < pending_download_portals.size(); i++) {
            render_portal_rect(painter, pending_download_portals[i], true);
        }
        for (int i = 0; i < portals.size(); i++) {
            if (portals[i].is_visible()) {
                render_portal_rect(painter, portals[i].get_rectangle(), false);
            }
        }

        for (int i = 0; i < bookmarks.size(); i++) {
            if (bookmarks[i].begin_y > -1) {
                if (bookmarks[i].end_x == -1) {

                    NormalizedWindowPos bookmark_window_pos = document_view->absolute_to_window_pos(
                        { bookmarks[i].begin_x, bookmarks[i].begin_y }
                    );

                    if (bookmark_window_pos.x > -1.5f && bookmark_window_pos.x < 1.5f &&
                        bookmark_window_pos.y > -1.5f && bookmark_window_pos.y < 1.5f) {

                        NormalizedWindowRect bookmark_normalized_window_rect = bookmarks[i].get_rectangle().to_window_normalized(document_view);

                        fz_irect window_rect = document_view->normalized_to_window_rect(bookmark_normalized_window_rect);
                        QRect window_qrect = QRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));

                        render_ui_icon_for_current_color_mode(painter, bookmark_icon, bookmark_icon_white, window_qrect);

                    }
                }
                else {
                    WindowRect window_rect = bookmarks[i].get_rectangle().to_window(document_view);
                    QRect window_qrect = QRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));

                    QFont font = painter->font();
                    float font_size = bookmarks[i].font_size == -1 ? FREETEXT_BOOKMARK_FONT_SIZE : bookmarks[i].font_size;
                    font.setPointSizeF(font_size * document_view->get_zoom_level() * 0.75);
                    painter->setFont(font);

                    std::array<float, 3> bookmark_color = cc3(bookmarks[i].color);
                    painter->setPen(convert_float3_to_qcolor(&bookmark_color[0]));
                    if (RENDER_FREETEXT_BORDERS) {
                        painter->drawRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                    }

                    int flags = Qt::TextWordWrap;
                    if (is_text_rtl(bookmarks[i].description)) {
                        flags |= Qt::AlignRight;
                    }
                    else {
                        flags |= Qt::AlignLeft;
                    }

                    if ((bookmarks[i].description[0] == L'#') && (bookmarks[i].description[1] != L' ' &&
                                bookmarks[i].description[2] == L' ' )) {
                        char mode = bookmarks[i].description[1];
                        if (mode >= 'a' && mode <= 'z') {
                            std::array<float, 3> box_color = cc3( & HIGHLIGHT_COLORS[3 * (mode - 'a')]);
                            painter->setPen(convert_float3_to_qcolor(&box_color[0]));
                            painter->drawRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                        }
                    }
                    else if (bookmarks[i].description[0] == L'#') {
                        painter->drawRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                    }
                    else if (bookmarks[i].description[0] == L'#') {
                        painter->drawRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                    }
                    else {
                        painter->drawText(window_qrect, flags, QString::fromStdWString(bookmarks[i].description));
                    }

                }

            }
        }
    }

    if (should_highlight_words && (!overview_page)) {
        setup_text_painter(painter);

        std::vector<std::string> tags = get_tags(word_rects.size());

        for (size_t i = 0; i < word_rects.size(); i++) {
            //auto [rect, page] = word_rects[i];
            DocumentRect current_word_rect = word_rects[i];


            NormalizedWindowRect window_rect = current_word_rect.to_window_normalized(document_view);

            int view_width = static_cast<float>(document_view->get_view_width());
            int view_height = static_cast<float>(document_view->get_view_height());

            int window_x0 = static_cast<int>(window_rect.x0 * view_width / 2 + view_width / 2);
            int window_y0 = static_cast<int>(-window_rect.y0 * view_height / 2 + view_height / 2);

            if (i > 0) {
                if (std::abs(word_rects[i - 1].rect.x0 - current_word_rect.rect.x0) < 5) {
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

            //auto [page, link] = all_visible_links[i];
            auto link = all_visible_links[i];

            bool should_draw = true;

            // some malformed doucments have multiple overlapping links which makes reading
            // the link labels difficult. Here we only draw the link text if there are no
            // other close links. This has quadratic runtime but it should not matter since
            // there are not many links in a single PDF page.
            if (HIDE_OVERLAPPING_LINK_LABELS) {
                for (int j = i + 1; j < all_visible_links.size(); j++) {
                    auto other_link = all_visible_links[j];
                    float distance = std::abs(other_link.rects[0].x0 - link.rects[0].x0) + std::abs(other_link.rects[0].y0 - link.rects[0].y0);
                    if (distance < 10) {
                        should_draw = false;
                    }
                }
            }

            NormalizedWindowRect window_rect = DocumentRect(link.rects[0], link.source_page).to_window_normalized(document_view);

            int view_width = static_cast<float>(document_view->get_view_width());
            int view_height = static_cast<float>(document_view->get_view_height());

            int window_x = static_cast<int>(window_rect.x0 * view_width / 2 + view_width / 2);
            int window_y = static_cast<int>(-window_rect.y0 * view_height / 2 + view_height / 2);

            if (tag_prefix.size() > 0) {
                if (index_string.find(tag_prefix) != 0) {
                    should_draw = false;
                }
                else {
                    index_string = index_string.substr(tag_prefix.size());
                }
            }
            if (should_draw) {
                painter->drawText(window_x, window_y, index_string.c_str());
            }
        }
    }

    if (should_show_rect_hints) {
        std::vector<std::pair<QRect, QString>> hints = get_hint_rect_and_texts();
        int flags = Qt::TextWordWrap | Qt::AlignCenter;

        for (auto [hint_rect, hint_text] : hints) {
            painter->fillRect(hint_rect, QColor(0, 0, 0, 200));
        }

        painter->setPen(QColor(255, 255, 255, 255));

        for (auto [hint_rect, hint_text] : hints) {
            painter->drawText(hint_rect, flags, hint_text);
        }
    }


    painter->beginNativePainting();
    glBindVertexArray(vertex_array_object);
    bind_default();
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glUseProgram(shared_gl_objects.highlight_program);

    render_text_highlights();
    render_highlight_annotations();

    if (overview_page) {
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        /* glBindVertexArray(vertex_array_object); */

        /* bind_default(); */
        render_overview(overview_page.value());
    }
    painter->endNativePainting();

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

void PdfViewOpenGLWidget::search_text(const std::wstring& text, SearchCaseSensitivity case_sensitive, bool regex, std::optional<std::pair<int, int>> range) {

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
                regex,
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

void PdfViewOpenGLWidget::set_synctex_highlights(std::vector<DocumentRect> highlights) {
    synctex_highlight_time = QTime::currentTime();
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

void PdfViewOpenGLWidget::draw_empty_helper_message(QPainter* painter, QString message) {
    // should be called with native painting disabled

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


NormalizedWindowRect PdfViewOpenGLWidget::get_overview_rect() {
    NormalizedWindowRect res;
    res.x0 = overview_offset_x - overview_half_width;
    res.y0 = overview_offset_y - overview_half_height;
    res.x1 = overview_offset_x + overview_half_width;
    res.y1 = overview_offset_y + overview_half_height;
    return res;
}

NormalizedWindowRect	PdfViewOpenGLWidget::get_overview_rect_pixel_perfect(int widget_width, int widget_height, int view_width, int view_height) {
    NormalizedWindowRect res;
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

std::vector<NormalizedWindowRect> PdfViewOpenGLWidget::get_overview_border_rects() {
    std::vector<NormalizedWindowRect> res;

    NormalizedWindowRect bottom_rect;
    NormalizedWindowRect top_rect;
    NormalizedWindowRect left_rect;
    NormalizedWindowRect right_rect;

    bottom_rect.x0 = overview_offset_x - overview_half_width;
    bottom_rect.y0 = overview_offset_y - overview_half_height - 0.05f;
    bottom_rect.x1 = overview_offset_x + overview_half_width;
    bottom_rect.y1 = overview_offset_y - overview_half_height + 0.05f;

    top_rect.x0 = overview_offset_x - overview_half_width;
    top_rect.y0 = overview_offset_y + overview_half_height - 0.05f;
    top_rect.x1 = overview_offset_x + overview_half_width;
    top_rect.y1 = overview_offset_y + overview_half_height + 0.05f;

    left_rect.x0 = overview_offset_x - overview_half_width - 0.05f;
    left_rect.y0 = overview_offset_y - overview_half_height;
    left_rect.x1 = overview_offset_x - overview_half_width + 0.05f;
    left_rect.y1 = overview_offset_y + overview_half_height;

    right_rect.x0 = overview_offset_x + overview_half_width - 0.05f;
    right_rect.y0 = overview_offset_y - overview_half_height;
    right_rect.x1 = overview_offset_x + overview_half_width + 0.05f;
    right_rect.y1 = overview_offset_y + overview_half_height;

    res.push_back(bottom_rect);
    res.push_back(top_rect);
    res.push_back(left_rect);
    res.push_back(right_rect);

    return res;
}

bool PdfViewOpenGLWidget::is_window_point_in_overview(NormalizedWindowPos window_point) {
    if (get_overview_page()) {
        NormalizedWindowRect rect = get_overview_rect();
        return rect.contains(window_point);
    }
    return false;
}

bool PdfViewOpenGLWidget::is_window_point_in_overview_border(NormalizedWindowPos window_point, OverviewSide* which_border) {

    if (!get_overview_page().has_value()) {
        return false;
    }

    std::vector<NormalizedWindowRect> rects = get_overview_border_rects();
    for (size_t i = 0; i < rects.size(); i++) {
        if (rects[i].contains(window_point)) {
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

void PdfViewOpenGLWidget::set_overview_side_pos(OverviewSide index, NormalizedWindowRect original_rect, fvec2 diff) {

    NormalizedWindowRect new_rect = original_rect;

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

void PdfViewOpenGLWidget::set_overview_rect(NormalizedWindowRect rect) {
    float halfwidth = rect.width() / 2;
    float halfheight = rect.height() / 2;
    float offset_x = rect.x0 + halfwidth;
    float offset_y = rect.y0 + halfheight;

    overview_offset_x = offset_x;
    overview_offset_y = offset_y;
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

void PdfViewOpenGLWidget::bind_program(bool force_light) {
    if ((!force_light) && (color_mode == ColorPalette::Dark)) {
        glUseProgram(shared_gl_objects.rendered_dark_program);
        glUniform1f(shared_gl_objects.dark_mode_contrast_uniform_location, DARK_MODE_CONTRAST);
    }
    else if ((!force_light) && (color_mode == ColorPalette::Custom)) {
        glUseProgram(shared_gl_objects.custom_color_program);
        float transform_matrix[16];
        get_custom_color_transform_matrix(transform_matrix);
        glUniformMatrix4fv(shared_gl_objects.custom_color_transform_uniform_location, 1, GL_TRUE, transform_matrix);

    }
    else {
        glUseProgram(shared_gl_objects.rendered_program);
        //glUniform1f(shared_gl_objects.gamma_uniform_location, GAMMA);
    }
}

DocumentPos PdfViewOpenGLWidget::window_pos_to_overview_pos(NormalizedWindowPos window_pos) {
    Document* target = doc(true);

    float window_width = static_cast<float>(size().width());
    float window_height = static_cast<float>(size().height());
    int window_x = static_cast<int>((1.0f + window_pos.x) / 2 * window_width);
    int window_y = static_cast<int>((1.0f - window_pos.y) / 2 * window_height);
    DocumentPos docpos = target->absolute_to_page_pos_uncentered(
        { get_overview_page()->absolute_offset_x, get_overview_page()->absolute_offset_y});

    float zoom_level = get_overview_zoom_level();

    int overview_h_mid = overview_offset_x * window_width / 2 + window_width / 2;
    int overview_mid = (-overview_offset_y) * window_height / 2 + window_height / 2;

    float relative_window_x = static_cast<float>(window_x - overview_h_mid) / zoom_level;
    float relative_window_y = static_cast<float>(window_y - overview_mid) / zoom_level;

    int page_half_width = target->get_page_width(docpos.page);
    float doc_offset_x = -docpos.x + relative_window_x + page_half_width;
    float doc_offset_y = docpos.y + relative_window_y;
    int doc_page = docpos.page;
    return { doc_page, doc_offset_x, doc_offset_y };
}

void PdfViewOpenGLWidget::toggle_highlight_words() {
    this->should_highlight_words = !this->should_highlight_words;
}

void PdfViewOpenGLWidget::set_highlight_words(std::vector<DocumentRect>& rects) {
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

    float background_color[4] = { 1.0f, 1.0f, 1.0f, 1 - FASTREAD_OPACITY };

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

void PdfViewOpenGLWidget::draw_stencil_rects(const std::vector<NormalizedWindowRect>& rects) {
    std::vector<float> window_rects;

    for (auto rect : rects) {

        float triangle1[6] = {
            rect.x0, rect.y0,
            rect.x0, rect.y1,
            rect.x1, rect.y0
        };
        float triangle2[6] = {
            rect.x1, rect.y0,
            rect.x0, rect.y1,
            rect.x1, rect.y1
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

void PdfViewOpenGLWidget::draw_stencil_rects(int page, const std::vector<PagelessDocumentRect>& rects) {
    std::vector<NormalizedWindowRect> normalized_rects;
    for (auto rect : rects) {
        normalized_rects.push_back(DocumentRect(rect, page).to_window_normalized(document_view));
    }
    draw_stencil_rects(normalized_rects);
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

void PdfViewOpenGLWidget::get_overview_window_vertices(float out_vertices[2 * 4]) {

    out_vertices[0] = overview_offset_x - overview_half_width;
    out_vertices[1] = overview_offset_y + overview_half_height;
    out_vertices[2] = overview_offset_x - overview_half_width;
    out_vertices[3] = overview_offset_y - overview_half_height;
    out_vertices[4] = overview_offset_x + overview_half_width;
    out_vertices[5] = overview_offset_y - overview_half_height;
    out_vertices[6] = overview_offset_x + overview_half_width;
    out_vertices[7] = overview_offset_y + overview_half_height;
}

void PdfViewOpenGLWidget::set_selected_rectangle(AbsoluteRect selected) {
    selected_rectangle = selected;
}

void PdfViewOpenGLWidget::clear_selected_rectangle() {
    selected_rectangle = {};
}
std::optional<AbsoluteRect> PdfViewOpenGLWidget::get_selected_rectangle() {
    return selected_rectangle;
}

void PdfViewOpenGLWidget::set_typing_rect(DocumentRect highlight_rect, std::optional<DocumentRect> wrong_rect) {
    AbsoluteRect absrect = document_view->get_document()->document_to_absolute_rect(highlight_rect);
    character_highlight_rect = absrect;

    if (wrong_rect) {
        AbsoluteRect abswrong = document_view->get_document()->document_to_absolute_rect(wrong_rect.value());
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

float PdfViewOpenGLWidget::get_overview_zoom_level(){

    if (overview_page->zoom_level > 0){
        return overview_page->zoom_level;
    }
    DocumentPos docpos = doc(true)->absolute_to_page_pos_uncentered({ 0, overview_page->absolute_offset_y });
    overview_page->zoom_level = (document_view->get_view_width() * overview_half_width) / doc(true)->get_page_width(docpos.page);
    return overview_page->zoom_level;

}

NormalizedWindowPos PdfViewOpenGLWidget::document_to_overview_pos(DocumentPos pos) {
    NormalizedWindowPos res;

    if (overview_page) {
        OverviewState overview = overview_page.value();
        Document* target_doc = get_current_overview_document();
        /* DocumentPos docpos = target_doc->absolute_to_page_pos_uncentered({ 0, overview.absolute_offset_y }); */

        AbsoluteDocumentPos abspos = target_doc->document_to_absolute_pos(pos);

        float overview_zoom_level = get_overview_zoom_level() / document_view->get_view_width() * 2;

        float relative_x = abspos.x * overview_zoom_level + overview.absolute_offset_x * overview_zoom_level;
        float aspect = static_cast<float>(width()) / static_cast<float>(height());
        float relative_y = (abspos.y - overview.absolute_offset_y) * overview_zoom_level * aspect;
        //float left = overview_offset_x - overview_half_width;
        float top = overview_offset_y;
        return { overview_offset_x + relative_x, top - relative_y };

        return res;
    }
    else {
        return res;
    }
}

NormalizedWindowRect PdfViewOpenGLWidget::document_to_overview_rect(DocumentRect doc_rect) {
    DocumentPos top_left = doc_rect.top_left();
    DocumentPos bottom_right = doc_rect.bottom_right();
    NormalizedWindowPos top_left_pos = document_to_overview_pos(top_left);
    NormalizedWindowPos bottom_right_pos = document_to_overview_pos(bottom_right);
    return NormalizedWindowRect(top_left_pos, bottom_right_pos);
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
            return static_cast<int>(search_results.size() - 1);
        }
        return res;
    };

    auto is_page_visible = [&](int page) {
        return std::find(visible_pages.begin(), visible_pages.end(), search_results[page].page) != visible_pages.end();
    };

    res.push_back(index);
    if (search_results.size() > 0) {
        int next = next_index(index);
        while ((next != index) && is_page_visible(next)) {
            res.push_back(next);
            next = next_index(next);
        }
        if (next != index) {
            int prev = prev_index(index);
            while (is_page_visible(prev)) {
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
    if ((breakpoint == search_results.size() - 1) || (search_results.size() == 1)) {
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
        else {
            return begin_index;
        }
    }
    else {
        if (search_results[midpoint].page >= search_results[begin_index].page) {
            return find_search_results_breakpoint_helper(midpoint, end_index);
        }
        else {
            return find_search_results_breakpoint_helper(begin_index, midpoint);
        }
    }
}

std::vector<DocumentRect> PdfViewOpenGLWidget::get_highlight_word_rects() {
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

    matmul<4, 4, 4>(outputs, inputs_inverse, matrix_data);
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

void PdfViewOpenGLWidget::set_underline(AbsoluteDocumentPos abspos) {
    underline = abspos;
}

void PdfViewOpenGLWidget::clear_underline() {
    underline = {};
}

PdfViewOpenGLWidget::ColorPalette PdfViewOpenGLWidget::get_current_color_mode() {
    return color_mode;
}

std::map<int, std::vector<MarkedDataRect>> PdfViewOpenGLWidget::get_marked_data_rect_map() {
    std::map<int, std::vector<MarkedDataRect>> res;
    for (auto mdr : marked_data_rects) {
        res[mdr.type].push_back(mdr);
    }
    return res;
}

void PdfViewOpenGLWidget::bind_points(const std::vector<float>& points) {
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.line_points_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points[0]) * points.size(), &points[0], GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void PdfViewOpenGLWidget::bind_default() {
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void PdfViewOpenGLWidget::add_coordinates_for_window_point(DocumentView* dv, float window_x, float window_y, float r, int point_polygon_vertices, std::vector<float>& out_coordinates){

    float thickness_x = dv->get_zoom_level() / width();
    float thickness_y = dv->get_zoom_level() / height();

    out_coordinates.push_back(window_x);
    out_coordinates.push_back(window_y);

    for (int i = 0; i <= point_polygon_vertices; i++) {
        out_coordinates.push_back(window_x + r * thickness_x * std::cos(2 * M_PI * i / point_polygon_vertices) / 2);
        out_coordinates.push_back(window_y + r * thickness_y * std::sin(2 * M_PI * i / point_polygon_vertices) / 2);
    }
}

template<typename T>
T lerp(T p1, T p2, float alpha) {
    float x = p1.x + alpha * (p2.x - p1.x);
    float y = p1.y + alpha * (p2.y - p1.y);
    return T{ x, y };
}

template<typename T>
T bezier_lerp(T p1, T p2, T p3, T p4, float alpha) {
    T q1 = lerp(p1, p2, alpha);
    T q2 = lerp(p2, p3, alpha);
    T q3 = lerp(p3, p4, alpha);

    T r1 = lerp(q1, q2, alpha);
    T r2 = lerp(q2, q3, alpha);
    return lerp(r1, r2, alpha);
}


FreehandDrawing smoothen_drawing(FreehandDrawing original) {
    if (original.points.size() < 3) {
        return original;
    }

    FreehandDrawing res;
    res.creattion_time = original.creattion_time;
    res.type = original.type;

    std::vector<Vec<float, 2>> velocities_at_points;
    velocities_at_points.reserve(original.points.size());

    velocities_at_points.push_back(original.points[1].pos - original.points[0].pos);

    for (int i = 1; i < original.points.size()-1; i++) {
        float alpha = 0.5f;
        velocities_at_points.push_back((original.points[i+1].pos - original.points[i - 1].pos) * alpha);
    }
    velocities_at_points.push_back(original.points[original.points.size()-1].pos - original.points[original.points.size()-2].pos);

    int N_POINTS = 4;
    FreehandDrawing smoothed_drawing;
    smoothed_drawing.creattion_time = original.creattion_time;
    smoothed_drawing.type = original.type;
    smoothed_drawing.points.reserve((original.points.size()-1) * N_POINTS);

    for (int i = 0; i < original.points.size()-1; i++){
        generate_bezier_with_endpoints_and_velocity(
            original.points[i].pos,
            original.points[i + 1].pos,
            velocities_at_points[i],
            velocities_at_points[i + 1],
            N_POINTS,
            original.points[i].thickness,
            smoothed_drawing.points
        );
        /* for (auto p : bezier_curve) { */
        /*     smoothed_drawing.points.push_back(FreehandDrawingPoint{ AbsoluteDocumentPos{ p.x(), p.y() }, original.points[i].thickness}); */
        /* } */
    }
    return smoothed_drawing;

}

void PdfViewOpenGLWidget::compile_drawings(DocumentView* dv, const std::vector<FreehandDrawing>& drawings) {
    if (scratchpad->cached_compiled_drawing_data) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->index_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->vertex_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->dots_index_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->dots_vertex_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->dots_uv_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->lines_type_index_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->dots_type_index_buffer);
        glDeleteVertexArrays(1, &scratchpad->cached_compiled_drawing_data->vao);
        scratchpad->cached_compiled_drawing_data = {};
    }
    if (drawings.size() == 0) {
        scratchpad->on_compile();
        return;
    }


    //float thickness_x = dv->get_zoom_level() / width();
    //float thickness_y = dv->get_zoom_level() / height();

    float thickness_x = 0.5f;
    float thickness_y = 0.5f;

    std::vector<float> coordinates;
    std::vector<unsigned int> indices;
    std::vector<GLint> type_indices;
    /* std::vector<short> */ 

    std::vector<float> dot_coordinates;
    std::vector<unsigned int> dot_indices;
    std::vector<GLint> dot_type_indices;
    /* std::vector<short> */ 
    GLuint primitive_restart_index = 0xFFFFFFFF;

    int index = 0;
    int dot_index = 0;

    auto add_point_coords = [&](FreehandDrawingPoint p, char index) {
        dot_coordinates.push_back(p.pos.x - p.thickness / 2);
        dot_coordinates.push_back(p.pos.y - p.thickness / 2);
        dot_type_indices.push_back(index);
        dot_indices.push_back(dot_index++);

        dot_coordinates.push_back(p.pos.x - p.thickness / 2);
        dot_coordinates.push_back(p.pos.y + p.thickness / 2);
        dot_type_indices.push_back(index);
        dot_indices.push_back(dot_index++);

        dot_coordinates.push_back(p.pos.x + p.thickness / 2);
        dot_coordinates.push_back(p.pos.y - p.thickness / 2);
        dot_type_indices.push_back(index);
        dot_indices.push_back(dot_index++);

        dot_coordinates.push_back(p.pos.x + p.thickness / 2);
        dot_coordinates.push_back(p.pos.y + p.thickness / 2);
        dot_type_indices.push_back(index);
        dot_indices.push_back(dot_index++);

        dot_indices.push_back(0xFFFFFFFF);
    };

    for (auto drawing : drawings) {
        if (DEBUG_SMOOTH_FREEHAND_DRAWINGS) {
            drawing = smoothen_drawing(drawing);
        }

        if (drawing.points.size() <= 0) {
            continue;
        }
        if (!visible_drawing_mask[drawing.type - 'a']) {
            continue;
        }
        if (drawing.points.size() == 1) {
            add_point_coords(drawing.points[0], drawing.type - 'a');

            continue;

        }

        float first_line_x = (drawing.points[1].pos.x - drawing.points[0].pos.x) * width();
        float first_line_y = (drawing.points[1].pos.y - drawing.points[0].pos.y) * height();
        float first_line_size = sqrt(first_line_x * first_line_x + first_line_y * first_line_y);
        first_line_x = first_line_x / first_line_size;
        first_line_y = first_line_y / first_line_size;

        float first_ortho_x = -first_line_y * thickness_x * drawing.points[0].thickness;
        float first_ortho_y = first_line_x * thickness_y * drawing.points[0].thickness;


        coordinates.push_back(drawing.points[0].pos.x - first_ortho_x);
        coordinates.push_back(drawing.points[0].pos.y - first_ortho_y);
        type_indices.push_back(drawing.type - 'a');
        indices.push_back(index++);
        coordinates.push_back(drawing.points[0].pos.x + first_ortho_x);
        coordinates.push_back(drawing.points[0].pos.y + first_ortho_y);
        type_indices.push_back(drawing.type - 'a');
        indices.push_back(index++);

        float prev_line_x = first_line_x;
        float prev_line_y = first_line_y;

        add_point_coords(drawing.points[0], drawing.type - 'a');
        add_point_coords(drawing.points.back(), drawing.type - 'a');
        for (int line_index = 0; line_index < drawing.points.size() - 1; line_index++) {
            float line_direction_x = (drawing.points[line_index + 1].pos.x - drawing.points[line_index].pos.x);
            float line_direction_y = (drawing.points[line_index + 1].pos.y - drawing.points[line_index].pos.y);
            float line_size = sqrt(line_direction_x * line_direction_x + line_direction_y * line_direction_y);
            line_direction_x = line_direction_x / line_size;
            line_direction_y = line_direction_y / line_size;

            float ortho_x1 = -line_direction_y * thickness_x * drawing.points[line_index].thickness;
            float ortho_y1 = line_direction_x * thickness_y * drawing.points[line_index].thickness;

            float ortho_x2 = -line_direction_y * thickness_x * drawing.points[line_index + 1].thickness;
            float ortho_y2 = line_direction_x * thickness_y * drawing.points[line_index + 1].thickness;

            coordinates.push_back(drawing.points[line_index].pos.x - ortho_x1);
            coordinates.push_back(drawing.points[line_index].pos.y - ortho_y1);
            type_indices.push_back(drawing.type - 'a');
            indices.push_back(index++);

            coordinates.push_back(drawing.points[line_index].pos.x + ortho_x1);
            coordinates.push_back(drawing.points[line_index].pos.y + ortho_y1);
            type_indices.push_back(drawing.type - 'a');
            indices.push_back(index++);

            coordinates.push_back(drawing.points[line_index + 1].pos.x - ortho_x1);
            coordinates.push_back(drawing.points[line_index + 1].pos.y - ortho_y1);
            type_indices.push_back(drawing.type - 'a');
            indices.push_back(index++);

            coordinates.push_back(drawing.points[line_index + 1].pos.x + ortho_x2);
            coordinates.push_back(drawing.points[line_index + 1].pos.y + ortho_y2);
            type_indices.push_back(drawing.type - 'a');
            indices.push_back(index++);

        }
        indices.push_back(primitive_restart_index);

        //std::vector<int> indices;

        //bind_points(coordinates);
        //glDrawArrays(GL_TRIANGLE_STRIP, 0, coordinates.size() / 2);
    }
    scratchpad->cached_compiled_drawing_data = compile_drawings_into_vertex_and_index_buffers(
        coordinates,
        indices,
        type_indices,
        dot_coordinates,
        dot_indices,
        dot_type_indices);
    scratchpad->on_compile();
}

//void PdfViewOpenGLWidget::compile_drawings_into_vertex_and_index_buffers(std::vector<float>& line_coordinates) {

CompiledDrawingData PdfViewOpenGLWidget::compile_drawings_into_vertex_and_index_buffers(const std::vector<float>& line_coordinates,
    const std::vector<unsigned int>& indices,
    const std::vector<GLint>& line_type_indices,
    const std::vector<float>& dot_coordinates,
    const std::vector<unsigned int>& dot_indices,
    const std::vector<GLint>& dot_type_indices){
    //std::vector<unsigned int> indices;
    //int num_rectangles = line_coordinates.size() / 

    std::vector<float> dot_uv_coordinates;
    dot_uv_coordinates.reserve(dot_coordinates.size());
    float uv_map[4 * 2] = { 0.0f, 0.0f,
                            1.0f, 0.0f,
                            0.0f, 1.0f,
                            1.0f, 1.0f
    };

    for (int i = 0; i < dot_coordinates.size(); i++) {
        dot_uv_coordinates.push_back(uv_map[i % 8]);
    }

    GLuint compiled_drawing_vao;
    glGenVertexArrays(1, &compiled_drawing_vao);
    GLuint compiled_vertex_array,
        compiled_index_array,
        dots_vertex_buffer,
        dots_index_buffer,
        dots_uv_buffer,
        lines_type_index_buffer,
        dots_type_index_buffer;

    glGenBuffers(1, &compiled_vertex_array);
    glGenBuffers(1, &compiled_index_array);
    glGenBuffers(1, &dots_vertex_buffer);
    glGenBuffers(1, &dots_index_buffer);
    glGenBuffers(1, &dots_uv_buffer);
    glGenBuffers(1, &lines_type_index_buffer);
    glGenBuffers(1, &dots_type_index_buffer);

    glBindVertexArray(compiled_drawing_vao);

    glBindBuffer(GL_ARRAY_BUFFER, compiled_vertex_array);
    glBufferData(GL_ARRAY_BUFFER, line_coordinates.size() * sizeof(float), line_coordinates.data(), GL_STATIC_DRAW);


    //glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    //glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, lines_type_index_buffer);
    glBufferData(GL_ARRAY_BUFFER, line_type_indices.size() * sizeof(GLint), line_type_indices.data(), GL_STATIC_DRAW);

    //glVertexAttribPointer(1, 1, GL_INT, GL_FALSE, 2 * sizeof(float), (void*)0);
    //glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, dots_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, dot_coordinates.size() * sizeof(float), dot_coordinates.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, dots_uv_buffer);
    glBufferData(GL_ARRAY_BUFFER, dot_uv_coordinates.size() * sizeof(float), dot_uv_coordinates.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, dots_type_index_buffer);
    glBufferData(GL_ARRAY_BUFFER, dot_type_indices.size() * sizeof(GLint), dot_type_indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, compiled_index_array);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dots_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, dot_indices.size() * sizeof(unsigned int), dot_indices.data(), GL_STATIC_DRAW);

    CompiledDrawingData res;
    res.vao = compiled_drawing_vao;
    res.vertex_buffer = compiled_vertex_array;
    res.index_buffer = compiled_index_array;
    res.dots_vertex_buffer = dots_vertex_buffer;
    res.dots_index_buffer = dots_index_buffer;
    res.dots_uv_buffer = dots_uv_buffer;
    res.lines_type_index_buffer = lines_type_index_buffer;
    res.dots_type_index_buffer = dots_type_index_buffer;
    res.n_elements = indices.size();
    res.n_dot_elements = dot_indices.size();
    return res;

}

void PdfViewOpenGLWidget::render_compiled_drawings() {
    //NormalizedWindowPos res;
    //float half_width = static_cast<float>(view_width) / zoom_level / 2;
    //float half_height = static_cast<float>(view_height) / zoom_level / 2;

    //res.x = (abs.x + offset_x) / half_width;
    //res.y = (-abs.y + offset_y) / half_height;
    // res.x = abs.x / half_width + offset_x / half_width
    //return res;
    float mode_highlight_colors[26 * 3];
    for (int i = 0; i < 26; i++) {
        auto res = cc3(&HIGHLIGHT_COLORS[3 * i]);
        mode_highlight_colors[i * 3 + 0] = res[0];
        mode_highlight_colors[i * 3 + 1] = res[1];
        mode_highlight_colors[i * 3 + 2] = res[2];
    }

    if (scratchpad->cached_compiled_drawing_data.has_value()) {
        float scale[] = {
            2 * scratchpad->get_zoom_level() / scratchpad->get_view_width() ,
            2 * scratchpad->get_zoom_level() / scratchpad->get_view_height()
        };
        float offset[] = { scratchpad->get_offset_x() * scale[0], scratchpad->get_offset_y() * scale[1]};
        float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
        //float color[] = {1.0f, 0.0f, 0.0f, 1.0f};

        glBindVertexArray(scratchpad->cached_compiled_drawing_data->vao);

        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->vertex_buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->lines_type_index_buffer);
        glVertexAttribIPointer(1, 1, GL_INT, 0, 0);
        glEnableVertexAttribArray(1);


        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->index_buffer);

        glUseProgram(shared_gl_objects.compiled_drawing_program);
        glUniform2fv(shared_gl_objects.compiled_drawing_offset_uniform_location, 1, offset);
        glUniform2fv(shared_gl_objects.compiled_drawing_scale_uniform_location, 1, scale);
        glUniform3fv(shared_gl_objects.compiled_drawing_colors_uniform_location, 26, mode_highlight_colors);
        //glUniform4fv(shared_gl_objects.compiled_drawing_color_uniform_location, 1, color);
        glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        glDrawElements(GL_TRIANGLE_STRIP, scratchpad->cached_compiled_drawing_data->n_elements, GL_UNSIGNED_INT, 0);

        //// bind dots buffers
        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->dots_vertex_buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->dots_uv_buffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->dots_type_index_buffer);
        glVertexAttribIPointer(2, 1, GL_INT, 0, 0);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->dots_index_buffer);

        glUseProgram(shared_gl_objects.compiled_dots_program);
        glUniform2fv(shared_gl_objects.compiled_dot_offset_uniform_location, 1, offset);
        glUniform2fv(shared_gl_objects.compiled_dot_scale_uniform_location, 1, scale);
        glUniform3fv(shared_gl_objects.compiled_dot_color_uniform_location, 26, mode_highlight_colors);

        glEnable(GL_BLEND);
        glDrawElements(GL_TRIANGLE_STRIP, scratchpad->cached_compiled_drawing_data->n_dot_elements, GL_UNSIGNED_INT, 0);
        glDisable(GL_BLEND);

    }
}

void PdfViewOpenGLWidget::render_drawings(DocumentView* dv, const std::vector<FreehandDrawing>& drawings, bool highlighted) {

    float time_diff = last_scratchpad_update_datetime.msecsTo(QDateTime::currentDateTime());
    last_scratchpad_update_datetime = QDateTime::currentDateTime();

    const int N_POINT_VERTICES = 10;
    float thickness_x = dv->get_zoom_level() / width();
    float thickness_y = dv->get_zoom_level() / height();
    //float thickness_y = thickness_x * width() / height();

    for (auto drawing : drawings) {
        if (DEBUG_SMOOTH_FREEHAND_DRAWINGS) {
            drawing = smoothen_drawing(drawing);
        }
        //if (drawings.size() % 2 == 0){
        //    drawing = smoothen_drawing(drawing);
        //}

        if (drawing.points.size() <= 0) {
            continue;
        }
        if (!visible_drawing_mask[drawing.type - 'a']) {
            continue;
        }

        float current_drawing_color_[4] = { HIGHLIGHT_COLORS[(drawing.type - 'a') * 3],
            HIGHLIGHT_COLORS[(drawing.type - 'a') * 3 + 1],
             HIGHLIGHT_COLORS[(drawing.type - 'a') * 3 + 2],
            1.0f
        };
        float current_drawing_color[4] = {0};
        get_color_for_current_mode(current_drawing_color_, current_drawing_color);
        current_drawing_color[3] = current_drawing_color_[3];

        if (highlighted) {
            current_drawing_color[0] = 1.0f;
            current_drawing_color[1] = 1.0f;
            current_drawing_color[2] = 0.0f;

        }

        if (drawing.points.size() == 1) {
            std::vector<float> coordinates;
            //float window_x, window_y;
            float thickness = drawing.points[0].thickness;

            NormalizedWindowPos window_pos = dv->absolute_to_window_pos({ drawing.points[0].pos.x, drawing.points[0].pos.y });
            add_coordinates_for_window_point(dv, window_pos.x, window_pos.y, thickness * 2, N_POINT_VERTICES, coordinates);

            bind_points(coordinates);
            glUniform4fv(shared_gl_objects.freehand_line_color_uniform_location, 1, current_drawing_color);
            glDrawArrays(GL_TRIANGLE_FAN, 0, coordinates.size() / 2);
            continue;
        }

        if (DEBUG_DISPLAY_FREEHAND_POINTS) {
            for (auto point : drawing.points) {
                std::vector<float> coordinates;
                //float window_x, window_y;
                float thickness = drawing.points[0].thickness;

                NormalizedWindowPos window_pos = dv->absolute_to_window_pos({ point.pos.x, point.pos.y });
                add_coordinates_for_window_point(dv, window_pos.x, window_pos.y, thickness * 2, N_POINT_VERTICES, coordinates);

                bind_points(coordinates);
                glUniform4fv(shared_gl_objects.freehand_line_color_uniform_location, 1, current_drawing_color);
                glDrawArrays(GL_TRIANGLE_FAN, 0, coordinates.size() / 2);
            }
            continue;
        }

        glUniform4fv(shared_gl_objects.freehand_line_color_uniform_location, 1, current_drawing_color);

        std::vector<NormalizedWindowPos> window_positions;
        window_positions.reserve(drawing.points.size());

        for (auto p : drawing.points) {
            NormalizedWindowPos window_pos = dv->absolute_to_window_pos({ p.pos.x, p.pos.y });
            window_positions.push_back(NormalizedWindowPos{ window_pos.x, window_pos.y });
        }

        std::vector<float> coordinates;
        /* coordinates.reserve(drawing.points.size() * 8 - 4); */
        std::vector<float> begin_point_coordinates;
        std::vector<float> end_point_coordinates;
        /* std::vector<std::vector<float>> all_point_coordinates; */

        float first_line_x = (window_positions[1].x - window_positions[0].x) * width();
        float first_line_y = (window_positions[1].y - window_positions[0].y) * height();
        float first_line_size = sqrt(first_line_x * first_line_x + first_line_y * first_line_y);
        first_line_x = first_line_x / first_line_size;
        first_line_y = first_line_y / first_line_size;
        float highlight_factor = highlighted ? 3.0f : 1.0f;

        float first_ortho_x = -first_line_y * thickness_x * drawing.points[0].thickness * highlight_factor;
        float first_ortho_y = first_line_x * thickness_y * drawing.points[0].thickness * highlight_factor;


        coordinates.push_back(window_positions[0].x - first_ortho_x);
        coordinates.push_back(window_positions[0].y - first_ortho_y);
        coordinates.push_back(window_positions[0].x + first_ortho_x);
        coordinates.push_back(window_positions[0].y + first_ortho_y);
        add_coordinates_for_window_point(dv, window_positions[0].x, window_positions[0].y, drawing.points[0].thickness * 2, N_POINT_VERTICES, begin_point_coordinates);
        float prev_line_x = first_line_x;
        float prev_line_y = first_line_y;

        for (int line_index = 0; line_index < drawing.points.size() - 1; line_index++) {
            float line_direction_x = (window_positions[line_index + 1].x - window_positions[line_index].x) * width();
            float line_direction_y = (window_positions[line_index + 1].y - window_positions[line_index].y) * height();
            float line_size = sqrt(line_direction_x * line_direction_x + line_direction_y * line_direction_y);
            line_direction_x = line_direction_x / line_size;
            line_direction_y = line_direction_y / line_size;

            float ortho_x1 = -line_direction_y * thickness_x * drawing.points[line_index].thickness * highlight_factor;
            float ortho_y1 = line_direction_x * thickness_y * drawing.points[line_index].thickness * highlight_factor;

            float ortho_x2 = -line_direction_y * thickness_x * drawing.points[line_index + 1].thickness * highlight_factor;
            float ortho_y2 = line_direction_x * thickness_y * drawing.points[line_index + 1].thickness * highlight_factor;

            float dot_prod_with_prev_direction = (prev_line_x * line_direction_x + prev_line_y * line_direction_y);

            coordinates.push_back(window_positions[line_index].x - ortho_x1);
            coordinates.push_back(window_positions[line_index].y - ortho_y1);
            coordinates.push_back(window_positions[line_index].x + ortho_x1);
            coordinates.push_back(window_positions[line_index].y + ortho_y1);

            coordinates.push_back(window_positions[line_index + 1].x - ortho_x1);
            coordinates.push_back(window_positions[line_index + 1].y - ortho_y1);
            coordinates.push_back(window_positions[line_index + 1].x + ortho_x2);
            coordinates.push_back(window_positions[line_index + 1].y + ortho_y2);

            /* std::vector<float> point_coordinates; */
            /* add_coordinates_for_window_point(dv, window_positions[line_index+1].x, window_positions[line_index+1].y, drawing.points[line_index+1].thickness * 2, N_POINT_VERTICES, point_coordinates); */
            /* all_point_coordinates.push_back(point_coordinates); */

        }

        add_coordinates_for_window_point(dv,
            window_positions[drawing.points.size() - 1].x,
            window_positions[drawing.points.size() - 1].y,
            drawing.points[drawing.points.size() - 1].thickness * 2 * highlight_factor,
            N_POINT_VERTICES,
            end_point_coordinates
        );

        std::vector<int> indices;


        //draw the lines
        bind_points(coordinates);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, coordinates.size() / 2);

        // draw a point at the start and end of drawing
        bind_points(begin_point_coordinates);
        glDrawArrays(GL_TRIANGLE_FAN, 0, begin_point_coordinates.size() / 2);
        bind_points(end_point_coordinates);
        glDrawArrays(GL_TRIANGLE_FAN, 1, end_point_coordinates.size() / 2);

        //for (auto coords : all_point_coordinates) {
        //    bind_points(coords);
        //    glDrawArrays(GL_TRIANGLE_FAN, 0, coords.size() / 2);
        //}
    }
}


bool PdfViewOpenGLWidget::is_normalized_y_in_window(float y) {
    return (y >= -1) && (y <= 1);
}

bool PdfViewOpenGLWidget::is_normalized_y_range_in_window(float y0, float y1) {
    if (y0 <= -1.0 && y1 >= 1.0f) return true;
    return is_normalized_y_in_window(y0) || is_normalized_y_in_window(y1);
}

void PdfViewOpenGLWidget::render_portal_rect(QPainter* painter, AbsoluteRect portal_rect, bool is_pending) {
    NormalizedWindowRect window_rect = portal_rect.to_window_normalized(document_view);

    if (is_normalized_y_range_in_window(window_rect.y0, window_rect.y1)) {
        fz_irect portal_window_rect = document_view->normalized_to_window_rect(window_rect);
        QRect window_qrect = QRect(portal_window_rect.x0, portal_window_rect.y0, fz_irect_width(portal_window_rect), fz_irect_height(portal_window_rect));
        QColor fill_color = QColor(255, 0, 0);
        if (is_pending) {
            fill_color = QColor(255, 255, 0);
        }

        if (is_pending) {
            hourglass_icon.paint(painter, window_qrect);
        }
        else{
            render_ui_icon_for_current_color_mode(painter, portal_icon, portal_icon_white, window_qrect);
        }
        //painter->fillRect(portal_window_rect.x0, portal_window_rect.y0, fz_irect_width(portal_window_rect), fz_irect_height(portal_window_rect), fill_color);

        //QFont font = painter->font();
        //font.setPointSizeF(5.0f * document_view->get_zoom_level());
        //painter->setFont(font);

        //painter->setPen(QColor(0, 0, 0));
        //painter->drawRect(portal_window_rect.x0, portal_window_rect.y0, fz_irect_width(portal_window_rect), fz_irect_height(portal_window_rect));
        //painter->drawText(window_qrect, Qt::AlignCenter, "P");
    }
}

void PdfViewOpenGLWidget::set_pending_download_portals(std::vector<AbsoluteRect>&& portal_rects){
    pending_download_portals = std::move(portal_rects);
}

bool PdfViewOpenGLWidget::should_show_synxtex_highlights() {
    if (synctex_highlights.size() > 0) {
        if ((HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT < 0) || (synctex_highlight_time.msecsTo(QTime::currentTime()) < (HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT * 1000.0f))) {
            return true;
        }
    }
    return false;

}

bool PdfViewOpenGLWidget::has_synctex_timed_out() {
    if (synctex_highlights.size() > 0 && (!should_show_synxtex_highlights())) {
        synctex_highlights.clear();
        return true;
    }
    return false;
}

struct UIRectDescriptor {
    UIRect* ui_rect;
    std::wstring* tap_command;
    std::wstring* hold_command;
    std::string name;
};

std::vector<std::pair<QRect, QString>> PdfViewOpenGLWidget::get_hint_rect_and_texts() {
    std::vector<std::pair< QRect, QString>> res;

    std::vector<UIRectDescriptor> rect_descriptors;

    if (screen()->orientation() == Qt::PortraitOrientation) {
        rect_descriptors = {
            UIRectDescriptor {&PORTRAIT_BACK_UI_RECT, &BACK_RECT_TAP_COMMAND, &BACK_RECT_HOLD_COMMAND, "back"},
            UIRectDescriptor {&PORTRAIT_FORWARD_UI_RECT, &FORWARD_RECT_TAP_COMMAND, &FORWARD_RECT_HOLD_COMMAND, "forward"},
            UIRectDescriptor {&PORTRAIT_VISUAL_MARK_PREV, &VISUAL_MARK_PREV_TAP_COMMAND, &VISUAL_MARK_PREV_HOLD_COMMAND, "move_ruler_prev"},
            UIRectDescriptor {&PORTRAIT_VISUAL_MARK_NEXT, &VISUAL_MARK_NEXT_TAP_COMMAND, &VISUAL_MARK_NEXT_HOLD_COMMAND, "move_ruler_next"},
            UIRectDescriptor {&PORTRAIT_MIDDLE_LEFT_UI_RECT, &MIDDLE_LEFT_RECT_TAP_COMMAND, &MIDDLE_LEFT_RECT_HOLD_COMMAND, "middle_left"},
            UIRectDescriptor {&PORTRAIT_MIDDLE_RIGHT_UI_RECT, &MIDDLE_RIGHT_RECT_TAP_COMMAND, &MIDDLE_RIGHT_RECT_HOLD_COMMAND, "middle_right"},
            UIRectDescriptor {&PORTRAIT_EDIT_PORTAL_UI_RECT, &EDIT_PORTAL_TAP_COMMAND, &EDIT_PORTAL_HOLD_COMMAND, "edit_portal"},
        };
    }
    else {
        rect_descriptors = {
            UIRectDescriptor {&LANDSCAPE_BACK_UI_RECT, &BACK_RECT_TAP_COMMAND, &BACK_RECT_HOLD_COMMAND, "back"},
            UIRectDescriptor {&LANDSCAPE_FORWARD_UI_RECT, &FORWARD_RECT_TAP_COMMAND, &FORWARD_RECT_HOLD_COMMAND, "forward"},
            UIRectDescriptor {&LANDSCAPE_VISUAL_MARK_PREV, &VISUAL_MARK_PREV_TAP_COMMAND, &VISUAL_MARK_PREV_HOLD_COMMAND, "move_ruler_prev"},
            UIRectDescriptor {&LANDSCAPE_VISUAL_MARK_NEXT, &VISUAL_MARK_NEXT_TAP_COMMAND, &VISUAL_MARK_NEXT_HOLD_COMMAND, "move_ruler_next"},
            UIRectDescriptor {&LANDSCAPE_MIDDLE_LEFT_UI_RECT, &MIDDLE_LEFT_RECT_TAP_COMMAND, &MIDDLE_LEFT_RECT_HOLD_COMMAND, "middle_left"},
            UIRectDescriptor {&LANDSCAPE_MIDDLE_RIGHT_UI_RECT, &MIDDLE_RIGHT_RECT_TAP_COMMAND, &MIDDLE_RIGHT_RECT_HOLD_COMMAND, "middle_right"},
            UIRectDescriptor {&LANDSCAPE_EDIT_PORTAL_UI_RECT, &EDIT_PORTAL_TAP_COMMAND, &EDIT_PORTAL_HOLD_COMMAND, "edit_portal"},
        };
    }

    for (auto desc : rect_descriptors) {
        bool is_visual = QString::fromStdString(desc.name).startsWith("move_ruler");

        if (is_visual || (desc.ui_rect->enabled && ((desc.hold_command->size() > 0) || (desc.tap_command->size() > 0)))) {
            QString str = "";
            if (desc.tap_command->size() > 0) {
                str += "tap: " + QString::fromStdWString(*desc.tap_command);
            }
            if (desc.hold_command->size() > 0) {
                if (str.size() > 0) str += "\n";
                str += "hold: " + QString::fromStdWString(*desc.hold_command);
            }
            if (is_visual) {
                if (str.size() > 0) str += "\n";
                str += "visual: " + QString::fromStdString(desc.name);
            }
            res.push_back(std::make_pair(desc.ui_rect->to_window(width(), height()), str));
        }
    }
    return res;
} 

void PdfViewOpenGLWidget::show_rect_hints() {
    should_show_rect_hints = true;
}

void PdfViewOpenGLWidget::hide_rect_hints() {
    should_show_rect_hints = false;
}

bool PdfViewOpenGLWidget::is_showing_rect_hints() {
    return should_show_rect_hints;
}

void PdfViewOpenGLWidget::get_color_for_current_mode(const float* input_color, float* output_color) {
    if (!ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE) {
        output_color[0] = input_color[0];
        output_color[1] = input_color[1];
        output_color[2] = input_color[2];
        return;
    }

    if (color_mode == ColorPalette::Dark) {
        float inverted_color[3];
        inverted_color[0] = (0.5f - input_color[0]) * DARK_MODE_CONTRAST + 0.5f;
        inverted_color[1] = (0.5f - input_color[1]) * DARK_MODE_CONTRAST + 0.5f;
        inverted_color[2] = (0.5f - input_color[2]) * DARK_MODE_CONTRAST + 0.5f;
        float hsv_color[3];
        rgb2hsv(inverted_color, hsv_color);
        float new_hue = fmod(hsv_color[0] + 0.5f, 1.0f);
        hsv_color[0] = new_hue;
        hsv2rgb(hsv_color, output_color);
    }
    else if (color_mode == ColorPalette::Custom) {
        float transform_matrix[16];
        float input_vector[4];
        float output_vector[4];
        input_vector[0] = input_color[0];
        input_vector[1] = input_color[1];
        input_vector[2] = input_color[2];
        input_vector[3] = 1.0f;

        get_custom_color_transform_matrix(transform_matrix);
        matmul<4, 4, 1>(transform_matrix, input_vector, output_vector);
        output_color[0] = fz_clamp(output_vector[0], 0, 1);
        output_color[1] = fz_clamp(output_vector[1], 0, 1);
        output_color[2] = fz_clamp(output_vector[2], 0, 1);
        return;
    }
    else {
        output_color[0] = input_color[0];
        output_color[1] = input_color[1];
        output_color[2] = input_color[2];
    }
}

std::array<float, 3> PdfViewOpenGLWidget::cc3(const float* input_color) {
    std::array<float, 3> result;
    get_color_for_current_mode(input_color, &result[0]);
    return result;
}

std::array<float, 4> PdfViewOpenGLWidget::cc4(const float* input_color) {
    std::array<float, 4> result;
    get_color_for_current_mode(input_color, &result[0]);
    result[3] = input_color[3];
    return result;
}

void PdfViewOpenGLWidget::render_ui_icon_for_current_color_mode(QPainter* painter, const QIcon& icon_black, const QIcon& icon_white, QRect window_qrect){

    float visible_annotation_icon_color[3] = {1, 1, 1};
    float mode_color[3] = {0};
    get_color_for_current_mode(visible_annotation_icon_color, mode_color);
    int num_adjust_pixels = static_cast<int>(window_qrect.width() * 0.1f);

    painter->fillRect(window_qrect.adjusted(num_adjust_pixels, num_adjust_pixels, -num_adjust_pixels, -num_adjust_pixels), convert_float3_to_qcolor(mode_color));

    if (is_bright(mode_color)){
        icon_black.paint(painter, window_qrect);
    }
    else{
        icon_white.paint(painter, window_qrect);
    }
}

void PdfViewOpenGLWidget::render_text_highlights(){

    std::array<float, 3> text_highlight_color = cc3(DEFAULT_TEXT_HIGHLIGHT_COLOR);
    glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &text_highlight_color[0]);
    glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
    std::vector<AbsoluteRect> bounding_rects;
    merge_selected_character_rects(*document_view->get_selected_character_rects(), bounding_rects);

    for (auto rect : bounding_rects) {
        render_highlight_absolute(shared_gl_objects.highlight_program, rect, HRF_FILL | HRF_BORDER);
    }
}

void PdfViewOpenGLWidget::render_highlight_annotations(){
    if (document_view->get_document()->can_use_highlights()) {
        const std::vector<Highlight>& highlights = document_view->get_document()->get_highlights();
        std::vector<int> visible_highlight_indices = document_view->get_visible_highlight_indices();

        for (size_t ind = 0; ind < visible_highlight_indices.size(); ind++) {
            int i = visible_highlight_indices[ind];
                for (size_t j = 0; j < highlights[i].highlight_rects.size(); j++) {
                    //glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &HIGHLIGHT_COLORS[(highlights[i].type - 'a') * 3]);
                    auto adjusted_highlight_color = cc3(get_highlight_type_color(highlights[i].type));
                    get_color_for_current_mode(get_highlight_type_color(highlights[i].type), &adjusted_highlight_color[0]);

                    //glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, get_highlight_type_color(highlights[i].type));
                    glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &adjusted_highlight_color[0]);
                    glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);

                    int flags = 0;
                    if (std::isupper(highlights[i].type)) {
                        flags |= HRF_UNDERLINE;
                    }
                    if (highlights[i].type == '_') {
                        flags |= HRF_STRIKE;
                    }
                    if (flags == 0) {
                        flags |= HRF_FILL;
                        if (i == selected_highlight_index) {
                            flags |= HRF_BORDER;
                        }
                    }
                    render_highlight_absolute(shared_gl_objects.highlight_program,
                            highlights[i].highlight_rects[j],
                            flags);
                }
        }
    }
}

bool PdfViewOpenGLWidget::on_vertical_scroll(){

    // returns true if the scroll even caused some change
    // in this widget
    bool res = false;

    if (should_highlight_words){
        should_highlight_words = false;
        res = true;
    }
    if (should_highlight_links){
        should_highlight_links = false;
        res = true;
    }
    return res;
}

void PdfViewOpenGLWidget::set_overview_highlights(const std::vector<DocumentRect>& rects){
    overview_highlights = rects;
}

bool PdfViewOpenGLWidget::needs_stencil_buffer() {
    return fastread_mode || selected_rectangle.has_value() || overview_page.has_value();
}

void PdfViewOpenGLWidget::zoom_overview(float scale){
    if (overview_page){
        float new_zoom_level = overview_page->zoom_level * scale;
        float min_zoom_level = 1.0f;
        if (new_zoom_level < min_zoom_level) new_zoom_level = min_zoom_level;
        if (new_zoom_level > 6.0) new_zoom_level = 6.0;
        overview_page->zoom_level = new_zoom_level;
    }
}

void PdfViewOpenGLWidget::zoom_in_overview(){
    zoom_overview(ZOOM_INC_FACTOR);
}

void PdfViewOpenGLWidget::zoom_out_overview(){
    zoom_overview(1.0f / ZOOM_INC_FACTOR);
}

void PdfViewOpenGLWidget::set_scratchpad(ScratchPad* pad) {
    scratchpad = pad;
}

ScratchPad* PdfViewOpenGLWidget::get_scratchpad() {
    return scratchpad;
}

void PdfViewOpenGLWidget::render_selected_rectangle() {

    if (selected_rectangle) {
        enable_stencil();

        write_to_stencil();
        float rectangle_color[] = { 0.0f, 0.0f, 0.0f };
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, rectangle_color);
        glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
        if (!(selected_rectangle.value() == fz_empty_rect)) {
            render_highlight_absolute(shared_gl_objects.highlight_program, selected_rectangle.value(), HRF_FILL | HRF_BORDER);
        }

        use_stencil_to_write(false);

        NormalizedWindowRect window_rect({ -1, -1, 1, 1 });
        render_highlight_window(shared_gl_objects.highlight_program, window_rect, true);

        disable_stencil();
    }
}

bool PdfViewOpenGLWidget::can_use_cached_scratchpad_framebuffer() {
    float current_offset_x = scratchpad->get_offset_x();
    float current_offset_y = scratchpad->get_offset_y();
    float current_zoom_level = scratchpad->get_zoom_level();
    if ((current_offset_x == last_cache_offset_x) && (current_offset_y == last_cache_offset_y) && (current_zoom_level == last_cache_zoom_level)){
        return true;
    }
    /* if (current_drawing.points.size() > 0 && cached_framebuffer.has_value()) { */
    /*     if (last_cache_num_drawings == scratchpad->drawings.size()) { */
    /*         return true; */
    /*     } */
    /* } */
    return false;
}

void PdfViewOpenGLWidget::clear_background_color() {
    if (color_mode == ColorPalette::Dark) {
        glClearColor(DARK_MODE_BACKGROUND_COLOR[0], DARK_MODE_BACKGROUND_COLOR[1], DARK_MODE_BACKGROUND_COLOR[2], 1.0f);
    }
    else if (color_mode == ColorPalette::Custom) {
        glClearColor(CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[0], CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[1], CUSTOM_COLOR_MODE_EMPTY_BACKGROUND_COLOR[2], 1.0f);
    }
    else {
        glClearColor(BACKGROUND_COLOR[0], BACKGROUND_COLOR[1], BACKGROUND_COLOR[2], 1.0f);
    }

    // for some reason clearing the stencil buffer tanks the performance on android 
    // so we only clear it if we absolutely need it
    if (needs_stencil_buffer()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    else {
        glClear(GL_COLOR_BUFFER_BIT);
    }
}

void PdfViewOpenGLWidget::set_tag_prefix(std::wstring prefix) {
    tag_prefix = utf8_encode(prefix);
}

void PdfViewOpenGLWidget::clear_tag_prefix() {
    tag_prefix = "";
}

void PdfViewOpenGLWidget::set_selected_highlight_index(int index) {
    selected_highlight_index = index;
}
