
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


#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/config.hpp>
#include <memoria/v1/core/tools/strings/string.hpp>

#include <vector>
#include <memory>

namespace memoria {
namespace v1 {

class File {
    String path_;

public:
    class FileListType: public std::vector<File*> {
    public:
        FileListType(){}
        virtual ~FileListType() noexcept;
    };

    File(StringRef path);
    File(const File& file);

    virtual ~File() noexcept;

    virtual String getName() const;
    virtual StringRef getPath() const;

    virtual bool isExists() const;
    virtual bool isDirectory() const;
    String getAbsolutePath() const;

    virtual bool mkDir() const;
    virtual bool mkDirs() const;
    virtual bool deleteFile() const;
    virtual bool delTree() const;
    virtual BigInt size() const;

    virtual void rename(StringRef new_name);

    virtual void copy(StringRef new_file);

    static FileListType* readDir(const File& file);
protected:
    static String normalizePath(StringRef name);
};


enum class OpenMode: Int {
    READ = 1, WRITE = 2, CREATE = 4, TRUNC = 8,

    RW = READ | WRITE,

    RWC = RW | CREATE,

    RWCT = RWC | TRUNC
};



inline constexpr OpenMode operator&(OpenMode m1, OpenMode m2)
{
    return static_cast<OpenMode>(static_cast<Int>(m1) & static_cast<Int>(m2));
}

inline constexpr OpenMode operator|(OpenMode m1, OpenMode m2)
{
    return static_cast<OpenMode>(static_cast<Int>(m1) | static_cast<Int>(m2));
}

inline constexpr bool to_bool(OpenMode mode) {
    return static_cast<Int>(mode) > 0;
}

enum class SeekType: Int {
    SET = 1, CUR = 2, END = 4
};


struct IRandomAccessFile {

    virtual ~IRandomAccessFile() {}

    virtual void open(const char* name, OpenMode mode)                          = 0;
    virtual void close()                                                        = 0;

    virtual UBigInt seek(UBigInt pos, SeekType where)                           = 0;
    virtual UBigInt read(void* buf, UBigInt size)                               = 0;
    virtual void readAll(void* buf, UBigInt size)                               = 0;
    virtual void write(const void* buf, UBigInt size)                           = 0;
    virtual void truncate(UBigInt size)                                         = 0;

    virtual void sync()                                                         = 0;
};


struct RAFileImpl;

class RAFile: public IRandomAccessFile {

    RAFileImpl* pimpl_;
    bool closed_ = true;

public:

    RAFile();

    virtual ~RAFile();

    virtual void open(const char* name, OpenMode mode);
    virtual void close();

    virtual UBigInt seek(UBigInt pos, SeekType where);
    virtual UBigInt read(void* buf, UBigInt size);
    virtual void readAll(void* buf, UBigInt size);
    virtual void write(const void* buf, UBigInt size);
    virtual void truncate(UBigInt size);
    virtual void sync();
};

}}
