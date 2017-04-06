
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.





#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/file.hpp>

#include <windows.h>

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <io.h>
#include <string.h>
#include <errno.h>
#include <memoria/v1/core/tools/strings/strings.hpp>
#include <stdio.h>
#include <sstream>
#include <iostream>




namespace memoria {
namespace v1 {


using namespace std;

String getErrorMsg(DWORD err_code)
{
    LPVOID lpMsgBuf;

    FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );

    String msg((const char*)lpMsgBuf);

    LocalFree(lpMsgBuf);

    return msg;
}

String getErrorMsg() {
    return getErrorMsg(GetLastError());
}


File::FileListType::~FileListType() noexcept {
    try {
        for (UInt c = 0; c < size(); c++) {
            delete operator [](c);
        }
    }
    catch (...) {}
}

BigInt File::size() const
{
    WIN32_FILE_ATTRIBUTE_DATA   fileInfo;
    bool fOk = GetFileAttributesEx(path_.c_str(), GetFileExInfoStandard, &fileInfo);

    if (fOk)
    {
        return (((BigInt)fileInfo.nFileSizeHigh) << 32) + fileInfo.nFileSizeLow;
    }
    else {
        throw FileException(MEMORIA_SOURCE, SBuf()<<"Can't get file stats: "<<getErrorMsg()<<" path="<<path_);
    }
}

bool is_directory(StringRef name, bool throw_ex)
{
    DWORD result = GetFileAttributes(name.c_str());

    if (result != INVALID_FILE_ATTRIBUTES)
    {
        return (result & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    else if (throw_ex) {
        throw FileException(MEMORIA_SOURCE, SBuf()<<"Can't get file attributes:"<<getErrorMsg()<<" path="<<name);
    }
    else {
        return false;
    }
}

bool File::isDirectory() const
{
    return is_directory(path_, true);
}

bool File::isExists() const
{
    return GetFileAttributes(path_.c_str()) != INVALID_FILE_ATTRIBUTES;
}

String File::getAbsolutePath() const
{
    if (path_[0] == '/')
    {
        return path_;
    }
    else {
        char buf[8192];
        if (GetCurrentDirectory(sizeof(buf) - 1, buf))
        {
            return String(buf)+"/"+path_;
        }
        else {
            throw FileException(MEMORIA_SOURCE, SBuf()<<"Can't get absolute path: "<<getErrorMsg()<<" path="<<path_);
        }
    }
}

bool mkdir(StringRef name)
{
    bool result = CreateDirectory(name.c_str(), NULL);
    if (result)
    {
        return true;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return File(name).isDirectory();
    }
    else {
        return false;
    }
}

bool File::mkDir() const
{
    return mkdir(path_);
}

bool File::mkDirs() const
{
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

void File::rename(StringRef new_name)
{
    if (!MoveFile(path_.c_str(), new_name.c_str()))
    {
        throw FileException(MEMORIA_SOURCE, SBuf()<<"Can't rename file: "<<getErrorMsg()<<", new name = "<<new_name);
    }
    path_ = new_name;
}

bool File::deleteFile() const
{
    if (isDirectory())
    {
        return RemoveDirectory(path_.c_str());
    }
    else {
        return DeleteFile(path_.c_str());
    }
}

bool rm(const File &file)
{
    if (file.isDirectory())
    {
        File::FileListType* list = File::readDir(file);

        bool result = true;
        for (UInt c = 0; c < list->size(); c++)
        {
            File* entry = list->operator[](c);
            result = rm(*entry) && result;
        }

        // memory leak is possible if exception occurs
        delete list;

        return result && file.deleteFile();
    }
    else {
        return file.deleteFile();
    }
}


bool File::delTree() const {
    return rm(*this);
}

StringRef File::getPath() const {
    return path_;
}

String File::getName() const {
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


void ThrowFE(const char* src, const File& file) {
    throw FileException(src, SBuf()<<"Can't read the directory: "<<getErrorMsg()<<" path="<<file.getPath());
}

File::FileListType* File::readDir(const File& file)
{
    if (file.isDirectory())
    {
        FileListType* list = new FileListType();

        WIN32_FIND_DATA fdata;
        HANDLE dhandle;

        // must append \* to the path
        {
            char buf[8192];
            sprintf_s(buf, sizeof(buf), "%s\\*", file.getAbsolutePath().c_str());
            if((dhandle = FindFirstFile(buf, &fdata)) == INVALID_HANDLE_VALUE)
            {
                delete list;
                ThrowFE(MEMORIA_SOURCE, file);
            }
        }

        // even an "empty" directory will give two results - . and ..
        String str(fdata.cFileName);

        if (str != "." && str != "..")
        {
            list->push_back(new File(str));
        }

        while(1)
        {
            if(FindNextFile(dhandle, &fdata))
            {
                String st(fdata.cFileName);
                if (st != "." && st != "..")
                {
                    list->push_back(new File(st));
                }
            }
            else {
                if(GetLastError() == ERROR_NO_MORE_FILES)
                {
                    break;
                }
                else {
                    FindClose(dhandle);
                    delete list;
                    ThrowFE(MEMORIA_SOURCE, file);
                }
            }
        }

        if(FindClose(dhandle) == 0)
        {
            delete list;
            ThrowFE(MEMORIA_SOURCE, file);
        }

		return list;
    }
    else
    {
        throw FileException(MEMORIA_SOURCE, SBuf()<<"File is not a directory: "<<file.getPath());
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


String File::normalizePath(StringRef path)
{
    if (path.find("/") == String::npos)
    {
        return path;
    }
    else if (isEmpty(path)) {
        throw Exception(MEMORIA_SOURCE, "Empty string is specified as a path");
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

String ConvertSlash(StringRef str) {
    String result = str;
    bool action;
    return replace(result, "/", "\\", action);
}

void File::copy(StringRef new_file)
{
	/*RAFile src_file;
	RAFile tgt_file;

	src_file.open(getPath().c_str(), OpenMode::READ);
	tgt_file.open(new_file.c_str(), OpenMode::READ | OpenMode::WRITE | OpenMode::CREATE | OpenMode::TRUNC);

	char buffer[4096];

	UBigInt size;
	while ((size = src_file.read(buffer, sizeof(buffer))))
	{
		tgt_file.write(buffer, size);
	}*/
}


}}