
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
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>

#include <memoria/v1/prototypes/bt/bt_iterator.hpp>

#include <memory>
#include <tuple>

namespace memoria {
namespace v1 {
namespace btfl {
namespace io {

using bt::StreamTag;

enum class Ending {
	CONTINUE, END_OF_PAGE, END_OF_IOBUFFER, END_OF_STREAM, END_OF_TOTALS
};

template <typename PkdStruct>
class ReadIterator {
	using ReadState = typename PkdStruct::ReadState;

	const PkdStruct* struct_ = nullptr;
	ReadState state_;
	Int idx_  = 0;
	Int size_ = 0;

public:
	ReadIterator(const PkdStruct* pstruct, Int idx = 0): struct_(pstruct), state_(pstruct->positions(idx)), idx_(idx)
	{
		size_ = pstruct->size();
	}

	ReadIterator() {}

	Int size() const {
		return size_;
	}

	Int idx() const {
		return idx_;
	}

	template <typename IOBuffer>
	auto next(IOBuffer& io_buffer)
	{
		if (idx_ < size_)
		{
			if (struct_->readTo(state_, io_buffer))
			{
				return Ending::CONTINUE;
			}

			return Ending::END_OF_IOBUFFER;
		}
		else {
			return Ending::END_OF_PAGE;
		}
	}
};



namespace {
	template <typename LeafNode, Int Idx, Int Streams, template <typename> class MapFn>
	struct MapAllStreams4Data {
		using Type = MergeLists<
				AsTuple<typename LeafNode::template StreamDispatcher<Idx>::template ForAllStructs<MapFn>>,
				typename MapAllStreams4Data<LeafNode, Idx + 1, Streams, MapFn>::Type
		>;
	};

	template <typename LeafNode, Int Idx, template <typename> class MapFn>
	struct MapAllStreams4Data<LeafNode, Idx, Idx, MapFn> {
		using Type = TL<>;
	};
}













class PopulateStatus {
	Int entries_;
	Ending ending_;
public:
	PopulateStatus(Int entries, Ending ending): entries_(entries), ending_(ending)
	{}

	PopulateStatus(): entries_(0), ending_(Ending::CONTINUE)
	{}

	auto entries() const {return entries_;}
	Ending ending() const {return ending_;}
};



template <typename IteratorT, typename IOBufferT>
class BTTLWalker {

	using CtrT		= typename IteratorT::Container;

	using Types 	= typename CtrT::Types;
	using MyType 	= BTTLWalker<IteratorT, IOBufferT>;

	using LeafPrefixRanks = typename Types::LeafPrefixRanks;

	using CtrSizeT  = typename Types::CtrSizeT;
	using CtrSizesT = typename Types::CtrSizesT;
	using NodeBaseG = typename Types::NodeBaseG;

	using LeafDispatcher = typename Types::Pages::LeafDispatcher;
	using Iterator  = IteratorT;

	static constexpr Int DataStreams 				= Types::DataStreams;
	static constexpr Int StructureStreamIdx 		= Types::StructureStreamIdx;

	using DataStreamsSizes 	= core::StaticVector<Int, DataStreams>;

	template <typename PackedStructDescr>
	using StreamDataMapFn = HasType<ReadIterator<typename PackedStructDescr::Type>>;

	using ReadStreamDataStates 	= AsTuple<typename MapAllStreams4Data<typename Types::LeafNode, 0, DataStreams, StreamDataMapFn>::Type>;

	using StructureIterator 	= typename Types::template LeafPackedStruct<IntList<StructureStreamIdx, 1>>::Iterator;




	class WriteStreamFn;
	friend class WriteStreamFn;

	class WriteStreamFn {

		MyType* walker_;

	public:

		PopulateStatus status_;

		WriteStreamFn(MyType* walker): walker_(walker) {}

		template <Int Idx, typename StreamsData>
		void process(StreamsData& stream_data, Int stream, BigInt length, IOBufferT& io_buffer)
		{
			if (Idx == stream)
			{
				Int entries = 0;

				for (BigInt c = 0; c < length; c++, entries++)
				{
					auto backup 	  = stream_data;
					auto write_status = write_entry(stream_data, io_buffer);

					if (write_status != Ending::CONTINUE)
					{
						stream_data = backup;

						status_ = PopulateStatus(entries, write_status);
						return;
					}
				}

				status_ = PopulateStatus(entries, Ending::CONTINUE);
			}
		}

		struct WriteEntryFn
		{
			Ending ending_ = Ending::CONTINUE;

			template <Int Idx, typename StreamsData>
			void process(StreamsData& stream_data, IOBufferT& io_buffer)
			{
				if (ending_ == Ending::CONTINUE)
				{
					ending_ = stream_data.next(io_buffer);
				}
			}
		};

		template <typename StreamData>
		Ending write_entry(StreamData& stream_data, IOBufferT& io_buffer)
		{
			WriteEntryFn fn;
			ForAllTuple<std::tuple_size<std::remove_reference_t<StreamData>>::value>::process(stream_data, fn, io_buffer);
			return fn.ending_;
		}
	};

	ReadStreamDataStates  stream_data_;

	Iterator* iter_;

	Int idx_;

	NodeBaseG leaf_;


	BigInt run_pos_ 	= 0;
	BigInt run_length_  = 0;

	CtrSizesT locals_;

	bool run_written_ = true;

	StructureIterator symbols_;

	Int stream_;

public:

	BTTLWalker(Iterator& iter):
		iter_(&iter),
		leaf_(iter.leaf()),
		stream_(iter.stream())
	{
		prepare_new_page(iter.idx());

		auto data_positions = rank(idx_);

		configure_data(data_positions);
	}

	Int idx() const {
		return idx_;
	}

	auto& leaf() {
		return leaf_;
	}

	const auto& leaf() const {
		return leaf_;
	}

	void prepare_new_page(Int start_idx = 0)
	{
		idx_ 	 = start_idx;
		symbols_ = leaf_structure()->iterator(idx_);
	}

	PopulateStatus write_stream(Int stream, BigInt length, IOBufferT& io_buffer)
	{
		WriteStreamFn fn(this);
		ForAllTuple<DataStreams>::process(stream_data_, fn, stream, length, io_buffer);
		return fn.status_;
	}

	template <typename IOBuffer>
	PopulateStatus populate(IOBuffer& buffer)
	{
		Int entries = 0;

		if (!run_written_)
		{
			if (buffer.template putSymbolsRun<DataStreams>(stream_, run_length_))
			{
				entries++;
				run_written_ = true;
			}
			else {
				throw Exception(MA_SRC, "Can't write Symbols run into IOBuffer that is probably to small");
			}
		}

		while(symbols_.has_data())
		{
			if (run_pos_ == run_length_)
			{
				run_pos_ = 0;
				auto run = symbols_.run();

				stream_ 	= run.symbol();
				run_length_ = run.length();

				if (buffer.template putSymbolsRun<DataStreams>(stream_, run_length_))
				{
					entries++;
					run_written_ = true;
				}
				else {
					run_written_ = false;
					return PopulateStatus(entries, Ending::END_OF_IOBUFFER);
				}
			}

			auto write_result = write_stream(stream_, run_length_ - run_pos_, buffer);

			entries  += write_result.entries();
			run_pos_ += write_result.entries();

			if (run_pos_ == run_length_)
			{
				symbols_.next_run();
			}

			if (write_result.ending() == Ending::CONTINUE)
			{

			}
			else if (write_result.ending() == Ending::END_OF_IOBUFFER)
			{
				return PopulateStatus(entries, Ending::END_OF_IOBUFFER);
			}
			else {
				throw Exception(MA_SRC, SBuf() << "Unexpected BTTLWalker operation result: " << (Int)write_result.ending());
			}
		}

		return PopulateStatus(entries, Ending::END_OF_PAGE);
	}


	bool next_page()
	{
		NodeBaseG next = iter_->ctr().getNextNodeP(leaf_);

		if (next)
		{
			leaf_ = next;

			prepare_new_page();

			configure_data(DataStreamsSizes());

			return true;
		}
		else {
			return false;
		}
	}

	const CtrSizesT& locals() {
		return locals_;
	}

private:

	DataStreamsSizes rank(Int idx) const
	{
		DataStreamsSizes sizes;

		auto s = this->leaf_structure();

		for (Int c = 0; c < DataStreamsSizes::Indexes; c++)
		{
			sizes[c] = s->rank(idx, c);
		}

		return sizes;
	}

    const auto* leaf_structure() const
    {
    	return getPackedStruct<IntList<StructureStreamIdx, 1>>();
    }


    template <typename SubstreamPath>
    struct GetPackedStructFn {
        template <typename T>
        auto treeNode(const LeafNode<T>* node) const
        {
            return node->template substream<SubstreamPath>();
        }

        template <typename T>
        auto treeNode(LeafNode<T>* node) const
        {
            return node->template substream<SubstreamPath>();
        }
    };

    template <typename SubstreamPath>
    const auto* getPackedStruct() const
    {
    	return LeafDispatcher::dispatch(leaf_, GetPackedStructFn<SubstreamPath>());
    }

    template <typename SubstreamPath>
    auto* getPackedStruct()
    {
    	return LeafDispatcher::dispatch(leaf_, GetPackedStructFn<SubstreamPath>());
    }


	void dump_symbols()
	{
		leaf_structure()->dump();
	}

    struct ConfigureDataFn {

		template <Int StreamIdx, typename StreamData, typename Node>
		void process(StreamData&& data, const DataStreamsSizes& idx, const Node* leaf)
		{
			constexpr Int Substreams = std::tuple_size<typename std::remove_reference<StreamData>::type>::value;

			ForAllTuple<Substreams>::process(data, *this, idx[StreamIdx], leaf, bt::StreamTag<StreamIdx>());
		}

		template <Int SubstreamIdx, typename StreamData, typename Node, Int StreamIdx>
		void process(StreamData&& data, Int idx, const Node* leaf, const bt::StreamTag<StreamIdx>&)
		{
			constexpr Int FullSubstreamIdx = Node::template StreamStartIdx<StreamIdx>::Value + SubstreamIdx;
			using T = typename Node::Dispatcher::template StreamTypeT<FullSubstreamIdx>::Type;

			const T* pstruct = leaf->allocator()->template get<T>(FullSubstreamIdx + Node::SubstreamsStart);

			data = ReadIterator<T>(pstruct, idx);
		}

    	template <typename NTypes>
    	void treeNode(const LeafNode<NTypes>* leaf, ReadStreamDataStates& stream_data, const DataStreamsSizes& idx)
    	{
    		ForAllTuple<DataStreams>::process(stream_data, *this, idx, leaf);
    	}
    };


	void configure_data(const DataStreamsSizes& idx)
	{
		return LeafDispatcher::dispatch(leaf_, ConfigureDataFn(), stream_data_, idx);
	}
};





}}}}
