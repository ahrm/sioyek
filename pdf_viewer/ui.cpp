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
