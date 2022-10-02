#include <qfile.h>

#include "checksum.h"

std::string compute_checksum(const QString &file_name, QCryptographicHash::Algorithm hash_algorithm)
{
    QFile infile(file_name);
    qint64 file_size = infile.size();
    const qint64 buffer_size = 10240;

    if (infile.open(QIODevice::ReadOnly))
    {
        char buffer[buffer_size];
        int bytes_read;
        int read_size = qMin(file_size, buffer_size);

        QCryptographicHash hash(hash_algorithm);
        while (read_size > 0 && (bytes_read = infile.read(buffer, read_size)) > 0) 
        {
            file_size -= bytes_read;
            hash.addData(buffer, bytes_read);
            read_size = qMin(file_size, buffer_size);
        }

        infile.close();
        return QString(hash.result().toHex()).toStdString();
    }
	return "";
}

CachedChecksummer::CachedChecksummer(const std::vector<std::pair<std::wstring, std::wstring>>* loaded_checksums){
    if (loaded_checksums) {
		for (const auto& [path, checksum_] : *loaded_checksums) {
			std::string checksum = QString::fromStdWString(checksum_).toStdString();
			cached_checksums[path] = checksum;
			cached_paths[checksum].push_back(path);
		}
    }
}

std::optional<std::string> CachedChecksummer::get_checksum_fast(std::wstring file_path) {
    // return the checksum only if it is alreay precomputed in cache
    if (cached_checksums.find(file_path) != cached_checksums.end()) {
        return cached_checksums[file_path];
    }
    return {};
}

std::string CachedChecksummer::get_checksum(std::wstring file_path) {

		auto cached_checksum = get_checksum_fast(file_path);

		if (!cached_checksum) {
			std::string checksum = compute_checksum(QString::fromStdWString(file_path), QCryptographicHash::Md5);
			cached_checksums[file_path] = checksum;
            cached_paths[checksum].push_back(file_path);
		}
		return cached_checksums[file_path];

}

std::optional<std::wstring> CachedChecksummer::get_path(std::string checksum) {
    const std::vector<std::wstring> paths = cached_paths[checksum];

    for (const auto& path_string : paths) {
        if (QFile::exists(QString::fromStdWString(path_string))) {
            return path_string;
        }
    }
    return {};
}