
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>
#include <memoria/v1/prototypes/bt_tl/bttl_input.hpp>
#include <memoria/v1/prototypes/bt_tl/bttl_iobuf_input.hpp>
#include <memoria/v1/core/container/container.hpp>

namespace memoria {
namespace v1 {
namespace bttl    {

using bt::StreamTag;

template <typename CtrT>
struct BTTLTupleFactory {
    template <Int StreamIdx>
    using InputTuple        = typename CtrT::Types::template StreamInputTuple<StreamIdx>;

    template <Int StreamIdx>
    using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

    using CtrSizeT          = typename CtrT::Types::CtrSizeT;

    static constexpr Int Streams = CtrT::Types::Streams;


    auto populate(StreamTag<0>, CtrSizeT col)
    {
        return InputTupleAdapter<0>::convert(0, IL<BigInt>({0}));
    }

    template <Int StreamIdx>
    auto populate(StreamTag<StreamIdx>, CtrSizeT col)
    {
        return InputTupleAdapter<StreamIdx>::convert(0, IL<BigInt>({0}));
    }

    auto populateLastStream(CtrSizeT prev_col, CtrSizeT col)
    {
        return InputTupleAdapter<Streams - 1>::convert(0, prev_col % 256);
    }
};



template <
    typename CtrT,
    typename RngT,
    template <typename> class TupleFactory = BTTLTupleFactory
>
class RandomDataInputProvider {


public:

    using CtrSizesT     = typename CtrT::Types::Position;
    using CtrSizeT      = typename CtrT::Types::CtrSizeT;

    using Rng           = RngT;

    template <Int StreamIdx>
    using InputTuple        = typename CtrT::Types::template StreamInputTuple<StreamIdx>;

    template <Int StreamIdx>
    using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

    static constexpr Int Streams = CtrT::Types::Streams;

    using LastStreamEntry = typename CtrT::Types::template StreamInputTuple<Streams - 1>;

private:
    CtrSizesT counts_;
    CtrSizesT limits_;
    CtrSizesT current_limits_;
    CtrSizesT totals_;

    Int level_ = 0;

    Rng& rng_;

    TupleFactory<CtrT> tuple_factory_;

    StreamEntryBuffer<LastStreamEntry> last_stream_buf_;

public:
    RandomDataInputProvider(const CtrSizesT& limits, Rng& rng, Int level = 0):
        limits_(limits),
        level_(level),
        rng_(rng)
    {
        current_limits_[0] = limits_[0];

        for (Int c = 1; c < Streams; c++)
        {
            current_limits_[c] = getRandom(limits_[c]);
        }
    }

    auto query()
    {
        if (counts_[level_] < current_limits_[level_])
        {
            if (level_ < Streams - 1)
            {
                return RunDescr(level_++, 1);
            }
            else {
                return RunDescr(level_, current_limits_[level_] - counts_[level_]);
            }
        }
        else if (level_ > 0)
        {
            counts_[level_] = 0;
            current_limits_[level_] = getRandom(limits_[level_]);

            level_--;

            return this->query();
        }
        else {
            return RunDescr(-1);
        }
    }

    template <typename Buffer>
    auto populate(StreamTag<0>, Buffer&& buffer, CtrSizeT length)
    {
        auto c0 = counts_[0];

        for (auto c = 0; c < length; c++, c0++)
        {
            buffer.append_entry(tuple_factory_.populate(StreamTag<0>(), c0));
        }

        counts_[0] += length;
        totals_[0] += length;

        return length;
    }

    template <Int StreamIdx, typename Buffer>
    auto populate(StreamTag<StreamIdx>, Buffer&& buffer, CtrSizeT length)
    {
        auto c0 = counts_[StreamIdx];

        for (auto c = 0; c < length; c++, c0++)
        {
            buffer.append_entry(tuple_factory_.populate(StreamTag<StreamIdx>(), c0));
        }

        counts_[StreamIdx] += length;
        totals_[StreamIdx] += length;

        return length;
    }

    template <typename Buffer>
    auto populateLastStream(Buffer&& buffer, CtrSizeT length)
    {
        auto col = counts_[Streams - 2];
        auto c0 = counts_[Streams - 1];


        if (!last_stream_buf_.has_data())
        {
            last_stream_buf_.reset();

            last_stream_buf_.append(length, [&](){
                return tuple_factory_.populateLastStream(col % 256, c0++);
            });
        }

        Int inserted = buffer.buffer()->append_buffer(last_stream_buf_);

        last_stream_buf_.commit(inserted);

        counts_[Streams - 1] += inserted;
        totals_[Streams - 1] += inserted;

        return inserted;
    }

    const CtrSizesT& consumed() const{
        return totals_;
    }
private:
    CtrSizeT getRandom(CtrSizeT limit)
    {
        return 1 + rng_() % (2 * limit - 1);
    }
};





template <
    typename CtrT,
    template <typename> class TupleFactory = BTTLTupleFactory
>
class DeterministicDataInputProvider {
public:

    using CtrSizesT     = typename CtrT::Types::Position;
    using CtrSizeT      = typename CtrT::Types::CtrSizeT;

    template <Int StreamIdx>
    using InputTupleAdapter = typename CtrT::Types::template InputTupleAdapter<StreamIdx>;

    static constexpr Int Streams = CtrT::Types::Streams;

    using LastStreamEntry = typename CtrT::Types::template StreamInputTuple<Streams - 1>;

private:
    CtrSizesT counts_;
    CtrSizesT limits_;

    CtrSizesT totals_;

    Int level_;

    TupleFactory<CtrT> tuple_factory_;

    StreamEntryBuffer<LastStreamEntry> last_stream_buf_;


public:
    DeterministicDataInputProvider(const CtrSizesT& limits, Int level = 0):
        limits_(limits),
        level_(level),
        last_stream_buf_(1000)
    {}

    auto query()
    {
        if (counts_[level_] < limits_[level_])
        {
            if (level_ < Streams - 1)
            {
                return RunDescr(level_++, 1);
            }
            else {
                return RunDescr(level_, limits_[level_] - counts_[level_]);
            }
        }
        else if (level_ > 0)
        {
            counts_[level_] = 0;
            level_--;

            return this->query();
        }
        else {
            return RunDescr(-1);
        }
    }

    template <typename Buffer>
    auto populate(StreamTag<0>, Buffer&& buffer, Int length)
    {
        auto c0 = counts_[0];

        for (auto c = 0; c < length; c++, c0++)
        {
            buffer.append_entry(tuple_factory_.populate(StreamTag<0>(), c0));
        }

        counts_[0] += length;
        totals_[0] += length;

        return length;
    }

    template <Int StreamIdx, typename Buffer>
    auto populate(StreamTag<StreamIdx>, Buffer&& buffer, Int length)
    {
        auto c0 = counts_[StreamIdx];

        for (auto c = 0; c < length; c++, c0++)
        {
            buffer.append_entry(tuple_factory_.populate(StreamTag<StreamIdx>(), c0));
        }

        counts_[StreamIdx] += length;
        totals_[StreamIdx] += length;

        return length;
    }

    template <typename Buffer>
    auto populateLastStream(Buffer&& buffer, Int length)
    {
        auto col = counts_[Streams - 2];
        auto c0 = counts_[Streams - 1];

        if (!last_stream_buf_.has_data())
        {
            last_stream_buf_.reset();

            last_stream_buf_.append(length, [&](){
                return tuple_factory_.populateLastStream(col % 256, c0++);
            });
        }

        Int inserted = buffer.buffer()->append_buffer(last_stream_buf_);

        last_stream_buf_.commit(inserted);

        counts_[Streams - 1] += inserted;
        totals_[Streams - 1] += inserted;

        return inserted;
    }

    const CtrSizesT& consumed() const{
        return totals_;
    }
};







namespace iobuf {



template <typename CtrT, typename IOBufferT = IOBuffer>
class DeterministicAdapterBase: public bttl::iobuf::FlatTreeIOBufferAdapter<CtrT::Types::Streams, IOBufferT> {

protected:

	static constexpr Int Streams = CtrT::Types::Streams;

	using Base 	 	= bttl::iobuf::FlatTreeIOBufferAdapter<Streams, IOBufferT>;
	using MyType 	= DeterministicAdapterBase<CtrT, IOBufferT>;

	using CtrSizesT = typename CtrT::Types::CtrSizesT;

	CtrSizesT structure_;
	IOBufferT io_buffer_;

	Int level_ = 0;

	struct StructureGenerator: public bttl::iobuf::FlatTreeStructureGeneratorBase<StructureGenerator, Streams> {

		MyType* adapter_;

		StructureGenerator(MyType* adapter, Int level):
			bttl::iobuf::FlatTreeStructureGeneratorBase<StructureGenerator, Streams>(level),
			adapter_(adapter)
		{}


		auto prepare(const StreamTag<0>&)
		{
			return adapter_->structure()[0];
		}

		template <Int Idx, typename Pos>
		auto prepare(const StreamTag<Idx>&, const Pos& pos)
		{
			return adapter_->structure()[Idx];
		}
	};


	StructureGenerator structure_generator_;

public:

	DeterministicAdapterBase(const CtrSizesT& structure, Int level = 0, size_t iobuffer_size = 65536):
		structure_(structure),
		io_buffer_(iobuffer_size),
		structure_generator_(this, level)
	{
		structure_generator_.init();
	}

	const CtrSizesT& structure() {
		return structure_;
	}

	StructureGenerator& structure_generator() {
		return structure_generator_;
	}

	const StructureGenerator& structure_generator() const {
		return structure_generator_;
	}


	virtual IOBuffer& buffer() {return io_buffer_;}

	virtual bttl::iobuf::RunDescr query()
	{
		return structure_generator_.query();
	}

	virtual Int populate_stream(Int stream, IOBuffer& buffer, Int length) = 0;
};




template <typename CtrT, typename RngT, typename IOBufferT = IOBuffer>
class RandomAdapterBase: public bttl::iobuf::FlatTreeIOBufferAdapter<CtrT::Types::Streams, IOBufferT> {
public:
	using Rng = RngT;
protected:


	static constexpr Int Streams = CtrT::Types::Streams;

	using Base 	 	= bttl::iobuf::FlatTreeIOBufferAdapter<Streams, IOBufferT>;
	using MyType 	= RandomAdapterBase<CtrT, RngT, IOBuffer>;

	using CtrSizesT = typename CtrT::Types::CtrSizesT;

	CtrSizesT structure_;
	IOBufferT io_buffer_;

	Int level_ = 0;

	struct StructureGenerator: public bttl::iobuf::FlatTreeStructureGeneratorBase<StructureGenerator, Streams> {

		MyType* adapter_;

		StructureGenerator(MyType* adapter, Int level):
			bttl::iobuf::FlatTreeStructureGeneratorBase<StructureGenerator, Streams>(level),
			adapter_(adapter)
		{}


		auto prepare(const StreamTag<0>&)
		{
			return adapter_->structure()[0];
		}

		template <Int Idx, typename Pos>
		auto prepare(const StreamTag<Idx>&, const Pos& pos)
		{
			auto limit = adapter_->structure()[Idx];
			BigInt v = adapter_->rng()();

			return 1 + v % (2 * limit - 1);
		}
	};


	StructureGenerator structure_generator_;

	RngT rng_;

public:

	RandomAdapterBase(const CtrSizesT& structure, const RngT& rng, Int level = 0, size_t iobuffer_size = 65536):
		structure_(structure),
		io_buffer_(iobuffer_size),
		structure_generator_(this, level)
	{
		structure_generator_.init();
	}

	const CtrSizesT& structure() {
		return structure_;
	}

	RngT& rng() {return rng_;}
	const RngT& rng() const {return rng_;}

	virtual IOBuffer& buffer() {return io_buffer_;}

	virtual bttl::iobuf::RunDescr query()
	{
		return structure_generator_.query();
	}

	StructureGenerator& structure_generator() {
		return structure_generator_;
	}

	const StructureGenerator& structure_generator() const {
		return structure_generator_;
	}

	virtual Int populate_stream(Int stream, IOBuffer& buffer, Int length) = 0;
};






}

}
}}
