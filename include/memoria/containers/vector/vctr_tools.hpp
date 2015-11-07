
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_vctr_TOOLS_HPP
#define _MEMORIA_CONTAINERS_vctr_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/layouts/bt_input_buffer.hpp>



namespace memoria       {
namespace mvector       {




template <typename Position, typename T>
class AbstractInputBufferProvider: public bt::InputBufferProvider<Position, T> {

protected:
	Position start_;
	Position size_;

	bool next_ = true;

public:

	AbstractInputBufferProvider(): start_(0), size_(0)  {}
	AbstractInputBufferProvider(Position start, Position size): start_(start), size_(size)  {}
	AbstractInputBufferProvider(Position start, Position size, bool next): start_(start), size_(size), next_(next) {}

	virtual Position start() const {
		return start_;
	}

	virtual Position size()	const {
		return size_;
	}

	virtual Position zero()	const {return 0;}

	virtual void consumed(Position sizes) {
		start_ += sizes;
	}

	virtual bool isConsumed() {
		return start_ >= size_;
	}

	virtual void nextBuffer() {
		next_ = false;
	}

	virtual bool hasData() const
	{
		return next_ || start_ < size_;
	}
};





template <typename Buffer>
class VectorInputBufferAdaptor: public AbstractInputBufferProvider<Int, Buffer> {

	using Base = AbstractInputBufferProvider<Int, Buffer>;

public:
	using Position = typename Buffer::Position;

	using Struct = typename Buffer::template PackedStruct<IntList<0>>;

	using Value = typename Struct::Value;

private:
	Buffer* buffer_ = nullptr;

public:
	VectorInputBufferAdaptor(SizeT capacity): Base(0, 0)
	{
		Int block_size = Buffer::block_size(Position(capacity));

		buffer_ = T2T<Buffer*>(malloc(block_size));
		buffer_->init(block_size, Position(capacity));
	}

	~VectorInputBufferAdaptor()
	{
		if (buffer_) free(buffer_);
	}

	virtual void nextBuffer()
	{
		buffer_->reset();

		this->start_ = 0;

		this->next_ = this->fill(get());

		this->size_ = get()->size();
	}

	virtual const Buffer* buffer() const {
		return buffer_;
	}

	virtual bool fill(Struct* data) = 0;

protected:
	Struct* get() {
		return buffer_->template get<IntList<0>>();
	}

	const Struct* get() const {
		return buffer_->template get<IntList<0>>();
	}
};

template <typename Buffer>
class StdVectorInputBuffer: public VectorInputBufferAdaptor<Buffer> {

	using Base 		= VectorInputBufferAdaptor<Buffer>;
	using Value 	= typename Base::Value;
	using Struct	= typename Base::Struct;

	vector<Value>& data_;

	SizeT pos_ = 0;

public:
	StdVectorInputBuffer(vector<Value>& data, SizeT buffer_capacity = 4096):
		Base(buffer_capacity > (SizeT)data.size() ? data.size() : buffer_capacity),
		data_(data)
	{}

	virtual bool fill(Struct* data)
	{
		auto max = data->max_size();
		Int to_read = (max + pos_ < data_.size()) ? max : (data_.size() - pos_);

		data->insertSpace(0, to_read);

		CopyBuffer(&data_[pos_], data->values(), to_read);

		pos_ += to_read;

		return to_read == max;
	}
};


template <typename CtrT, typename InputIterator>
class IteratorVectorInputProvider: public memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
	using Base = memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:

	using Buffer 	= typename Base::Buffer;
	using CtrSizeT	= typename Base::CtrSizeT;
	using Position	= typename Base::Position;

	using Value = typename CtrT::Types::Value;

	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<0>;
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;


	InputIterator current_;
	InputIterator end_;

public:
	IteratorVectorInputProvider(CtrT& ctr, InputIterator begin, InputIterator end, Int capacity = 10000):
		Base(ctr, capacity),
		current_(begin),
		end_(end)
	{}

	virtual bool get(InputTuple& value)
	{
		if (current_ != end_)
		{
			value = InputTupleAdapter::convert(*current_);

			current_++;
			return true;
		}
		else {
			return false;
		}
	}
};

}
}

#endif
