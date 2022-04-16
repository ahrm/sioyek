#include "new_file_checker.h"

extern std::wstring PAPERS_FOLDER_PATH;

std::vector<QString> NewFileChecker::get_dir_files() {
	QDir dir(path);
	dir.setFilter(QDir::Files | QDir::NoSymLinks);
	dir.setSorting(QDir::Time);
	QFileInfoList list = dir.entryInfoList();
	std::vector<QString> res;
	for (int i = 0; i < list.size(); i++) {
		res.push_back(list.at(i).absoluteFilePath());
	}
	return res;
}

void NewFileChecker::update_files() {
	last_files.clear();
	last_files = get_dir_files();
}

QString NewFileChecker::get_lastest_new_file_path() {
	auto new_files = get_dir_files();

	for (auto new_file : new_files) {
		bool found = false;

		for (auto prev_file : last_files) {
			if (prev_file == new_file) {
				found = true;
				break;
			}
		}
		if (!found) {
			if (new_file.endsWith(".pdf")) {
				return new_file;
			}
		}
	}
	return "";
}

NewFileChecker::NewFileChecker(std::wstring dirpath, MainWidget* main_widget) {

	path = QString::fromStdWString(dirpath);
	if (dirpath.size() > 0) {
		update_files();
		paper_folder_watcher.addPath(QString::fromStdWString(PAPERS_FOLDER_PATH));
		QObject::connect(&paper_folder_watcher, &QFileSystemWatcher::directoryChanged, [&, main_widget](const QString& path) {
			auto new_path = get_lastest_new_file_path();
			if (new_path.size() > 0) {
				main_widget->on_new_paper_added(new_path.toStdWString());
			}
			update_files();
			});
	}
}
