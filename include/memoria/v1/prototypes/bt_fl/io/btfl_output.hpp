
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
namespace iobuf {

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

	template <
		typename MyType,
    	template <Int> class StreamsSizesPath,
		typename CtrSizeT,
		typename Position
	>
	struct StructureFlattenerFnBase {
		static constexpr Int BufferSize = 32;
		static constexpr Int Streams 	= Position::Indexes;

		CtrSizeT pos_;
		CtrSizeT target_;

		Position sizes_;
		Position prefix_;
		Position indexes_;

		StructureFlattenerFnBase(const Position& sizes, const Position& prefix, CtrSizeT target):
			target_(target),
			sizes_(sizes),
			prefix_(prefix),
			indexes_()
		{
			pos_ = prefix.sum();
		}

		MyType& self() {return *T2T<MyType*>(this);}
		const MyType& self() const {return *T2T<const MyType*>(this);}

		template <Int StreamIdx, typename NextHelper, typename Node>
		void process(const Node* node) {
			process<StreamIdx, NextHelper>(node, prefix_[StreamIdx], sizes_[StreamIdx]);
		}

		template <Int StreamIdx, typename NextHelper, typename Node>
		void process(const Node* node, CtrSizeT start, CtrSizeT end)
		{
			CtrSizeT buffer[BufferSize];
			for (auto& v: buffer) v = 0;

			Int buffer_pos = 0;
			Int buffer_size = 0;

			const Int size   = sizes_[StreamIdx];
			const Int limit  = end <= size ? end : size;

			for (auto i = start; i < limit; i++)
			{
				if (pos_ < target_)
				{
					auto next_length = this->template buffered_get<StreamIdx>(node, i, buffer_pos, buffer_size, size, buffer);

					indexes_[StreamIdx]++;

					self().symbol(StreamIdx, pos_, 1);

					pos_++;

					auto next_offset = prefix_[StreamIdx  + 1] + indexes_[StreamIdx + 1];

					NextHelper::process(node, *this, next_offset, next_offset + next_length);
				}
				else {
					break;
				}
			}
		}

		template <Int StreamIdx, typename Node>
		void processLast(const Node* node) {
			processLast<StreamIdx>(node, prefix_[StreamIdx], sizes_[StreamIdx]);
		}

		template <Int StreamIdx, typename Node, typename... Args>
		void processLast(const Node* node, CtrSizeT start, CtrSizeT end, Args&&...)
		{
			CtrSizeT size = node->template streamSize<StreamIdx>();

			const CtrSizeT limit = end <= size ? end : size;

			auto delta = target_ - pos_;

			if (start + delta <= limit)
			{
				indexes_[StreamIdx] += delta;

				self().symbol(StreamIdx, pos_, delta);

				pos_ += delta;
			}
			else {
				indexes_[StreamIdx] += limit - start;

				self().symbol(StreamIdx, pos_, limit - start);

				pos_ += limit - start;
			}
		}

	private:

		template <Int StreamIdx, typename Node>
		CtrSizeT buffered_get(const Node* node, Int idx, Int& p0, Int& s0, Int size, CtrSizeT buffer[BufferSize])
		{
			if (idx < s0) {
				return buffer[idx - p0];
			}
			else {
				Int limit = (idx + BufferSize) < size ? idx + BufferSize : size;

				using Path = StreamsSizesPath<StreamIdx>;

				// FIXME: use Value Iterator pattern here

				Int idx0 = 0;
				node->template substream<Path>()->read(0, idx, limit, make_fn_with_next([&](Int block, auto&& value){
					buffer[idx0] = value;
				}, [&]{idx0++;}));

				p0 = idx;
				s0 = limit;

				return buffer[0];
			}
		}
	};





	template <Int Idx>
	struct StructurePrefixFlattenerHelper
	{
	    template <typename Walker, typename Node, typename Position, typename Prefixes, typename... Args>
	    static void process(const Walker* walker, Node&& node, Position&& sizes, Prefixes&& prefixes, Args&&... args)
	    {
	    	auto total = prefixes[Idx - 1].sum();

	    	if (total > 0) {
	    		walker->template flatten_tree<Idx>(std::forward<Node>(node), sizes, prefixes[Idx], total, std::forward<Args>(args)...);
	    	}

	        StructurePrefixFlattenerHelper<Idx - 1>::process(walker, std::forward<Node>(node), sizes, prefixes, std::forward<Args>(args)...);
	    }
	};


	template <>
	struct StructurePrefixFlattenerHelper<1> {
	    template <typename Walker, typename Node, typename Position, typename Prefixes, typename... Args>
	    static void process(const Walker* walker, Node&& node, Position&& sizes, Prefixes&& prefixes, Args&&... args)
	    {
	    	auto total = prefixes[0].sum();
	    	if (total > 0) {
	    		walker->template flatten_tree<1>(std::forward<Node>(node), sizes, prefixes[1], total, std::forward<Args>(args)...);
	    	}
	    }
	};


	template <typename LeafNode, Int Idx, Int Streams, template <typename> class MapFn>
	struct MapAllStreams4Data {
		using Type = MergeLists<
				AsTuple<typename LeafNode::template BTTLStreamDataDispatcher<Idx>::template ForAllStructs<MapFn>>,
				typename MapAllStreams4Data<LeafNode, Idx + 1, Streams, MapFn>::Type
		>;
	};

	template <typename LeafNode, Int Idx, template <typename> class MapFn>
	struct MapAllStreams4Data<LeafNode, Idx, Idx, MapFn> {
		using Type = AsTuple<typename LeafNode::template BTTLLastStreamDataDispatcher<Idx>::template ForAllStructs<MapFn>>;
	};





}



template <typename CtrT, typename ConsumerFn>
class FlatTreeStructureBuilder {

	using Types 	= typename CtrT::Types;
	using MyType 	= FlatTreeStructureBuilder<CtrT, ConsumerFn>;

	using LeafPrefixRanks = typename Types::LeafPrefixRanks;

	using CtrSizeT  = typename Types::CtrSizeT;
	using CtrSizesT = typename Types::CtrSizesT;
	using NodeBaseG = typename Types::NodeBaseG;

	using LeafDispatcher = typename Types::Pages::LeafDispatcher;

	template <Int StreamIdx>
	using LeafSizesSubstreamPath = typename Types::template LeafSizesSubstreamPath<StreamIdx>;


	static constexpr Int Streams 			= Types::Streams;
	static constexpr Int SearchableStreams 	= Streams - 1;

	template <Int> friend struct StructurePrefixFlattenerHelper;

	class StructureFlattenerFn: public StructureFlattenerFnBase<StructureFlattenerFn, LeafSizesSubstreamPath, CtrSizeT, CtrSizesT>
	{
		using Base = StructureFlattenerFnBase<StructureFlattenerFn, LeafSizesSubstreamPath, CtrSizeT, CtrSizesT>;

		ConsumerFn& consumer_fn_;

	public:
		StructureFlattenerFn(ConsumerFn& consumer, const CtrSizesT& sizes, const CtrSizesT& prefix, CtrSizeT target):
			Base(sizes, prefix, target),
			consumer_fn_(consumer)
		{}

		void symbol(Int sym, CtrSizeT pos, CtrSizeT length)
		{
			consumer_fn_(sym, pos, length);
		}
	};



    template <Int Stream>
    struct StructureBuilderFn {

    	template <typename NTypes, typename... Args>
    	void treeNode(const LeafNode<NTypes>* leaf, ConsumerFn& consumer_fn, const CtrSizesT& sizes, const CtrSizesT& prefix, CtrSizeT pos, Args&&... args)
    	{
    		StructureFlattenerFn fn(consumer_fn, sizes, prefix, pos);
    		bttl::detail::StreamsRankHelper<Stream, SearchableStreams>::process(leaf, fn, std::forward<Args>(args)...);
    	}
    };

    const CtrT* ctr_;

    ConsumerFn& consumer_fn_;

public:

    FlatTreeStructureBuilder(const CtrT& ctr, ConsumerFn& consumer_fn): ctr_(&ctr), consumer_fn_(consumer_fn) {}

	void build(const NodeBaseG& leaf, const CtrSizesT& leaf_extent)
	{
		CtrSizesT sizes = ctr_->getLeafStreamSizes(leaf);
		LeafPrefixRanks prefixes;

        ctr_->compute_leaf_prefixes(leaf, leaf_extent, prefixes);

		StructurePrefixFlattenerHelper<Streams - 1>::process(this, leaf, sizes, prefixes);

		this->template flatten_tree<0>(leaf, sizes, prefixes[0], sizes.sum());
	}

	void consume(Int symbol, CtrSizeT pos, CtrSizeT length)
	{
		consumer_fn_(symbol, pos, length);
	}

private:

    template <Int Stream, typename... Args>
    void flatten_tree(const NodeBaseG& leaf, Args&&... args) const
    {
        return LeafDispatcher::dispatch(leaf, StructureBuilderFn<Stream>(), consumer_fn_, std::forward<Args>(args)...);
    }
};












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

	static constexpr Int Streams 			= Types::Streams;
	static constexpr Int SearchableStreams 	= Streams - 1;

	using StreamsSizes 	= core::StaticVector<Int, Streams>;

	template <typename PackedStructDescr>
	using StreamDataMapFn = HasType<ReadIterator<typename PackedStructDescr::Type>>;

	using ReadStreamDataStates = AsTuple<typename MapAllStreams4Data<typename Types::LeafNode, 0, SearchableStreams, StreamDataMapFn>::Type>;


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

	Int stream_ = 0;
	StreamsSizes idx_;

	DefaultIOBuffer symbols_;
	size_t symbol_runs_ = 0;
	size_t current_symbols_run_ = 0;

	NodeBaseG leaf_;
	CtrSizesT leaf_extents_;

	BigInt run_pos_ 	= 0;
	BigInt run_length_  = 0;

	CtrSizesT locals_;

	bool run_written_ = true;

public:

	BTTLWalker(Iterator& iter):
		iter_(&iter),
		stream_(iter.stream()),
		symbols_(512),
		leaf_(iter.leaf()),
		leaf_extents_(iter.leaf_extent())
	{
		locals_ = iter.path();

		prepare_new_page();

		select(stream_, iter.idx());

		configure_data(idx_);
	}

	void prepare_new_page()
	{
		symbols_.rewind();
		idx_.clear();

		symbol_runs_ 		 = 0;
		current_symbols_run_ = 0;

		iter_->ctr().build_node_layout(leaf_, leaf_extents_, [&](Int symbol, CtrSizeT pos, CtrSizeT length)
		{
			auto backup = symbols_.pos();
			if (!symbols_.putSymbolsRun<Streams>(symbol, length))
			{
				symbols_.pos(backup);

				symbols_.enlarge();

				if (!symbols_.putSymbolsRun<Streams>(symbol, length))
				{
					throw Exception(MA_SRC, "Can't enlarge symbols IOBuffer enough to put next symbols run");
				}
			}

			symbol_runs_++;
		});

		symbols_.rewind();
	}

	PopulateStatus write_stream(Int stream, BigInt length, IOBufferT& io_buffer)
	{
		WriteStreamFn fn(this);
		ForAllTuple<Streams>::process(stream_data_, fn, stream_, length, io_buffer);
		return fn.status_;
	}

	template <typename IOBuffer>
	PopulateStatus populate(IOBuffer& buffer)
	{
		Int entries = 0;

		if (!run_written_)
		{
			if (buffer.template putSymbolsRun<Streams>(stream_, run_length_))
			{
				entries++;
				run_written_ = true;
			}
			else {
				throw Exception(MA_SRC, "Can't write Symbols run into IOBuffer that is probably to small");
			}
		}

		while(current_symbols_run_ < symbol_runs_)
		{
			if (run_pos_ == run_length_)
			{
				run_pos_ = 0;
				auto run = symbols_.template getSymbolsRun<Streams>();

				stream_ 	= run.symbol();
				run_length_ = run.length();

				if (buffer.template putSymbolsRun<Streams>(stream_, run_length_))
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
			idx_[stream_] += write_result.entries();

			resetLocals(stream_, write_result.entries());

			if (run_pos_ == run_length_)
			{
				current_symbols_run_++;
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
			leaf_extents_ += iter_->ctr().node_extents(leaf_);

			leaf_ = next;

			prepare_new_page();

			configure_data(StreamsSizes());

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

	void dump_symbols()
	{
		auto pos = symbols_.pos();
		symbols_.rewind();

		for (size_t c = 0; c < symbol_runs_; c++)
		{
			auto run = symbols_.template getSymbolsRun<Streams>();
			cout << run.symbol() << " -- " << run.length() << endl;
		}

		symbols_.pos(pos);
	}

    struct ConfigureDataFn {

		template <Int StreamIdx, typename StreamData, typename Node>
		void process(StreamData&& data, const StreamsSizes& idx, const Node* leaf)
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
    	void treeNode(const LeafNode<NTypes>* leaf, ReadStreamDataStates& stream_data, const StreamsSizes& idx)
    	{
    		ForAllTuple<Streams>::process(stream_data, *this, idx, leaf);
    	}
    };


	void configure_data(const StreamsSizes& idx)
	{
		return LeafDispatcher::dispatch(leaf_, ConfigureDataFn(), stream_data_, idx);
	}

	void resetLocals(Int stream, CtrSizeT length)
	{
		locals_[stream] += length;

		for (Int s = stream + 1; s < Streams; s++)
		{
			locals_[s] = 0;
		}
	}



	void select(Int stream, Int idx)
	{
		symbols_.rewind();
		idx_.clear();
		idx_[stream] = idx;

		BigInt symbol_base = 0;

		while (true)
		{
			auto run = symbols_.template getSymbolsRun<Streams>();

			if (run.symbol() == stream)
			{
				Int run_length 	= run.length();

				if (idx >= symbol_base + run_length)
				{
					symbol_base += run_length;
				}
				else {
					run_pos_ 	= idx - symbol_base;
					run_length_ = run_length;

					break;
				}
			}
			else {
				idx_[run.symbol()] += run.length();
			}

			current_symbols_run_++;
		}
	}
};





}}}}
