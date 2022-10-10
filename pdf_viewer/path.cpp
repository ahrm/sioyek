#include <qdir.h>
#include "path.h"
#include <qfileinfo.h>

Path::Path() : Path(L"")
{
}

Path::Path(std::wstring pathname)
{
	pathname = strip_string(pathname);
	canon_path = get_canonical_path(pathname);
}

Path Path::slash(const std::wstring& suffix) const
{
	std::wstring new_path = concatenate_path(get_path(), suffix);
	return Path(new_path);
}

std::optional<std::wstring> Path::filename() const
{
	std::vector<std::wstring> all_parts;
	parts(all_parts);
	if (all_parts.size() > 0) {
		return all_parts[all_parts.size() - 1];
	}
	return {};
}

Path Path::file_parent() const
{
	QFileInfo info(QString::fromStdWString(get_path()));
	return Path(info.dir().absolutePath().toStdWString());
}

std::wstring Path::get_path() const
{

	if (canon_path.size() == 0) return canon_path;

#ifdef Q_OS_WIN
	return canon_path;
#else
	if (canon_path[0] != '/'){
		return L"/" + canon_path;
	}
	else{
		return  canon_path;
	}
#endif
}

std::string Path::get_path_utf8() const
{
	return std::move(utf8_encode(get_path()));
}

void Path::create_directories()
{
	QDir().mkpath(QString::fromStdWString(canon_path));
}

//std::wstring Path::add_redundant_dot() const
//{
//	std::wstring file_name = filename().value();
//	return parent().get_path() + L"/./" + file_name;
//}

bool Path::dir_exists() const
{
	return QDir(QString::fromStdWString(canon_path)).exists();
}

bool Path::file_exists() const
{
	return QFile::exists(QString::fromStdWString(canon_path));
}

void Path::parts(std::vector<std::wstring>& res) const
{
	split_path(canon_path, res);
}

std::wostream& operator<<(std::wostream& stream, const Path& path) {
	stream << path.get_path();
	return stream;
}

void copy_file(Path src, Path dst)
{
	QFile::copy(QString::fromStdWString(src.get_path()), QString::fromStdWString(dst.get_path()));
}


//Path add_redundant_dot_to_path(const Path& sane_path) {
//
//	std::wstring file_name = sane_path.filename();
//
////#ifdef Q_OS_WIN
////	wchar_t separator = '\\';
////#else
////	wchar_t separator = '/';
////#endif
////
////	std::vector<std::wstring> parts;
////	split_path(sane_path, parts);
////	std::wstring res = L"";
////	for (int i = 0; i < parts.size(); i++) {
////		res = concatenate_path(res, parts[i]);
////	}
//
//	//QDir sane_path_dir(QString::fromStdWString(sane_path));
//	//sane_path_dir.filenam
//	//sane_path_dir.cdUp();
//
//	//return concatenate_path(concatenate_path(sane_path_dir.canonicalPath().toStdWString(), L"."), sa
//	//return sane_path.parent_path() / "." / sane_path.filename();
//}
