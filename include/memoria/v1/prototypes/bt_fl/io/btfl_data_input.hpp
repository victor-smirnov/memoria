
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/prototypes/bt/layouts/bt_input.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_input_tools.hpp>

#include <memory>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {



template <typename DataBuffer, typename CtrSizeT>
struct DataStreamInputBuffer {
	using Buffer    = DataBuffer;
protected:

	using BufferT 		= typename DataBuffer::PtrType;
	using BufferSizes 	= typename BufferT::BufferSizesT;

	using AppendState	= typename BufferT::AppendState;

	DataBuffer 	buffer_;
	BufferT*   	buffer_ptr_;
	AppendState append_state_;

public:
	DataStreamInputBuffer(){}

	void init(DataBuffer&& buffer)
	{
		buffer_ 	= std::move(buffer);
		buffer_ptr_ = buffer_.get();
	}

	void init(Int capacity)
	{
		init(create_input_buffer(capacity));
	}


	DataBuffer& buffer() {return buffer_;}
	const DataBuffer& buffer() const {return buffer_;}


	void reset()
	{
		if (!buffer_.is_null())
		{
			buffer_->reset();
			append_state_ =  buffer_->append_state();
		}
	}

	void finish()
	{
		buffer_->reindex();
	}


	BufferSizes data_capacity() const {
		return buffer_->data_capacity();
	}

	auto* create_input_buffer(const BufferSizes& buffer_sizes)
	{
		Int block_size  = BufferT::block_size(buffer_sizes);
		BufferT* buffer = T2T<BufferT*>(malloc(block_size));
		if (buffer)
		{
			buffer->setTopLevelAllocator();
			buffer->init(block_size, buffer_sizes);
			return buffer;
		}
		else {
			throw OOMException(MA_RAW_SRC);
		}
	}

	auto* create_input_buffer(Int buffer_size)
	{
		Int block_size  = BufferT::block_size(buffer_size) + 500;
		BufferT* buffer = T2T<BufferT*>(malloc(block_size));
		if (buffer)
		{
			buffer->setTopLevelAllocator();
			buffer->init(block_size, buffer_size);
			return buffer;
		}
		else {
			throw OOMException(MA_RAW_SRC);
		}
	}

	template <typename IOBuffer>
	void append_stream_entries(Int entries, IOBuffer& buffer)
	{
		for (Int c = 0; c < entries; c++)
		{
			this->append_io_entry(buffer);
		}
	}


	template <typename IOBuffer>
	void append_io_entry(IOBuffer& io_buffer, Int enlargements = 0)
	{
		size_t pos = io_buffer.pos();

		auto tmp = append_state_;

		if (this->buffer_ptr_->append_bttl_entry_from_iobuffer(append_state_, io_buffer))
		{
			return;
		}
		else {
			append_state_ = tmp;
			io_buffer.pos(pos);

			if (enlargements < 5)
			{
				this->enlarge();
				append_io_entry(io_buffer, enlargements + 1);
			}
			else {
				throw Exception(MA_RAW_SRC, "Supplied entry is too large for InputBuffer");
			}
		}
	}

protected:

	void enlarge()
	{
		BufferSizes current_capacity 	= data_capacity();
		BufferSizes new_capacity 		= current_capacity;
		VectorAdd(new_capacity, new_capacity);

		auto new_buffer = create_input_buffer(new_capacity);

		buffer_.get()->copyTo(new_buffer);

		init(DataBuffer(new_buffer));
	}
};








}}}}
