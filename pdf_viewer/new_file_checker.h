#pragma once

#include "main_widget.h"

#include <vector>
#include <qstring.h>
#include <qfilesystemwatcher.h>
#include <qdir.h>

class NewFileChecker {
private:
	std::vector<QString> last_files;
	QString path;
	QFileSystemWatcher paper_folder_watcher;

public:
	std::vector<QString> get_dir_files();
	void update_files();
	QString get_lastest_new_file_path();
	NewFileChecker(std::wstring dirpath, MainWidget* main_widget);
};
