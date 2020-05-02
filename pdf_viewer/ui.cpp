#include "ui.h"

bool select_pdf_file_name(wchar_t* out_file_name, int max_length) {

	cout << filesystem::current_path().string() << endl;
	OPENFILENAMEW ofn;
	ZeroMemory(out_file_name, max_length);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFilter = L"Pdf Files\0*.pdf\0Any File\0*.*\0";
	ofn.lpstrFile = out_file_name;
	ofn.nMaxFile = max_length;
	ofn.lpstrTitle = L"Select a document";
	ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;


	if (GetOpenFileNameW(&ofn)) {
		cout << filesystem::current_path().string() << endl;
		return true;
	}
	return false;
}

vector<ConfigFileChangeListener*> ConfigFileChangeListener::registered_listeners;

ConfigFileChangeListener::ConfigFileChangeListener() {
	cout << "config file change listener constructor called" << endl;
	registered_listeners.push_back(this);
}

ConfigFileChangeListener::~ConfigFileChangeListener() {
	cout << "config file change listener destructor called" << endl;
	registered_listeners.erase(std::find(registered_listeners.begin(), registered_listeners.end(), this));
}

void ConfigFileChangeListener::notify_config_file_changed(ConfigManager* new_config_manager) {
	for (auto* it : ConfigFileChangeListener::registered_listeners) {
		it->on_config_file_changed(new_config_manager);
	}
}
