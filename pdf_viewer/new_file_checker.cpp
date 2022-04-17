#include "new_file_checker.h"

extern std::wstring PAPERS_FOLDER_PATH;

void NewFileChecker::get_dir_files_helper(QString parent_path, std::vector<QString>& paths) {

	QDir parent(parent_path);
	parent.setFilter(QDir::Files | QDir::NoSymLinks);
	parent.setSorting(QDir::Time);
	QFileInfoList list = parent.entryInfoList();

	for (int i = 0; i < list.size(); i++) {
		paths.push_back(list.at(i).absoluteFilePath());
	}

	parent.setFilter(QDir::Dirs | QDir::NoSymLinks);
	QFileInfoList dirlist = parent.entryInfoList();
	for (auto dir : dirlist) {
		if (dir.fileName() == "." || dir.fileName() == "..") {
			continue;
		}
		get_dir_files_helper(dir.absoluteFilePath(), paths);
	}

}

std::vector<QString> NewFileChecker::get_dir_files() {

	std::vector<QString> res;
	get_dir_files_helper(path, res);
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

void NewFileChecker::register_subdirectories(QString dirpath) {
	paper_folder_watcher.addPath(dirpath);

	QDir parent(dirpath);
	parent.setFilter(QDir::Dirs | QDir::NoSymLinks);
	QFileInfoList dirlist = parent.entryInfoList();
	for (auto dir : dirlist) {
		if (dir.fileName() == "." || dir.fileName() == "..") {
			continue;
		}
		register_subdirectories(dir.absoluteFilePath());
	}
}

NewFileChecker::NewFileChecker(std::wstring dirpath, MainWidget* main_widget) {

	path = QString::fromStdWString(dirpath);
	if (dirpath.size() > 0) {
		update_files();
		register_subdirectories(QString::fromStdWString(PAPERS_FOLDER_PATH));
		QObject::connect(&paper_folder_watcher, &QFileSystemWatcher::directoryChanged, [&, main_widget](const QString& path) {
			auto new_path = get_lastest_new_file_path();
			if (new_path.size() > 0) {
				main_widget->on_new_paper_added(new_path.toStdWString());
			}
			update_files();
			});
	}
}
