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

bool HierarchialSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
	//static int num_calls = 0;
	//num_calls++;
	//cout << num_calls << endl;
	// custom behaviour :
	if (filterRegExp().isEmpty() == false)
	{
		// get source-model index for current row
		QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent);
		if (source_index.isValid())
		{
			// check current index itself :
			QString key = sourceModel()->data(source_index, filterRole()).toString();
			bool parent_contains = key.contains(filterRegExp());

			if (parent_contains) return true;

			// if any of children matches the filter, then current index matches the filter as well
			int i, nb = sourceModel()->rowCount(source_index);
			for (i = 0; i < nb; ++i)
			{
				if (filterAcceptsRow(i, source_index))
				{
					return true;
				}
			}
			return false;
		}
	}
	// parent call for initial behaviour
	return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}
