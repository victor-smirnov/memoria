
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

	virtual String GetName() const;
	virtual StringRef GetPath() const;

	virtual bool IsExists() const;
	virtual bool IsDirectory() const;
	String GetAbsolutePath() const;

	virtual bool MkDir() const;
	virtual bool MkDirs() const;
	virtual bool Delete() const;
	virtual bool DelTree() const;
	virtual BigInt Size() const;

	virtual void Rename(StringRef new_name);

	static FileListType* ReadDir(const File& file);
protected:
	static String NormalizePath(StringRef name);
};

}

#endif
