
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/reactor/file.hpp>
#include <memoria/v1/core/memory/smart_ptrs.hpp>

#include <memoria/v1/core/tools/iostreams.hpp>

namespace memoria {
namespace v1 {
namespace reactor {

class FileImplBase {
protected:
    filesystem::path path_;
public:
    FileImplBase (filesystem::path file_path): path_(file_path) {}
    virtual ~FileImplBase() noexcept {}

    const filesystem::path& path() const {return path_;}
};



class GenericFile: public FileImplBase {
protected:
	HANDLE fd_{ INVALID_HANDLE_VALUE };
	bool no_buffering_;
	bool closed_{false};
public:

	GenericFile(filesystem::path file_path, FileFlags flags, FileMode mode, bool no_buffering_);
	virtual ~GenericFile() noexcept;

	uint64_t size() const {
		return filesystem::file_size(path_);
	}

	virtual void close();
	virtual bool is_closed() const { return closed_; }

	virtual size_t read(uint8_t* buffer, uint64_t offset, size_t size);
	virtual size_t write(const uint8_t* buffer, uint64_t offset, size_t size);	 
};


class BufferedFileImpl: public GenericFile, public IBinaryIOStream, public EnableSharedFromThis<BufferedFileImpl> {
	uint64_t pos_{};
public:
	using GenericFile::read;
	using GenericFile::write;

    BufferedFileImpl(filesystem::path file_path, FileFlags flags, FileMode mode):
		GenericFile(file_path, flags, mode, false)
	{}

	uint64_t seek(uint64_t position) {
		pos_ = position;
		return pos_;
	}

	uint64_t fpos() {
		return pos_;
	}

	virtual void close() {
		GenericFile::close();
	}
	virtual bool is_closed() const { return GenericFile::is_closed(); }

	virtual size_t read(uint8_t* buffer, size_t size) 
	{
		auto rr = GenericFile::read(buffer, pos_, size);
		pos_ += rr;
		return rr;
	}

	virtual size_t write(const uint8_t* buffer, size_t size) 
	{
		auto rr = GenericFile::write(buffer, pos_, size);
		pos_ += rr;
		return rr;	
	}
	
	virtual BinaryInputStream istream() 
    {
       	return BinaryInputStream(this->shared_from_this());
    }
    
	virtual BinaryOutputStream ostream() 
    {
       	return BinaryOutputStream(this->shared_from_this());
    }

	virtual void fsync();

    virtual void fdsync();

    virtual void flush() {
		fsync();
	}
};


class DMAFileImpl: public GenericFile {
public:
    DMAFileImpl(filesystem::path file_path, FileFlags flags, FileMode mode):
		GenericFile(file_path, flags, mode, true)
	{}

	uint64_t alignment() { return 512; }
	
	virtual size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error);
};



}}}
