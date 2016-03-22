
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>
#include <memoria/core/tools/strings/string.hpp>

#include <vector>
#include <memory>

namespace memoria{

class MEMORIA_API File {
    String path_;

public:
    class FileListType: public std::vector<File*> {
    public:
        FileListType(){}
        virtual ~FileListType() throw();
    };

    File(StringRef path);
    File(const File& file);

    virtual ~File() throw();

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

    RW = OpenMode::READ | OpenMode::WRITE,

    RWC = OpenMode::RW | OpenMode::CREATE,

    RWCT = OpenMode::RWC | OpenMode::TRUNC
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

}
