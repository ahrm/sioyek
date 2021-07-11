//todo: cleanup the code
//todo: handle document memory leak (because documents are not deleted since adding state history)
//todo: tests!
//todo: clean up parsing code
//todo: autocomplete in command window
//todo: simplify word selection logic (also avoid inefficient extra insertions followed by clears in selected_characters)
//todo: make it so that all commands that change document state (for example goto_offset_withing_page, goto_link, etc.) do not change the document
// state, instead they return a DocumentViewState object that is then applied using push_state and chnage_state functions
// (chnage state should be a function that just applies the state without pushing it to history)
//todo: add "repeat last command" command

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

#include <mupdf/fitz.h>
#include "sqlite3.h"
#include <filesystem>


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

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "fts_fuzzy_match.h"
#undef FTS_FUZZY_MATCH_IMPLEMENTATION

#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif


extern float BACKGROUND_COLOR[3] = { 1.0f, 1.0f, 1.0f };
extern float ZOOM_INC_FACTOR = 1.2f;
extern float VERTICAL_MOVE_AMOUNT = 1.0f;
extern float HORIZONTAL_MOVE_AMOUNT = 1.0f;
extern float VERTICAL_LINE_WIDTH = 0.1f;
extern float VERTICAL_LINE_FREQ = 0.001f;
extern float MOVE_SCREEN_PERCENTAGE = 0.8f;
extern const unsigned int CACHE_INVALID_MILIES = 1000;
extern const int PERSIST_MILIES = 1000 * 60;
extern const int PAGE_PADDINGS = 0;
extern const int MAX_PENDING_REQUESTS = 31;
extern bool LAUNCHED_FROM_FILE_ICON = false;
extern bool FLAT_TABLE_OF_CONTENTS = false;
extern bool SHOULD_USE_MULTIPLE_MONITORS = false;
extern std::wstring LIBGEN_ADDRESS = L"";
extern std::wstring GOOGLE_SCHOLAR_ADDRESS = L"";

extern std::filesystem::path last_path_file_absolute_location = "";
extern std::filesystem::path parent_path = "";


std::mutex mupdf_mutexes[FZ_LOCK_MAX];

void lock_mutex(void* user, int lock) {
	std::mutex* mut = (std::mutex*)user;
	(mut + lock)->lock();
}

void unlock_mutex(void* user, int lock) {
	std::mutex* mut = (std::mutex*)user;
	(mut + lock)->unlock();
}


int main(int argc, char* args[]) {

	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	QApplication app(argc, args);

	parent_path = QCoreApplication::applicationDirPath().toStdWString();
	std::string exe_path = utf8_encode(QCoreApplication::applicationFilePath().toStdWString());

#ifdef NDEBUG
	install_app(exe_path.c_str());
#else
	std::filesystem::path source_file_path = __FILE__;
	parent_path = source_file_path.parent_path();
#endif

	last_path_file_absolute_location = (parent_path / "last_document_path.txt").wstring();

	std::filesystem::path default_config_path = parent_path / "prefs.config";
	std::filesystem::path user_config_path = parent_path / "prefs_user.config";
	ConfigManager config_manager(default_config_path, user_config_path);

	sqlite3* db;
	char* error_message = nullptr;
	int rc;

	rc = sqlite3_open((parent_path / "test.db").string().c_str(), &db);
	if (rc) {
		std::cerr << "could not open database" << sqlite3_errmsg(db) << std::endl;
	}

	create_tables(db);

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

	std::filesystem::path keymap_default_path = parent_path / "keys.config";
	std::filesystem::path keymap_user_path = parent_path / "keys_user.config";
	InputHandler input_handler(keymap_default_path, keymap_user_path);

	//char file_path[MAX_PATH] = { 0 };
	std::wstring file_path;
	std::string file_path_;
	std::ifstream last_state_file(last_path_file_absolute_location);
	std::getline(last_state_file, file_path_);
	file_path = utf8_decode(file_path_);
	last_state_file.close();

	LAUNCHED_FROM_FILE_ICON = false;
	if (argc > 1) {
		file_path = app.arguments().at(1).toStdWString();
		LAUNCHED_FROM_FILE_ICON = true;
	}

	DocumentManager document_manager(mupdf_context, db);

	QFileSystemWatcher pref_file_watcher;
	pref_file_watcher.addPath(QString::fromStdWString(default_config_path.wstring()));
	pref_file_watcher.addPath(QString::fromStdWString(user_config_path.wstring()));


	QFileSystemWatcher key_file_watcher;
	key_file_watcher.addPath(QString::fromStdWString(keymap_default_path.wstring()));
	key_file_watcher.addPath(QString::fromStdWString(keymap_user_path.wstring()));


	QString font_path = QString::fromStdWString((parent_path / "fonts" / "monaco.ttf").wstring());
	if (QFontDatabase::addApplicationFont(font_path) < 0) {
		std::wcout << "could not add font!" << endl;
	}

	QIcon icon(QString::fromStdWString((parent_path / "icon2.ico").wstring()));
	app.setWindowIcon(icon);

	MainWidget main_widget(mupdf_context, db, &document_manager, &config_manager, &input_handler, &quit);
	main_widget.open_document(file_path);
	main_widget.resize(500, 500);
	main_widget.showMaximized();

	// live reload the config file
	QObject::connect(&pref_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {
		std::wifstream default_config_file(default_config_path);
		std::wifstream user_config_file(user_config_path);

		config_manager.deserialize(default_config_file, user_config_file);
		default_config_file.close();
		user_config_file.close();

		ConfigFileChangeListener::notify_config_file_changed(&config_manager);
		main_widget.validate_render();
		});

	QObject::connect(&key_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {
		input_handler.reload_config_files(keymap_default_path, keymap_user_path);
		});

	app.exec();

	quit = true;

	return 0;
}
