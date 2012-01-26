
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)




#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/strings.hpp>

#include <memoria/core/tools/file.hpp>


#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <unistd.h>


namespace memoria {

using namespace memoria::vapi;
using namespace std;

File::FileListType::~FileListType() throw() {
	try {
		for (UInt c = 0; c < size(); c++) {
			delete operator [](c);
		}
	}
	catch (...) {}
}

BigInt File::Size() const {
	struct stat buf;
	if (stat(path_.c_str(), &buf) == 0)
	{
		return buf.st_size;
	}
	else {
		throw FileException(MEMORIA_SOURCE, "Can't get file stats:" + String(strerror(errno)), path_);
	}
}

bool is_directory(StringRef name, bool throw_ex) {
	struct stat buf;
	if (stat(name.c_str(), &buf) == 0)
	{
		return S_ISDIR(buf.st_mode);
	}
	else if (throw_ex) {
		throw FileException(MEMORIA_SOURCE, "Can't get file stats: " + String(strerror(errno)), name);
	}
	else {
		return false;
	}
}

bool File::IsDirectory() const {
	return is_directory(path_, true);
}

bool File::IsExists() const {
	struct stat buf;
	if (stat(path_.c_str(), &buf) == 0) {
		return true;
	}
	else {
		return errno != ENOENT;
	}
}

//int getcwd(char* buf, size_t size) {
//	return 0;
//}
//
//int rmdir(const char* path) {
//	return 0;
//}

String File::GetAbsolutePath() const {
	if (path_[0] == '/')
	{
		return path_;
	}
	else {
		char buf[8192];
		if (getcwd(buf, sizeof(buf)))
		{
			return String(buf)+"/"+path_;
		}
		else {
			throw FileException(MEMORIA_SOURCE, "Can't get absolute path:" + String(strerror(errno)), path_);
		}
	}
}

bool mkdir(StringRef name) {
	struct stat buf;
	if (stat(name.c_str(), &buf) != 0)
	{
		return ::mkdir(name.c_str(), 0777) == 0;
	}
	else {
		return S_ISDIR(buf.st_mode);
	}
}

bool File::MkDir() const {
	return mkdir(path_);
}

bool File::MkDirs() const {

	typedef String::size_type SizeT;
	SizeT pos = path_[0] == '/' ? 1 : 0;

	while (pos != String::npos && pos < path_.length())
	{
		SizeT idx = path_.find("/", pos);
		if (idx != String::npos)
		{
			if (!mkdir(path_.substr(0, idx)))
			{
				return false;
			}
			pos = idx + 1;
		}
		else {
			if (!mkdir(path_))
			{
				return false;
			}
			else {
				break;
			}
		}
	}
	return true;
}

void File::Rename(StringRef new_name) {
	if (rename(path_.c_str(), new_name.c_str()) != 0)
	{
		throw FileException(MEMORIA_SOURCE, "Can't rename file: " + String(strerror(errno)), new_name);
	}
	path_ = new_name;
}

bool File::Delete() const {
	if (IsDirectory())
	{
		return rmdir(path_.c_str()) == 0;
	}
	else {
		return remove(path_.c_str()) == 0;
	}
}

bool rm(const File &file)
{
	if (file.IsDirectory())
	{
		File::FileListType* list = File::ReadDir(file);

		bool result = true;
		for (UInt c = 0; c < list->size(); c++)
		{
			File* entry = list->operator[](c);
			result = rm(*entry) && result;
		}

		// memory leak is possible if exception occurs
		delete list;

		return result && file.Delete();
	}
	else {
		return file.Delete();
	}
}


bool File::DelTree() const {
	return rm(*this);
}


StringRef File::GetPath() const {
	return path_;
}

String File::GetName() const {
	String::size_type idx = path_.find_last_of('/');
	if (idx == String::npos)
	{
		return path_;
	}
	else if (idx == path_.length() - 1){
		return "";
	}
	else {
		return path_.substr(idx + 1, path_.length() - idx - 1);
	}
}

File::FileListType* File::ReadDir(const File& file)
{
	if (file.IsDirectory())
	{
		FileListType* list = new FileListType();

		DIR *dp;
		struct dirent *ep;

		dp = opendir (file.GetPath().c_str());
		if (dp != NULL)
		{
			while ((ep = readdir (dp)))
			{
				String name(ep->d_name);
				if (name != "." && name != "..")
				{
					list->push_back(new File(file.GetPath() + "/" + name));
				}
			}

			closedir (dp);
		}
		else {
			throw FileException(MEMORIA_SOURCE, "Can't open directory:", strerror(errno));
		}

		return list;
	}
	else
	{
		throw FileException(MEMORIA_SOURCE, "File is not a directory", file.GetPath());
	}
}

inline String replace(String& text, StringRef from, StringRef to, bool& action)
{
	typedef String::size_type SizeT;
	action = false;
	for(SizeT index=0; index = text.find(from, index), index != String::npos;)
	{
		text.replace(index, from.length(), to);
		index += to.length();
		action = true;
	}
	return text;
}


String File::NormalizePath(StringRef path)
{
	if (path.find("/") == String::npos)
	{
		return path;
	}
	else if (IsEmpty(path)) {
		throw MemoriaException(MEMORIA_SOURCE, "Empty string is specified as a path");
	}
	else {
		typedef String::size_type SizeT;

		String buf = path;
		bool action = true;
		while (action)
		{
			buf = replace(buf, "//", "/", action);
		}

		SizeT start_nosp = buf.find_first_not_of(" ");
		SizeT start_slash = buf.find("/");
		if (start_nosp == start_slash && start_nosp != String::npos && start_nosp > 0)
		{
			return buf.substr(start_nosp, buf.length() - start_nosp);
		}
		else {
			return buf;
		}

		return buf;
	}
}

}

