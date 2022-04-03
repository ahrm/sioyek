//todo: cleanup the code
//todo: tests!
//todo: clean up parsing code
//todo: simplify word selection logic (also avoid inefficient extra insertions followed by clears in selected_characters)
//todo: make it so that all commands that change document state (for example goto_offset_withing_page, goto_link, etc.) do not change the document
// state, instead they return a DocumentViewState object that is then applied using push_state and change_state functions
// (chnage state should be a function that just applies the state without pushing it to history)
//todo: handle input errors in command line parsing
//todo: fix configure_paths for MacOS
//todo: highlight sometimes doesn't work

#include <iostream>
#include <vector>
#include <string>
#include <regex>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <optional>
#include <utility>
#include <memory>

#include <qapplication.h>
#include <qpushbutton.h>
#include <qopenglwidget.h>
#include <qopenglextrafunctions.h>
#include <qopenglfunctions.h>
#include <qopengl.h>
#include <qwindow.h>
#include <qkeyevent.h>
#include <qlineedit.h>
#include <qtreeview.h>
#include <qsortfilterproxymodel.h>
#include <qabstractitemmodel.h>
#include <qopenglshaderprogram.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <qstackedwidget.h>
#include <qboxlayout.h>
#include <qlistview.h>
#include <qstringlistmodel.h>
#include <qlabel.h>
#include <qtextedit.h>
#include <qfilesystemwatcher.h>
#include <qdesktopwidget.h>
#include <qfontdatabase.h>
#include <qstandarditemmodel.h>
#include <qscrollarea.h>
#include <qdesktopservices.h>
#include <qprocess.h>
#include <qstandardpaths.h>
#include <qcommandlineparser.h>
#include <qdir.h>
#include <qsurfaceformat.h>

#include <mupdf/fitz.h>
#include "sqlite3.h"


#include "input.h"
#include "database.h"
#include "book.h"
#include "utils.h"
#include "ui.h"
#include "pdf_renderer.h"
#include "document.h"
#include "document_view.h"
#include "pdf_view_opengl_widget.h"
#include "config.h"
#include "utf8.h"
#include "main_widget.h"
#include "path.h"
#include "RunGuard.h"
#include "checksum.h"
#include "OpenWithApplication.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION

//#define LINUX_STANDARD_PATHS


std::wstring APPLICATION_NAME = L"sioyek";
std::string LOG_FILE_NAME = "sioyek_log.txt";
std::ofstream LOG_FILE;
int FONT_SIZE = -1;
int STATUS_BAR_FONT_SIZE = -1;
std::string APPLICATION_VERSION = "1.1.0";
float BACKGROUND_COLOR[3] = { 1.0f, 1.0f, 1.0f };
float DARK_MODE_BACKGROUND_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float CUSTOM_BACKGROUND_COLOR[3] = { 1.0f, 1.0f, 1.0f };
float CUSTOM_TEXT_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float STATUS_BAR_COLOR[3] = { 0.0f, 0.0f, 0.0f };
float STATUS_BAR_TEXT_COLOR[3] = { 1.0f, 1.0f, 1.0f };
std::wstring SEARCH_URLS[26];
std::wstring EXECUTE_COMMANDS[26];
std::wstring MIDDLE_CLICK_SEARCH_ENGINE = L"s";
std::wstring SHIFT_MIDDLE_CLICK_SEARCH_ENGINE = L"l";
float HIGHLIGHT_COLORS[26 * 3] = { \
0.5182963463943647, 0.052279561076773784, 0.42942409252886155, \
0.673198309637537, 0.14250443697242887, 0.1842972533900342, \
0.1259143196334698, 0.3472546716690144, 0.5268310086975159, \
0.44867634475259244, 0.36152631940627494, 0.18979733584113254, \
0.25561951195738114, 0.5203940586174391, 0.2239864294251798, \
0.46620566366115457, 0.34950449396122557, 0.18428984237761986, \
0.7766958121833649, 0.18529941752256116, 0.03800477029407397, \
0.14245148690647982, 0.27376105738246703, 0.5837874557110532, \
0.15069695522822338, 0.6757965126090706, 0.173506532162706, \
0.20214309005349734, 0.388109281902417, 0.40974762804408554, \
0.5282008406153603, 0.3604221142506678, 0.11137704513397183, \
0.11065494801726693, 0.43355028291683534, 0.4557947690658978, \
0.4623270941397442, 0.2575781303014751, 0.28009477555878065, \
0.13682260642246874, 0.843494092757017, 0.019683300820514175, \
0.3779898334061099, 0.10067511592122631, 0.5213350506726637, \
0.20252688176577896, 0.46636886381356, 0.33110425442066094, \
0.26429496078170356, 0.4214065241882322, 0.31429851503006423, \
0.2778665356071555, 0.31938061671537193, 0.40275284767747266, \
0.2859415758796114, 0.3778585576392479, 0.33619986648114064, \
0.06881479216543497, 0.49813975498043, 0.43304545285413504, \
0.5411102077276201, 0.050950286432382155, 0.4079395058399978, \
0.13956877643913856, 0.4133573589812949, 0.44707386457956655, \
0.5672781038824454, 0.026174925202518497, 0.4065469709150361, \
0.33594461744136966, 0.30463905854351836, 0.359416324015112, \
0.16837764593670387, 0.43225375356473283, 0.3993686004985632, \
0.21290269578043475, 0.5704883842115632, 0.21660892000800203, \
};

float DARK_MODE_CONTRAST = 0.8f;
float ZOOM_INC_FACTOR = 1.2f;
float VERTICAL_MOVE_AMOUNT = 1.0f;
float HORIZONTAL_MOVE_AMOUNT = 1.0f;
float MOVE_SCREEN_PERCENTAGE = 0.8f;
const unsigned int CACHE_INVALID_MILIES = 1000;
const int PERSIST_MILIES = 1000 * 60;
const int PAGE_PADDINGS = 0;
const int MAX_PENDING_REQUESTS = 31;
bool FLAT_TABLE_OF_CONTENTS = false;
bool SHOULD_USE_MULTIPLE_MONITORS = false;
bool SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP = true;
bool DEFAULT_DARK_MODE = false;
bool SORT_BOOKMARKS_BY_LOCATION = false;
std::wstring LIBGEN_ADDRESS = L"";
std::wstring GOOGLE_SCHOLAR_ADDRESS = L"";
std::wstring INVERSE_SEARCH_COMMAND = L"";
std::wstring SHARED_DATABASE_PATH = L"";
std::wstring UI_FONT_FACE_NAME = L"";
bool SHOULD_LOAD_TUTORIAL_WHEN_NO_OTHER_FILE = false;
bool SHOULD_LAUNCH_NEW_INSTANCE = true;
bool SHOULD_DRAW_UNRENDERED_PAGES = true;
bool HOVER_OVERVIEW = false;
bool RERENDER_OVERVIEW = false;
bool LINEAR_TEXTURE_FILTERING = false;
bool SMALL_TOC = false;
float VISUAL_MARK_NEXT_PAGE_FRACTION = 0.25f;
float VISUAL_MARK_NEXT_PAGE_THRESHOLD = 0.1f;
std::wstring ITEM_LIST_PREFIX = L">";
std::wstring STARTUP_COMMANDS = L"";
float SMALL_PIXMAP_SCALE = 0.75f;
float DISPLAY_RESOLUTION_SCALE = -1;
float FIT_TO_PAGE_WIDTH_RATIO = 1;
int MAIN_WINDOW_SIZE[2] = { -1, -1 };
int HELPER_WINDOW_SIZE[2] = { -1, -1 };
int MAIN_WINDOW_MOVE[2] = { -1, -1 };
int HELPER_WINDOW_MOVE[2] = { -1, -1 };
float TOUCHPAD_SENSITIVITY = 1.0f;
int SINGLE_MAIN_WINDOW_SIZE[2] = { -1, -1 };
int SINGLE_MAIN_WINDOW_MOVE[2] = { -1, -1 };

float PAGE_SEPARATOR_WIDTH = 0.0f;
float PAGE_SEPARATOR_COLOR[3] = {0.9f, 0.9f, 0.9f};

Path default_config_path(L"");
Path default_keys_path(L"");
std::vector<Path> user_config_paths = {};
std::vector<Path> user_keys_paths = {};
Path database_file_path(L"");
Path local_database_file_path(L"");
Path global_database_file_path(L"");
Path tutorial_path(L"");
Path last_opened_file_address_path(L"");
Path shader_path(L"");

QStringList convert_arguments(QStringList input_args){
    // convert the relative path of filename (if it exists) to absolute path

    QStringList output_args;

    //the first argument is always path of the executable
    output_args.push_back(input_args.at(0));
    input_args.pop_front();

    if (input_args.size() > 0){
        QString path = input_args.at(0);

        bool is_path_argument = true;

        if (path.size() > 2 && path.startsWith("--")){
            is_path_argument = false;
        }

        if (is_path_argument){
            Path path_object(path.toStdWString());
            output_args.push_back(QString::fromStdWString(path_object.get_path()));
            input_args.pop_front();
        }
    }
    for (int i =0; i < input_args.size(); i++){
        output_args.push_back(input_args.at(i));
    }

    return output_args;
}

void configure_paths(){

	Path parent_path(QCoreApplication::applicationDirPath().toStdWString());
	std::string exe_path = utf8_encode(QCoreApplication::applicationFilePath().toStdWString());

	shader_path = parent_path.slash(L"shaders");



#ifdef Q_OS_LINUX
	QStringList all_config_paths = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
#ifdef LINUX_STANDARD_PATHS
	Path home_path(QDir::homePath().toStdWString());
	Path standard_data_path = home_path.slash(L".local").slash(L"share").slash(L"sioyek");
	Path standard_config_path = Path(L"/etc/sioyek");
	Path read_only_data_path = Path(L"/usr/share/sioyek");
	standard_data_path.create_directories();

	default_config_path = standard_config_path.slash(L"prefs.config");
	default_keys_path = standard_config_path.slash(L"keys.config");

	for (int i = all_config_paths.size()-1; i >= 0; i--) {
		user_config_paths.push_back(Path(all_config_paths.at(i).toStdWString()).slash(L"prefs_user.config"));
		user_keys_paths.push_back(Path(all_config_paths.at(i).toStdWString()).slash(L"keys_user.config"));
	}

	database_file_path = standard_data_path.slash(L"test.db");
	local_database_file_path = standard_data_path.slash(L"local.db");
	global_database_file_path = standard_data_path.slash(L"shared.db");
	tutorial_path = read_only_data_path.slash(L"tutorial.pdf");
	last_opened_file_address_path = standard_data_path.slash(L"last_document_path.txt");
	shader_path = read_only_data_path.slash(L"shaders");
#else
	char* APPDIR = std::getenv("XDG_CONFIG_HOME");

	if (!APPDIR){
		APPDIR = std::getenv("HOME");
	}

	Path standard_data_path = Path(utf8_decode(APPDIR));
	standard_data_path = standard_data_path.slash(L".local").slash(L"share").slash(L"Sioyek");
	standard_data_path.create_directories();

	default_config_path = parent_path.slash(L"prefs.config");
	//user_config_path = standard_data_path.slash(L"prefs_user.config");
	user_config_paths.push_back(standard_data_path.slash(L"prefs_user.config"));
	default_keys_path = parent_path.slash(L"keys.config");
	user_keys_paths.push_back(standard_data_path.slash(L"keys_user.config"));
	database_file_path = standard_data_path.slash(L"test.db");
	local_database_file_path = standard_data_path.slash(L"local.db");
	global_database_file_path = standard_data_path.slash(L"shared.db");
	tutorial_path = standard_data_path.slash(L"tutorial.pdf");
	last_opened_file_address_path = standard_data_path.slash(L"last_document_path.txt");

	if (!tutorial_path.file_exists()) {
		copy_file(parent_path.slash(L"tutorial.pdf"), tutorial_path);
	}
#endif
#else //windows
#ifdef NDEBUG
	//install_app(exe_path.c_str());
#endif
	Path standard_data_path(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0).toStdWString());
	QStringList all_config_paths = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);

	standard_data_path.create_directories();

	default_config_path = parent_path.slash(L"prefs.config");
	default_keys_path = parent_path.slash(L"keys.config");
	tutorial_path = parent_path.slash(L"tutorial.pdf");

#ifdef NON_PORTABLE
	user_config_paths.push_back(standard_data_path.slash(L"prefs_user.config"));
	user_keys_paths.push_back(standard_data_path.slash(L"keys_user.config"));
	for (int i = all_config_paths.size()-1; i > 0; i--) {
		user_config_paths.push_back(Path(all_config_paths.at(i).toStdWString()).slash(L"prefs_user.config"));
		user_keys_paths.push_back(Path(all_config_paths.at(i).toStdWString()).slash(L"keys_user.config"));
	}
	database_file_path = standard_data_path.slash(L"test.db");
	local_database_file_path = standard_data_path.slash(L"local.db");
	global_database_file_path = standard_data_path.slash(L"shared.db");
	last_opened_file_address_path = standard_data_path.slash(L"last_document_path.txt");
#else
	user_config_paths.push_back(parent_path.slash(L"prefs_user.config"));
	user_keys_paths.push_back(parent_path.slash(L"keys_user.config"));
	database_file_path = parent_path.slash(L"test.db");
	local_database_file_path = parent_path.slash(L"local.db");
	global_database_file_path = parent_path.slash(L"shared.db");
	last_opened_file_address_path = parent_path.slash(L"last_document_path.txt");
#endif

#endif
}

void verify_paths(){
#define CHECK_DIR_EXIST(path) do{ if(!(path).dir_exists() ) std::wcout << L"Error: " << #path << ": " << path << L" doesn't exist!\n"; } while(false)
#define CHECK_FILE_EXIST(path) do{ if(!(path).file_exists() ) std::wcout << L"Error: " << #path << ": " << path << L" doesn't exist!\n"; } while(false)

    std::wcout << L"default_config_path: " << default_config_path << L"\n";
    CHECK_FILE_EXIST(default_config_path);
    std::wcout << L"default_keys_path: " << default_keys_path << L"\n";
    CHECK_FILE_EXIST(default_keys_path);
    for (size_t i = 0; i < user_config_paths.size(); i++) {
        std::wcout << L"user_config_path: [ " << i << " ] " << user_config_paths[i] << L"\n";
    }
    for (size_t i = 0; i < user_keys_paths.size(); i++) {
        std::wcout << L"user_keys_path: [ " << i << " ] " << user_keys_paths[i] << L"\n";
    }
    std::wcout << L"database_file_path: " << database_file_path << L"\n";
    std::wcout << L"local_database_file_path: " << local_database_file_path << L"\n";
    std::wcout << L"global_database_file_path: " << global_database_file_path << L"\n";
    std::wcout << L"tutorial_path: " << tutorial_path << L"\n";
    std::wcout << L"last_opened_file_address_path: " << last_opened_file_address_path << L"\n";
    std::wcout << L"shader_path: " << shader_path << L"\n";
    CHECK_DIR_EXIST(shader_path);

#undef CHECK_DIR_EXIST
#undef CHECK_FILE_EXIST
}

std::mutex mupdf_mutexes[FZ_LOCK_MAX];

void lock_mutex(void* user, int lock) {
	std::mutex* mut = (std::mutex*)user;
	(mut + lock)->lock();
}

void unlock_mutex(void* user, int lock) {
	std::mutex* mut = (std::mutex*)user;
	(mut + lock)->unlock();
}

void add_paths_to_file_system_watcher(QFileSystemWatcher& watcher, const Path& default_path, const std::vector<Path>& user_paths) {
	if (QFile::exists(QString::fromStdWString(default_path.get_path()))) {
		watcher.addPath(QString::fromStdWString(default_path.get_path()));
	}

    for (auto user_path : user_paths){
        if (QFile::exists(QString::fromStdWString(user_path.get_path()))) {
            watcher.addPath(QString::fromStdWString(user_path.get_path()));
        }
    }
}

int main(int argc, char* args[]) {

	QSurfaceFormat format;
	format.setVersion(3, 3);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);

	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	OpenWithApplication app(argc, args);

    QCommandLineParser* parser = get_command_line_parser();
    parser->process(app.arguments());

	configure_paths();
	verify_paths();

	ConfigManager config_manager(default_config_path, user_config_paths);

	if (SHARED_DATABASE_PATH.size() > 0) {
		global_database_file_path = SHARED_DATABASE_PATH;
	}
	char* shared_database_path_arg = get_argv_value(argc, args, "--shared-database-path");
	if (shared_database_path_arg) {
		global_database_file_path = utf8_decode(std::string(shared_database_path_arg));
	}

	// should we launche a new instance each time the user opens a PDF or should we reuse the previous instance
	bool use_single_instance = !SHOULD_LAUNCH_NEW_INSTANCE;

	if (should_reuse_instance(argc, args)) {
		use_single_instance = true;
	}
	else if (should_new_instance(argc, args)) {
		use_single_instance = false;
	}

	RunGuard guard("sioyek");

	if (use_single_instance) {
		if (!guard.isPrimary()) {
            QStringList sent_args = convert_arguments(app.arguments());
            guard.sendMessage(serialize_string_array(sent_args));
			return 0;
		}
	}

	QCoreApplication::setApplicationName(QString::fromStdWString(APPLICATION_NAME));
	QCoreApplication::setApplicationVersion(QString::fromStdString(APPLICATION_VERSION));

	QStringList positional_args = parser->positionalArguments();
	delete parser;

	DatabaseManager db_manager;
	if (local_database_file_path.file_exists() && global_database_file_path.file_exists()) {
		db_manager.open(local_database_file_path.get_path(), global_database_file_path.get_path());
	}
	else {
		db_manager.open(database_file_path.get_path(), database_file_path.get_path());
	}
	db_manager.ensure_database_compatibility(local_database_file_path.get_path(), global_database_file_path.get_path());

	fz_locks_context locks;
	locks.user = mupdf_mutexes;
	locks.lock = lock_mutex;
	locks.unlock = unlock_mutex;

	fz_context* mupdf_context = fz_new_context(nullptr, &locks, FZ_STORE_UNLIMITED);

	if (!mupdf_context) {
		std::cerr << "could not create mupdf context" << std::endl;
		return -1;
	}
	bool fail = false;
	fz_try(mupdf_context) {
		fz_register_document_handlers(mupdf_context);
	}
	fz_catch(mupdf_context) {
		std::cerr << "could not register document handlers" << std::endl;
		fail = true;
	}

	if (fail) {
		return -1;
	}

	bool quit = false;

	InputHandler input_handler(default_keys_path, user_keys_paths);

	std::vector<std::pair<std::wstring, std::wstring>> prev_path_hash_pairs;
	db_manager.get_prev_path_hash_pairs(prev_path_hash_pairs);

	CachedChecksummer checksummer(&prev_path_hash_pairs);

	DocumentManager document_manager(mupdf_context, &db_manager, &checksummer);

	QFileSystemWatcher pref_file_watcher;
	add_paths_to_file_system_watcher(pref_file_watcher, default_config_path, user_config_paths);

	QFileSystemWatcher key_file_watcher;
	add_paths_to_file_system_watcher(key_file_watcher, default_keys_path, user_keys_paths);

	MainWidget main_widget(mupdf_context, &db_manager, &document_manager, &config_manager, &input_handler, &checksummer, &quit);

	if (DEFAULT_DARK_MODE) {
		main_widget.toggle_dark_mode();
	}

	QString startup_commands_list = QString::fromStdWString(STARTUP_COMMANDS);
	QStringList startup_commands = startup_commands_list.split(";");
	CommandManager* command_manager = main_widget.get_command_manager();

	for (auto command : startup_commands) {
		main_widget.handle_command(command_manager->get_command_with_name(command.toStdString()), 1);
	}

	if (use_single_instance) {
		if (guard.isPrimary()) {
			QObject::connect(&guard, &RunGuard::messageReceived, [&main_widget](const QByteArray& message) {
				QStringList args = deserialize_string_array(message);
				main_widget.handle_args(args);
				main_widget.activateWindow();
				});
		}
	}

	main_widget.apply_window_params_for_one_window_mode();
	main_widget.show();

	main_widget.handle_args(app.arguments());

	// load input file from `QFileOpenEvent` for macOS drag and drop & "open with"
	QObject::connect(&app, &OpenWithApplication::file_ready, [&main_widget](const QString& file_name) {
		main_widget.handle_args(QStringList() << file_name << file_name);
	});

    // live reload the config files
	QObject::connect(&pref_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {

		config_manager.deserialize(default_config_path, user_config_paths);

		ConfigFileChangeListener::notify_config_file_changed(&config_manager);
		main_widget.validate_render();
		add_paths_to_file_system_watcher(pref_file_watcher, default_config_path, user_config_paths);
		});

	QObject::connect(&key_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {
		input_handler.reload_config_files(default_keys_path, user_keys_paths);
		add_paths_to_file_system_watcher(key_file_watcher, default_keys_path, user_keys_paths);
		});

	if (SHOULD_CHECK_FOR_LATEST_VERSION_ON_STARTUP) {
		check_for_updates(&main_widget, APPLICATION_VERSION);
	}

	app.exec();

	quit = true;
	return 0;
}
