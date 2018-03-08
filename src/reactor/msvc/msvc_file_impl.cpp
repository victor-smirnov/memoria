
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
#include <memoria/v1/core/tools/iostreams.hpp>
#include <memoria/v1/core/strings/string_buffer.hpp>

#include <memoria/v1/reactor/file_streams.hpp>


#include <memoria/v1/reactor/reactor.hpp>

#include "msvc_file_impl.hpp"

#include <stdio.h>

#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <exception>
#include <tuple>
#include <memory>

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
	
	if (to_bool(flags & FileFlags::CREATE)) 
	{
		if (to_bool(flags & FileFlags::EXCL)) {
			disposition = CREATE_NEW;
		}
		else {
			disposition = OPEN_ALWAYS;
		}
	}
	else {
		disposition |= OPEN_EXISTING;
	}
	
	return disposition;
}

DWORD get_share_mode(FileFlags flags, FileMode mode) {
	return FILE_SHARE_READ;
}

DWORD get_flags_and_attributes(FileFlags flags, FileMode mode, bool no_buffering) 
{
	DWORD file_flags { FILE_ATTRIBUTE_NORMAL };

	if (to_bool(flags & FileFlags::CLOEXEC)) {
		file_flags |= FILE_FLAG_DELETE_ON_CLOSE;
	}

	file_flags |= FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS ;

	if (no_buffering) {
		file_flags |= FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH;
	}

	return file_flags;
}



GenericFile::GenericFile(filesystem::path file_path, FileFlags flags, FileMode mode, bool no_buffering):
    FileImplBase(file_path), no_buffering_(no_buffering)
{
	DWORD creation_disposition = get_disposition(flags, mode);

	DWORD last_error{};

	std::tie(fd_, last_error) = engine().run_in_thread_pool([&] {

		auto handle = CreateFileW(
			file_path.std_wstring().c_str(),
			get_access(flags, mode),
			get_share_mode(flags, mode),
			nullptr,
			creation_disposition,
			get_flags_and_attributes(flags, mode, no_buffering_),
			nullptr
		);

		return std::make_tuple(
			handle, 
			GetLastError()
		);
	});


	if (fd_ == INVALID_HANDLE_VALUE) 
	{
		MMA1_THROW(SystemException(last_error)) << fmt::format_ex(u"Can't open/create file {}", file_path);
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
		MMA1_THROW(SystemException()) << fmt::format_ex(u"Can't close file {}", path_);
	}

	closed_ = true;
}



size_t GenericFile::read(uint8_t* buffer, uint64_t offset, size_t size)
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
			else if (overlapped.error_code_ == ERROR_HANDLE_EOF) {
				return 0;
			}
			else {
				MMA1_THROW(SystemException(overlapped.error_code_)) << fmt::format_ex(u"Error reading from file {}", path_);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else {
			MMA1_THROW(SystemException(error_code)) << fmt::format_ex(u"Error starting read from file {}", path_);
		}
	}
}




size_t GenericFile::write(const uint8_t* buffer, uint64_t offset, size_t size)
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
				MMA1_THROW(SystemException(overlapped.error_code_)) << fmt::format_ex(u"Error writing to file {}", path_);
			}
		}
		else if (error_code == ERROR_INVALID_USER_BUFFER || error_code == ERROR_NOT_ENOUGH_MEMORY) {
			message.wait_for(); // jsut sleep and wait for required resources to appear
		}
		else {
			MMA1_THROW(SystemException(error_code)) << fmt::format_ex(u"Error starting write to file {}", path_);
		}
	}
}


size_t DMAFileImpl::process_batch(IOBatchBase& batch, bool rise_ex_on_error)
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
						MMA1_THROW(SystemException(error_code)) << fmt::format_ex(
							u"Error submiting AIO {} operation number {} to {}", 
							(ovl->operation_ == OVERLAPPEDMsg::WRITE ? u"write" : u"read"),
							wait_num,
							path_
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








void BufferedFileImpl::fsync()
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
			MMA1_THROW(SystemException(last_error)) << fmt::format_ex(u"Can't sync file {}", path_);
		}
}

void BufferedFileImpl::fdsync()
{
	fsync();
}



File open_buffered_file(filesystem::path file_path, FileFlags flags, FileMode mode)
{
	return MakeLocalShared<BufferedFileImpl>(file_path, flags, mode);
}

DMAFile open_dma_file(filesystem::path file_path, FileFlags flags, FileMode mode)
{
	return MakeLocalShared<DMAFileImpl>(file_path, flags, mode);
}




DMABuffer allocate_dma_buffer(size_t size)
{
	if (size != 0)
	{
		void* ptr = _aligned_malloc(size, 512);

		if (ptr) {
			DMABuffer buf(tools::ptr_cast<uint8_t>(ptr));
			return buf;
		}
		else {
			MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't allocate dma buffer of {} bytes", size);
		}
	}
	else {
		MMA1_THROW(RuntimeException()) << WhatCInfo("Can't allocate dma buffer of 0 bytes");
	}
}


}}}
