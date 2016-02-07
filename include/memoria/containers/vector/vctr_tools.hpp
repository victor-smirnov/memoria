
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_vctr_TOOLS_HPP
#define _MEMORIA_CONTAINERS_vctr_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/layouts/bt_input_buffer.hpp>





namespace memoria       {
namespace mvector       {




//template <typename Position, typename T>
//class AbstractInputBufferProvider: public bt::InputBufferProvider<Position, T> {
//
//protected:
//	Position start_;
//	Position size_;
//
//	bool next_ = true;
//
//public:
//
//	AbstractInputBufferProvider(): start_(0), size_(0)  {}
//	AbstractInputBufferProvider(Position start, Position size): start_(start), size_(size)  {}
//	AbstractInputBufferProvider(Position start, Position size, bool next): start_(start), size_(size), next_(next) {}
//
//	virtual Position start() const {
//		return start_;
//	}
//
//	virtual Position size()	const {
//		return size_;
//	}
//
//	virtual Position zero()	const {return 0;}
//
//	virtual void consumed(Position sizes) {
//		start_ += sizes;
//	}
//
//	virtual bool isConsumed() {
//		return start_ >= size_;
//	}
//
//	virtual void nextBuffer() {
//		next_ = false;
//	}
//
//	virtual bool hasData() const
//	{
//		return next_ || start_ < size_;
//	}
//};





//template <typename Buffer>
//class VectorInputBufferAdaptor: public AbstractInputBufferProvider<Int, Buffer> {
//
//	using Base = AbstractInputBufferProvider<Int, Buffer>;
//
//public:
//	using Position = typename Buffer::Position;
//
//	using Struct = typename Buffer::template PackedStruct<IntList<0>>;
//
//	using Value = typename Struct::Value;
//
//private:
//	Buffer* buffer_ = nullptr;
//
//public:
//	VectorInputBufferAdaptor(SizeT capacity): Base(0, 0)
//	{
//		Int block_size = Buffer::block_size(Position(capacity));
//
//		buffer_ = T2T<Buffer*>(malloc(block_size));
//		buffer_->init(block_size, Position(capacity));
//	}
//
//	~VectorInputBufferAdaptor()
//	{
//		if (buffer_) free(buffer_);
//	}
//
//	virtual void nextBuffer()
//	{
//		buffer_->reset();
//
//		this->start_ = 0;
//
//		this->next_ = this->fill(get());
//
//		this->size_ = get()->size();
//	}
//
//	virtual const Buffer* buffer() const {
//		return buffer_;
//	}
//
//	virtual bool fill(Struct* data) = 0;
//
//protected:
//	Struct* get() {
//		return buffer_->template get<IntList<0>>();
//	}
//
//	const Struct* get() const {
//		return buffer_->template get<IntList<0>>();
//	}
//};
//
//template <typename Buffer>
//class StdVectorInputBuffer: public VectorInputBufferAdaptor<Buffer> {
//
//	using Base 		= VectorInputBufferAdaptor<Buffer>;
//	using Value 	= typename Base::Value;
//	using Struct	= typename Base::Struct;
//
//	vector<Value>& data_;
//
//	SizeT pos_ = 0;
//
//public:
//	StdVectorInputBuffer(vector<Value>& data, SizeT buffer_capacity = 4096):
//		Base(buffer_capacity > (SizeT)data.size() ? data.size() : buffer_capacity),
//		data_(data)
//	{}
//
//	virtual bool fill(Struct* data)
//	{
//		auto max = data->max_size();
//		Int to_read = (max + pos_ < data_.size()) ? max : (data_.size() - pos_);
//
//		data->insertSpace(0, to_read);
//
//		CopyBuffer(&data_[pos_], data->values(), to_read);
//
//		pos_ += to_read;
//
//		return to_read == max;
//	}
//};


template <typename CtrT, typename InputIterator>
class IteratorVectorInputProvider2: public memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {

	using Base = memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:
	using CtrSizeT	= typename Base::CtrSizeT;
	using Position	= typename Base::Position;

	using Value = typename CtrT::Types::Value;

	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<0>;
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;


	InputIterator current_;
	const BigInt length_;
	BigInt processed_ = 0;

public:
	IteratorVectorInputProvider2(CtrT& ctr, InputIterator start, BigInt length, Int capacity = 10000):
		Base(ctr, capacity),
		current_(start),
		length_(length)
	{}

	virtual bool get(InputTuple& value)
	{
		if (processed_ < length_)
		{
			value = InputTupleAdapter::convert(*current_);

			current_++;
			processed_++;
			return true;
		}
		else {
			return false;
		}
	}
};


template <typename CtrT, typename InputIterator>
class IteratorVectorInputProvider: public memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength> {
	using Base = memoria::btss::AbstractBTSSInputProvider<CtrT, CtrT::Types::LeafDataLength>;

public:

	using CtrSizeT	= typename Base::CtrSizeT;
	using Position	= typename Base::Position;
	using InputBuffer = typename Base::InputBuffer;

	using InputTuple 		= typename CtrT::Types::template StreamInputTuple<0>;
	using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<0>;

	using InputValue = InputTuple;

	InputIterator current_;
	BigInt length_;



	Int input_start_ = 0;
	Int input_size_ = 0;
	static constexpr Int INPUT_END = 1000;

	InputValue input_value_buffer_[INPUT_END];

public:
	IteratorVectorInputProvider(CtrT& ctr, InputIterator start, BigInt length, Int capacity = 10000):
		Base(ctr, capacity),
		current_(start),
		length_(length)
	{
	}

	virtual Int get(InputBuffer* buffer, Int pos)
	{
		if (input_start_ == input_size_)
		{
			input_start_ = 0;

			for (input_size_ = 0 ;length_ > 0 && input_size_ < INPUT_END; input_size_++, current_++, length_--)
			{
				input_value_buffer_[input_size_] = InputTupleAdapter::convert(0, *current_);
			}
		}

		if (input_start_ < input_size_)
		{
			auto inserted = buffer->append(input_value_buffer_, input_start_, input_size_ - input_start_);

			input_start_ += inserted;

			return inserted;
		}

		return -1;
	}
};


}
}

#endif
