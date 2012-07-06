
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_FILE_HPP_
#define MEMORIA_CORE_TOOLS_FILE_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/config.hpp>

#include <vector>

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

	static FileListType* readDir(const File& file);
protected:
	static String normalizePath(StringRef name);
};

}

#endif
