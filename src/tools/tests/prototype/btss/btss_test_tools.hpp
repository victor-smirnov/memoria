
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/prototypes/bt_ss/btss_input.hpp>
#include <memoria/v1/core/tools/bignum/int64_codec.hpp>


namespace memoria {
namespace v1 {

template <typename CtrT, typename Data, LeafDataLengthType LeafDataLength = CtrT::Types::LeafDataLength> class BTSSTestInputProvider;


template <typename CtrT, typename Data>
class BTSSTestInputProvider<CtrT, Data, LeafDataLengthType::FIXED>: public bt::BufferProducer<DefaultIOBuffer> {

	using IOBuffer = DefaultIOBuffer;

	IOBuffer io_buffer_;

	using Value = typename Data::value_type;

	const Data& data_;

	size_t current_ = 0;

public:
	BTSSTestInputProvider(const Data& data, size_t buffer_size = 65536):
		io_buffer_(buffer_size),
		data_(data)
	{}

	virtual IOBuffer& buffer() {
		return io_buffer_;
	}

	virtual Int populate(IOBuffer& buffer)
	{
		Int total = 0;

		for (;current_ < data_.size(); current_++, total++)
		{
			auto pos = buffer.pos();
			if (!IOBufferAdapter<Value>::put(buffer, data_[current_]))
			{
				buffer.pos(pos);
				return total;
			}
		}

		return -total;
	}
};


template <typename CtrT, typename Data>
class BTSSTestInputProvider<CtrT, Data, LeafDataLengthType::VARIABLE>: public bt::BufferProducer<DefaultIOBuffer> {

	using IOBuffer = DefaultIOBuffer;

	IOBuffer io_buffer_;

	using Value = typename Data::value_type;

	const Data& data_;

	size_t current_ = 0;

public:
	BTSSTestInputProvider(const Data& data, size_t buffer_size = 65536):
		io_buffer_(buffer_size),
		data_(data)
	{}

	virtual IOBuffer& buffer() {
		return io_buffer_;
	}

	virtual Int populate(IOBuffer& buffer)
	{
		Int total = 0;

		for (;current_ < data_.size(); current_++, total++)
		{
			auto pos = buffer.pos();
			if (!buffer.putVLen(data_[current_]))
			{
				buffer.pos(pos);
				return total;
			}
		}

		return -total;
	}
};




}}
