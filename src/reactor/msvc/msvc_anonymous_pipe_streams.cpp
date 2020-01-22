
// Copyright 2018 Victor Smirnov
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

#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/pipe_streams.hpp>
#include <memoria/core/memory/smart_ptrs.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

#include <Winbase.h>

namespace memoria {
namespace reactor {

class AnonymousPipeInputStreamImpl: public IPipeInputStream {

	HANDLE handle_;

    bool op_closed_{false};
    bool data_closed_{false};

public:
	AnonymousPipeInputStreamImpl(HANDLE handle): handle_(handle)
	{
    }

    virtual ~AnonymousPipeInputStreamImpl() noexcept {
        close();
    }

	

    virtual IOHandle handle() const {return (IOHandle)handle_;}

	virtual IOHandle detach() 
	{
		auto tmp = handle_;
		handle_ = INVALID_HANDLE_VALUE;
		op_closed_ = true;
		return tmp;
	}

	size_t read(uint8_t* data, size_t size)
	{
		DWORD actual_size{};
		DWORD error_code{};
		bool is_error{};

		std::tie(actual_size, error_code, is_error) = engine().run_in_thread_pool([&] {
			DWORD actual_read{};
			if (ReadFile(handle_, data, size, &actual_read, NULL)) 
			{
				return std::make_tuple(actual_read, (DWORD)0, false);
			}
			else {
				return std::make_tuple(actual_read, GetLastError(), true);
			}
		});

		if (is_error)
		{
			if (error_code == ERROR_BROKEN_PIPE) {
				data_closed_ = true;
			}
			else {
				MMA_THROW(SystemException()) << WhatInfo(GetErrorMessage(error_code));
			}
		}

		return actual_size;
	}

    virtual void close()
    {
		if (!op_closed_)
		{
			op_closed_ = true;

			if (!::CloseHandle(handle_)) {
				MMA_THROW(SystemException());
			}
		}
    }

    virtual bool is_closed() const {
        return op_closed_ || data_closed_;
    }
};

class AnonymousPipeOutputStreamImpl: public IPipeOutputStream {
	HANDLE handle_;

	bool op_closed_{ false };
	bool data_closed_{ false };

public:
	AnonymousPipeOutputStreamImpl(HANDLE handle): handle_(handle)
    {
		
    }

	

    virtual ~AnonymousPipeOutputStreamImpl() noexcept {
        close();
    }

    virtual IOHandle handle() const {return (IOHandle)handle_;}

	virtual IOHandle detach() 
	{
		auto tmp = handle_;
		handle_ = INVALID_HANDLE_VALUE;
		op_closed_ = true;
		return tmp;
	}

    virtual size_t write(const uint8_t* data, size_t size)
    {
        size_t total{};
        while (total < size) {
            total += write_(data + total, size - total);
        }
        return total;
    }

	size_t write_(const uint8_t* data, size_t size)
	{
		DWORD actual_size{};
		DWORD error_code{};
		bool is_error{};

		std::tie(actual_size, error_code, is_error) = engine().run_in_thread_pool([&] {
			DWORD actual_read{};
			if (WriteFile(handle_, data, size, &actual_read, NULL))
			{
				return std::make_tuple(actual_read, (DWORD)0, false);
			}
			else {
				return std::make_tuple(actual_read, GetLastError(), true);
			}
		});

		if (is_error)
		{
			if (error_code == ERROR_BROKEN_PIPE) {
				data_closed_ = true;
			}
			else {
				MMA_THROW(SystemException()) << WhatInfo(GetErrorMessage(error_code));
			}
		}

		return actual_size;
	}


    virtual void close()
    {
        if (!op_closed_)
        {
            op_closed_ = true;

			if (!::CloseHandle(handle_)) {
				MMA_THROW(SystemException());
			}
        }
    }

    virtual bool is_closed() const {
        return op_closed_ || data_closed_;
    }

    virtual void flush() {}
};




PipeStreams open_pipe()
{
	HANDLE h_read{};
	HANDLE h_write{};

	if (!CreatePipe(&h_read, &h_write, nullptr, 0)) 
	{
		MMA_THROW(SystemException()) << WhatCInfo("Can't create a pair of pipes");
	}

	return PipeStreams{
		PipeInputStream(MakeLocalShared<AnonymousPipeInputStreamImpl>(h_read)),
		PipeOutputStream(MakeLocalShared<AnonymousPipeOutputStreamImpl>(h_write)),
	};
}


}}
