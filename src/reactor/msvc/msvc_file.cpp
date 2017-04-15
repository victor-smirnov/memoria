
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



#include <memoria/v1/reactor/msvc/msvc_file.hpp>
#include <memoria/v1/reactor/msvc/msvc_io_poller.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/message/fiber_io_message.hpp>

#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/bzero_struct.hpp>
#include <memoria/v1/core/tools/perror.hpp>
#include <memoria/v1/core/tools/strings/string_buffer.hpp>



#include <stdio.h>

#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <exception>
#include <tuple>

#include <malloc.h>


namespace memoria {
namespace v1 {
namespace reactor {


DWORD get_access(FileFlags flags, FileMode mode) {

	DWORD access{};

	if (to_bool(flags & FileFlags::RDONLY)) access |= GENERIC_READ;
	if (to_bool(flags & FileFlags::WRONLY)) access |= GENERIC_WRITE;

	if (to_bool(flags & FileFlags::RDWR)) access |= (GENERIC_WRITE | GENERIC_READ);

	return access;
}

DWORD get_disposition(FileFlags flags, FileMode mode) {

	DWORD disposition{};

	if (to_bool(flags & FileFlags::TRUNCATE)) {
		disposition = TRUNCATE_EXISTING;
	}
	else if (to_bool(flags & FileFlags::CREATE)) 
	{
		if (to_bool(flags & FileFlags::EXCL)) {
			disposition = CREATE_NEW;
		}
		else {
			disposition = OPEN_ALWAYS;
		}
	}
	
	return disposition;
}

DWORD get_share_mode(FileFlags flags, FileMode mode) {
	return FILE_SHARE_READ;
}

DWORD get_flags_and_attributes(FileFlags flags, FileMode mode, bool no_buffering) 
{
	DWORD file_flags { FILE_ATTRIBUTE_NORMAL };

	if (to_bool(flags & FileFlags::CLOEXEC)) file_flags |= FILE_FLAG_DELETE_ON_CLOSE;

	file_flags |= FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS ;

	if (no_buffering) {
		file_flags |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
	}

	return file_flags;
}


class GenericFile: public File {
	HANDLE fd_{ INVALID_HANDLE_VALUE };
	bool no_buffering_;
	bool closed_{false};
public:
	GenericFile(filesystem::path file_path, FileFlags flags, FileMode mode, bool no_buffering_);
	virtual ~GenericFile() noexcept;

	virtual uint64_t alignment() { return no_buffering_ ? 512 : 1; }

	virtual void close();

	virtual uint64_t read(char* buffer, uint64_t offset, uint64_t size);
	virtual uint64_t write(const char* buffer, uint64_t offset, uint64_t size);

	virtual size_t process_batch(IOBatchBase& batch, bool rise_ex_on_error = true);

	virtual void fsync();
	virtual void fdsync();
};

GenericFile::GenericFile(filesystem::path file_path, FileFlags flags, FileMode mode, bool no_buffering):
    File(file_path), no_buffering_(no_buffering)
{
	DWORD creation_disposition = get_disposition(flags, mode);

	DWORD last_error{};

	std::tie(fd_, last_error) = engine().run_in_thread_pool([&] {
		return std::make_tuple(
			CreateFile(
				file_path.string().c_str(),
				get_access(flags, mode),
				get_share_mode(flags, mode),
				NULL,
				creation_disposition,
				get_flags_and_attributes(flags, mode, no_buffering_),
				NULL
			), 
			GetLastError()
		);
	});


	if (fd_ == INVALID_HANDLE_VALUE) 
	{
		rise_win_error(
			SBuf() << "Can't open/create file " << file_path,
			GetLastError()
		);
	}
	else {
		Reactor& r = engine();
		CreateIoCompletionPort(fd_, r.io_poller().completion_port(), (u_long)0, 0);
	}
}

GenericFile::~GenericFile() noexcept {
	if (!closed_) {
		CloseHandle(fd_);
	}
}
    
void GenericFile::close()
{
	if (fd_ != INVALID_HANDLE_VALUE && !CloseHandle(fd_)) 
	{
		rise_win_error(SBuf() << "Can't close file ", GetLastError());
	}

	closed_ = true;
}



uint64_t GenericFile::read(uint8_t* buffer, uint64_t offset, uint64_t size)
{    
	Reactor& r = engine();

	AIOMessage message(r.cpu());

	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;
	overlapped.Offset = (DWORD)offset;
	overlapped.OffsetHigh = (DWORD)(offset >> 32);
	
	while (true) {
		bool read_result = ::ReadFile(fd_, buffer, (DWORD)size, nullptr, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();
			
			if (overlapped.status_) {
				return overlapped.size_;
			}
			else {
				rise_win_error(
					SBuf() << "Error reading from file " << path_,
					overlapped.error_code_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else {
			rise_win_error(SBuf() << "Error starting read from file " << path_, error_code);
		}
	}
}




uint64_t GenericFile::write(const uint8_t* buffer, uint64_t offset, uint64_t size)
{    
	Reactor& r = engine();

	AIOMessage message(r.cpu());

	auto overlapped = tools::make_zeroed<OVERLAPPEDMsg>();
	overlapped.msg_ = &message;
	overlapped.Offset = (DWORD)offset;
	overlapped.OffsetHigh = (DWORD)(offset >> 32);
	
	while (true) 
	{
		bool read_result = ::WriteFile(fd_, buffer, (DWORD)size, nullptr, &overlapped);

		DWORD error_code = GetLastError();

		if (read_result || error_code == ERROR_IO_PENDING)
		{
			message.wait_for();
			
			if (overlapped.status_) {
				return overlapped.size_;
			}
			else {
				rise_win_error(
					SBuf() << "Error writing to file " << path_,
					overlapped.error_code_
				);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else {
			rise_win_error(
				SBuf() << "Error starting write to file " << path_, error_code
			);
		}
	}
}


size_t GenericFile::process_batch(IOBatchBase& batch, bool rise_ex_on_error)
{
	Reactor& r = engine();

	AIOMessage message(r.cpu());

	batch.configure(&message);

	while (true)
	{
		size_t wait_num = 0;

		for (size_t c = 0; c < batch.nblocks(); c++, wait_num++) 
		{
			auto* ovl = batch.block(c);

			bool op_result;

			while (true)
			{
				if (ovl->operation_ == OVERLAPPEDMsg::WRITE)
				{
					op_result = ::WriteFile(fd_, ovl->data_, (DWORD)ovl->requested_size_, nullptr, ovl);
				}
				else {
					op_result = ::ReadFile(fd_, ovl->data_, (DWORD)ovl->requested_size_, nullptr, ovl);
				}

				DWORD error_code = GetLastError();

				if (op_result || error_code == ERROR_IO_PENDING) {
					// just continue batch submition
					break;
				}
				else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
					message.wait_for(); // jsut sleep and wait for required resources to appear
				}
				else 
				{
					// Wait for submited events to complete
					if (wait_num > 0) {
						message.set_count(wait_num);
						message.wait_for();
					}
					
					batch.set_submited(wait_num);

					if (rise_ex_on_error) 
					{
						rise_win_error(
							SBuf() << "Error submiting AIO " << (ovl->operation_ == OVERLAPPEDMsg::WRITE ? "write" : "read")
							<< " operation number " << wait_num
							<< " to " << path_,
							error_code
						);
					}
					else {						
						return wait_num;
					}
				}
			}
		}
		
		message.set_count(wait_num);
		message.wait_for();

		batch.set_submited(wait_num);

		return wait_num;
	}
}








void GenericFile::fsync()
{
	bool status;
	DWORD last_error{};

	std::tie(status, last_error) = engine().run_in_thread_pool([&] {
		return std::make_tuple(
			FlushFileBuffers(fd_),
			GetLastError()
		);
	});

	if (!status) 
	{
		rise_win_error(
			SBuf() << "Can't sync file " << path_,
			last_error
		);
	}
}

void GenericFile::fdsync()
{
	fsync();
}


DMABuffer allocate_dma_buffer(size_t size) 
{
	if (size != 0) 
	{
		void* ptr = _aligned_malloc(size, 512);

		if (ptr) {
			DMABuffer buf(tools::ptr_cast<char>(ptr));
			return buf;
		}
		else {
			tools::rise_perror(SBuf() << "Cant allocate dma buffer of " << size << " bytes");
		}
	}
	else {
		tools::rise_error(SBuf() << "Cant allocate dma buffer of 0 bytes");
	}
}

std::shared_ptr<File> open_dma_file(filesystem::path file_path, FileFlags flags, FileMode mode)
{
	return std::static_pointer_cast<File>(std::make_shared<GenericFile>(file_path, flags, mode, true));
}

std::shared_ptr<File> open_buffered_file(filesystem::path file_path, FileFlags flags, FileMode mode)
{
	return std::static_pointer_cast<File>(std::make_shared<GenericFile>(file_path, flags, mode, true));
}

    
}}}
