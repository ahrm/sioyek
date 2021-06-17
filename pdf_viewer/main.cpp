//todo: cleanup the code
//todo: visibility test is still buggy??
//todo: add fuzzy search
//todo: handle document memory leak (because documents are not deleted since adding state history)
//todo: tests!
//todo: handle right to left documents

//#include "imgui.h"
//#include "imgui_impl_sdl.h"
//#include "imgui_impl_opengl3.h"

#include <iostream>
#include <vector>
#include <string>
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

#include <Windows.h>
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


extern float background_color[3] = { 1.0f, 1.0f, 1.0f };
extern float ZOOM_INC_FACTOR = 1.2f;
extern float vertical_move_amount = 1.0f;
extern float horizontal_move_amount = 1.0f;
extern float move_screen_percentage = 0.8f;
extern const unsigned int cache_invalid_milies = 1000;
extern const int persist_milies = 1000 * 60;
extern const int page_paddings = 0;
extern const int max_pending_requests = 31;
extern bool launched_from_file_icon = false;
extern bool flat_table_of_contents = false;

extern filesystem::path last_path_file_absolute_location = "";
extern filesystem::path parent_path = "";

using namespace std;

mutex mupdf_mutexes[FZ_LOCK_MAX];

void lock_mutex(void* user, int lock) {
	mutex* mut = (mutex*)user;
	(mut + lock)->lock();
}

void unlock_mutex(void* user, int lock) {
	mutex* mut = (mutex*)user;
	(mut + lock)->unlock();
}


int main(int argc, char* args[]) {
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	QApplication app(argc, args);

	char exe_file_name[MAX_PATH];
	GetModuleFileNameA(NULL, exe_file_name, sizeof(exe_file_name));
	filesystem::path exe_path = exe_file_name;

	// parent path is the directory in which config files and required shaders are located
	// in debug mode this is the same directory as the source file but in release mode it is
	// the directory in which the executable is located
	parent_path = exe_path.parent_path();

#ifdef NDEBUG
	install_app(exe_file_name);
#else
	filesystem::path source_file_path = __FILE__;
	parent_path = source_file_path.parent_path();
#endif

	last_path_file_absolute_location = (parent_path / "last_document_path.txt").wstring();

	filesystem::path config_path = parent_path / "prefs.config";
	ConfigManager config_manager(config_path.wstring());

	sqlite3* db;
	char* error_message = nullptr;
	int rc;

	rc = sqlite3_open((parent_path / "test.db").string().c_str(), &db);
	if (rc) {
		cerr << "could not open database" << sqlite3_errmsg(db) << endl;
	}

	create_tables(db);

	fz_locks_context locks;
	locks.user = mupdf_mutexes;
	locks.lock = lock_mutex;
	locks.unlock = unlock_mutex;

	fz_context* mupdf_context = fz_new_context(nullptr, &locks, FZ_STORE_UNLIMITED);

	if (!mupdf_context) {
		cerr << "could not create mupdf context" << endl;
		return -1;
	}
	bool fail = false;
	fz_try(mupdf_context) {
		fz_register_document_handlers(mupdf_context);
	}
	fz_catch(mupdf_context) {
		cerr << "could not register document handlers" << endl;
		fail = true;
	}

	if (fail) {
		return -1;
	}

	bool quit = false;

	wstring keymap_path = (parent_path / "keys.config").wstring();
	InputHandler input_handler(keymap_path);

	//char file_path[MAX_PATH] = { 0 };
	wstring file_path;
	string file_path_;
	ifstream last_state_file(last_path_file_absolute_location);
	getline(last_state_file, file_path_);
	file_path = utf8_decode(file_path_);
	last_state_file.close();

	launched_from_file_icon = false;
	if (argc > 1) {
		//file_path = utf8_decode(args[1]);
		file_path = app.arguments().at(1).toStdWString();
		launched_from_file_icon = true;
	}

	bool is_waiting_for_symbol = false;
	const Command* current_pending_command = nullptr;

	DocumentManager document_manager(mupdf_context, db);

	QFileSystemWatcher pref_file_watcher;
	pref_file_watcher.addPath(QString::fromStdWString(config_path));

	QFileSystemWatcher key_file_watcher;
	key_file_watcher.addPath(QString::fromStdWString(keymap_path));




	QString font_path = QString::fromStdWString((parent_path / "fonts" / "monaco.ttf").wstring());
	if (QFontDatabase::addApplicationFont(font_path) < 0) {
		cout << "could not add font!" << endl;
	}


	QIcon icon(QString::fromStdWString((parent_path / "icon2.ico").wstring()));
	app.setWindowIcon(icon);

	MainWidget main_widget(mupdf_context, db, &document_manager, &config_manager, &input_handler, &quit);
	main_widget.open_document(file_path);
	main_widget.resize(500, 500);
	main_widget.showMaximized();

	// live reload the config file
	QObject::connect(&pref_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {
		wifstream config_file(config_path);
		config_manager.deserialize(config_file);
		config_file.close();
		ConfigFileChangeListener::notify_config_file_changed(&config_manager);
		main_widget.validate_render();
		});

	QObject::connect(&key_file_watcher, &QFileSystemWatcher::fileChanged, [&]() {
		input_handler.reload_config_file(keymap_path);
		});

	app.exec();

	quit = true;
	return 0;
}

