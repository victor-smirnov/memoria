// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_MAP_LABELS_BASE_HPP_
#define MEMORIA_CORE_PACKED_MAP_LABELS_BASE_HPP_

#include <memoria/core/packed/map/packed_map_fse_tree_base.hpp>
#include <memoria/core/packed/map/packed_map_vle_tree_base.hpp>

namespace memoria {

template <Int Bits>
struct LabelDescr {};


namespace internal {

template <typename Labels, Int Idx = 0>
class LabelDispatcherListBuilder;

template <Int Bits, typename... Tail, Int Idx>
class LabelDispatcherListBuilder<TypeList<LabelDescr<Bits>, Tail...>, Idx> {
    typedef PkdFSSeq<typename PkdFSSeqTF<Bits>::Type>                           LabelStream;
public:
     typedef typename MergeLists<
                StreamDescr<LabelStream, Idx>,
                typename LabelDispatcherListBuilder<
                    TypeList<Tail...>,
                    Idx + 1
                >::Type
     >::Result                                                                  Type;
};

template <Int Idx>
class LabelDispatcherListBuilder<TypeList<>, Idx> {
public:
    typedef TypeList<>                                                          Type;
};



template <typename List> struct LabelsBlockSizeBuilder;

template <typename LabelStream, Int Idx, typename... Tail>
struct LabelsBlockSizeBuilder<TypeList<StreamDescr<LabelStream, Idx>, Tail...>> {
    static const Int Value = LabelStream::Indexes +
            LabelsBlockSizeBuilder<TypeList<Tail...>>::Value;
};

template <>
struct LabelsBlockSizeBuilder<TypeList<>> {
    static const Int Value = 0;
};


}



template <Int Blocks, typename Key, typename HiddenLabels, typename Labels>
class PackedMapLabelsBase: public PackedMapTreeBase<Blocks, Key> {

    typedef PackedMapTreeBase<Blocks, Key>                                      Base;
public:

    typedef PackedMapLabelsBase<Blocks, Key, HiddenLabels, Labels>              MyType;

    typedef HiddenLabels                                                        HiddenLabelsList;
    typedef Labels                                                              LabelsList;

    static const Int HiddenLabelsListSize                                       = ListSize<HiddenLabelsList>::Value;
    static const Int LabelsListSize                                             = ListSize<LabelsList>::Value;

    static const Int HiddenLabelsOffset                                         = 1;
    static const Int LabelsOffset                                               = HiddenLabelsOffset +
                                                                                    ListSize<HiddenLabelsList>::Value;

    static const Int TotalLabels                                                = HiddenLabelsListSize + LabelsListSize;


    typedef typename memoria::internal::LabelDispatcherListBuilder<HiddenLabelsList>::Type  HiddenLabelsStructsList;
    typedef typename memoria::internal::LabelDispatcherListBuilder<LabelsList>::Type        LabelsStructsList;


    typedef typename PackedDispatcherTool<
            HiddenLabelsOffset,
            HiddenLabelsStructsList
    >::Type                                                                     HiddenLabelsDispatcher;

    typedef typename PackedDispatcherTool<
            HiddenLabelsOffset + ListSize<HiddenLabelsList>::Value,
            LabelsStructsList
    >::Type                                                                     LabelsDispatcher;


    static const Int LabelsIndexes  = memoria::internal::LabelsBlockSizeBuilder<HiddenLabelsStructsList>::Value +
                                      memoria::internal::LabelsBlockSizeBuilder<LabelsStructsList>::Value;


    struct EmptySizeFn {
        Int size_ = 0;

        template <Int StreamIdx, typename Stream>
        void stream(Stream*)
        {
            size_ += Stream::empty_size();
        }
    };


    static Int labels_empty_size()
    {
        EmptySizeFn fn;

        HiddenLabelsDispatcher::dispatchAllStatic(fn);
        LabelsDispatcher::dispatchAllStatic(fn);

        return fn.size_;
    }


    struct LabelsBlockSizeFn {
        Int size_ = 0;

        template <Int StreamIdx, typename Stream>
        void stream(Stream*, Int stream_size)
        {
            size_ += Stream::estimate_block_size(stream_size);
        }
    };


    static Int labels_block_size(Int size)
    {
        LabelsBlockSizeFn fn;

        HiddenLabelsDispatcher::dispatchAllStatic(fn, size);
        LabelsDispatcher::dispatchAllStatic(fn, size);

        return fn.size_;
    }





    struct InitStructFn {
        PackedAllocator* allocator_;
        Int offset_;

        InitStructFn(PackedAllocator* target, Int offset): allocator_(target), offset_(offset) {}

        template <Int StreamIdx, typename Stream>
        void stream(Stream*)
        {
            allocator_->template allocateEmpty<Stream>(StreamIdx + offset_);
        }
    };

    void labels_init()
    {
        HiddenLabelsDispatcher::dispatchAllStatic(InitStructFn(this, HiddenLabelsOffset));
        LabelsDispatcher::dispatchAllStatic(InitStructFn(this, LabelsOffset));
    }






    struct LabelsDataLengthFn {
    	template <Int StreamIdx, typename Stream, typename Entropy, typename Entry>
    	void stream(Stream* stream, const Entry& entry, Entropy& entropy)
    	{
    		std::get<1 + StreamIdx + HiddenLabelsListSize>(entry) +=
    				Stream::computeDataLength(std::get<StreamIdx>(entry.labels()));
    	}
    };

    struct HiddenLabelsDataLengthFn {
    	template <Int StreamIdx, typename Stream, typename Entropy, typename Entry>
    	void stream(Stream* stream, const Entry& entry, Entropy& entropy)
    	{
    		std::get<1 + StreamIdx>(entry) +=
    				Stream::computeDataLength(std::get<StreamIdx>(entry.hidden_labels()));
    	}
    };


    template <typename Entropy, typename Entry>
    static void computeLabelsEntryDataLength(const Entry& entry, Entropy& entropy)
    {
    	HiddenLabelsDispatcher::dispatchAllStatic(HiddenLabelsDataLengthFn(), entry, entropy);
    	LabelsDispatcher::dispatchAllStatic(LabelsDataLengthFn(), entry, entropy);
    }



    template <typename Entry, typename MapSums>
    struct InsertLabelsFn {
        const Entry& entry_;
        MapSums& sums_;


        InsertLabelsFn(const Entry& entry, MapSums& sums):
            entry_(entry),
            sums_(sums)
        {}

        template <Int LabelIdx, typename Stream>
        void stream(Stream* stream, Int idx)
        {
            Int offset = MyType::label_block_offset(LabelIdx) + Blocks + 1;

            Int label = std::get<LabelIdx>(entry_.labels());

            stream->insert(idx, label);

            sums_[offset + label] += 1;
        }
    };

    template <typename Entry, typename MapSums>
    struct InsertHiddenLabelsFn {
        const Entry& entry_;
        MapSums& sums_;


        InsertHiddenLabelsFn(const Entry& entry, MapSums& sums):
            entry_(entry),
            sums_(sums)
        {}

        template <Int LabelIdx, typename Stream>
        void stream(Stream* stream, Int idx)
        {
            Int offset = MyType::hidden_label_block_offset(LabelIdx) + Blocks + 1;

            Int label  = std::get<LabelIdx>(entry_.hidden_labels());

            stream->insert(idx, label);

            sums_[offset + label] += 1;
        }
    };



    template <typename Entry, typename MapSums>
    void insertLabels(Int idx, const Entry& entry, MapSums& sums)
    {
        HiddenLabelsDispatcher::dispatchAll(this, InsertHiddenLabelsFn<Entry, MapSums>(entry, sums), idx);
        LabelsDispatcher::dispatchAll(this, InsertLabelsFn<Entry, MapSums>(entry, sums), idx);
    }




    struct InsertSpaceFn {
        template <Int StreamIdx, typename Stream>
        void stream(Stream* stream, Int start, Int length)
        {
            stream->insertSpace(start, length);
        }
    };


    void insertLabelsSpace(Int room_start, Int room_length)
    {
        HiddenLabelsDispatcher::dispatchAll(this, InsertSpaceFn(), room_start, room_length);
        LabelsDispatcher::dispatchAll(this, InsertSpaceFn(), room_start, room_length);
    }

    struct RemoveFn {
        template <Int StreamIdx, typename Stream>
        void stream(Stream* stream, Int start, Int end)
        {
            stream->removeSpace(start, end);
        }
    };

    void removeLabelsSpace(Int room_start, Int room_end)
    {
        HiddenLabelsDispatcher::dispatchAll(this, RemoveFn(), room_start, room_end);
        LabelsDispatcher::dispatchAll(this, RemoveFn(), room_start, room_end);
    }




    struct SplitToFn {
        PackedAllocator* target_;
        Int offset_;

        SplitToFn(PackedAllocator* target, Int offset): target_(target), offset_(offset) {}

        template <Int StreamIdx, typename Stream>
        void stream(Stream* stream, Int idx)
        {
            Stream* tgt_stream = target_->template get<Stream>(StreamIdx + offset_);
            stream->splitTo(tgt_stream, idx);
        }
    };

    void splitLabelsTo(MyType* other, Int split_idx)
    {
        HiddenLabelsDispatcher::dispatchAll(this, SplitToFn(other, HiddenLabelsOffset), split_idx);
        LabelsDispatcher::dispatchAll(this, SplitToFn(other, LabelsOffset), split_idx);
    }

    struct MergeWithFn {
        PackedAllocator* target_;
        Int offset_;

        MergeWithFn(PackedAllocator* target, Int offset): target_(target), offset_(offset) {}

        template <Int StreamIdx, typename Stream>
        void stream(Stream* stream)
        {
            Stream* tgt_stream = target_->template get<Stream>(StreamIdx + offset_);
            stream->mergeWith(tgt_stream);
        }
    };

    void mergeLabelsWith(MyType* other)
    {
        HiddenLabelsDispatcher::dispatchAll(this, MergeWithFn(other, HiddenLabelsOffset));
        LabelsDispatcher::dispatchAll(this, MergeWithFn(other, LabelsOffset));
    }



    struct ReindexFn {
        template <Int StreamIdx, typename Stream>
        void stream(Stream* stream)
        {
            stream->reindex();
        }
    };


    void reindexLabels()
    {
        HiddenLabelsDispatcher::dispatchAll(this, ReindexFn());
        LabelsDispatcher::dispatchAll(this, ReindexFn());
    }



    struct CheckFn {
        template <Int StreamIdx, typename Stream>
        void stream(const Stream* stream)
        {
            stream->check();
        }
    };

    void checkLabels() const
    {
        HiddenLabelsDispatcher::dispatchAll(this, CheckFn());
        LabelsDispatcher::dispatchAll(this, CheckFn());
    }



    struct DumpFn {
        template <Int StreamIdx, typename Stream>
        void stream(const Stream* stream, std::ostream& out)
        {
            out<<"Stream: "<<StreamIdx<<std::endl;
            stream->dump(out);
        }
    };


    void dumpLabels(std::ostream& out = std::cout) const
    {
        out<<"Hidden Labels:"<<std::endl;
        HiddenLabelsDispatcher::dispatchAll(this, DumpFn(), out);

        out<<"Labels:"<<std::endl;
        LabelsDispatcher::dispatchAll(this, DumpFn(), out);
    }


    // ====================================== Labels Operations ====================================== //

    struct GetLabelFn: RtnPkdHandlerBase<Int> {
        template <Int StreamIdx, typename Stream>
        Int stream(const Stream* stream, Int idx)
        {
            return stream->symbol(idx);
        }
    };


    Int label(Int idx, Int label_num) const
    {
        return LabelsDispatcher::dispatchRtn(label_num, this, GetLabelFn(), idx);
    }

    Int hidden_label(Int idx, Int label_num) const
    {
        return HiddenLabelsDispatcher::dispatchRtn(label_num, this, GetLabelFn(), idx);
    }


    struct SetLabelFn: RtnPkdHandlerBase<Int> {
        template <Int StreamIdx, typename Stream>
        Int stream(Stream* stream, Int idx, Int value)
        {
            Int old_value = stream->symbol(idx);

            stream->symbol(idx) = value;

            stream->reindex();

            return old_value;
        }
    };

    Int set_label(Int idx, Int label_num, Int label)
    {
        return LabelsDispatcher::dispatchRtn(label_num, this, SetLabelFn(), idx, label);
    }

    Int set_hidden_label(Int idx, Int label_num, Int label)
    {
        return HiddenLabelsDispatcher::dispatchRtn(label_num, this, SetLabelFn(), idx, label);
    }



    struct GetLabelBlockOffset
    {
        Int offset_ = 0;

        template <Int StreamIdx, typename Stream>
        void stream(Stream*, Int label_block)
        {
            if (StreamIdx < label_block)
            {
                offset_ += Stream::AlphabetSize;
            }
        }
    };

    static Int label_block_offset(Int label_num)
    {
        GetLabelBlockOffset offset_fn;

        HiddenLabelsDispatcher::dispatchAllStatic(offset_fn, 100);
        LabelsDispatcher::dispatchAllStatic(offset_fn, label_num);

        return offset_fn.offset_;
    }

    static Int hidden_label_block_offset(Int label_num)
    {
        GetLabelBlockOffset offset_fn;

        HiddenLabelsDispatcher::dispatchAllStatic(offset_fn, label_num);

        return offset_fn.offset_;
    }


    struct RankFn {

        using ResultType = Int;

        template <Int StreamIdx, typename Stream>
        ResultType stream(const Stream* stream, Int end, Int label)
        {
            return stream->rank(end, label);
        }

        template <Int StreamIdx, typename Stream>
        ResultType stream(const Stream* stream, Int start, Int end, Int label)
        {
            return stream->rank(start, end, label);
        }
    };


    BigInt h_rank(Int label_num, Int end, Int label) const
    {
        return HiddenLabelsDispatcher::dispatchRtn(label_num, this, RankFn(), end, label);
    }

    BigInt rank(Int label_num, Int end, Int label) const
    {
        return LabelsDispatcher::dispatchRtn(label_num, this, RankFn(), end, label);
    }


    BigInt h_rank(Int label_num, Int start, Int end, Int label) const
    {
        return HiddenLabelsDispatcher::dispatchRtn(label_num, this, RankFn(), start, end, label);
    }

    BigInt rank(Int label_num, Int start, Int end, Int label) const
    {
        return LabelsDispatcher::dispatchRtn(label_num, this, RankFn(), start, end, label);
    }




    struct SelectFwFn {

        using ResultType = SelectResult;

        template <Int StreamIdx, typename Stream>
        ResultType stream(const Stream* stream, Int start, Int symbol, BigInt rank)
        {
            return stream->selectFw(start, symbol, rank);
        }

        template <Int StreamIdx, typename Stream>
        ResultType stream(const Stream* stream, Int symbol, BigInt rank)
        {
            return stream->selectFw(symbol, rank);
        }
    };

    SelectResult h_selectFw(Int label_num, Int start, Int symbol, BigInt rank) const
    {
        return HiddenLabelsDispatcher::dispatchRtn(label_num, this, SelectFwFn(), start, symbol, rank);
    }

    SelectResult selectFw(Int label_num, Int start, Int symbol, BigInt rank) const
    {
        return LabelsDispatcher::dispatchRtn(label_num, this, SelectFwFn(), start, symbol, rank);
    }


    SelectResult h_selectFw(Int label_num, Int symbol, BigInt rank) const
    {
        return HiddenLabelsDispatcher::dispatchRtn(label_num, this, SelectFwFn(), symbol, rank);
    }

    SelectResult selectFw(Int label_num, Int symbol, BigInt rank) const
    {
        return LabelsDispatcher::dispatchRtn(label_num, this, SelectFwFn(), symbol, rank);
    }




    struct SelectBwFn {
        using ResultType = SelectResult;

        template <Int StreamIdx, typename Stream>
        ResultType stream(const Stream* stream, Int start, Int symbol, BigInt rank)
        {
            return stream->selectBw(start, symbol, rank);
        }

        template <Int StreamIdx, typename Stream>
        ResultType stream(const Stream* stream, Int symbol, BigInt rank)
        {
            return stream->selectBw(symbol, rank);
        }
    };


    SelectResult h_selectBw(Int label_num, Int start, Int symbol, BigInt rank) const
    {
        return HiddenLabelsDispatcher::dispatchRtn(label_num, this, SelectBwFn(), start, symbol, rank);
    }

    SelectResult selectBw(Int label_num, Int start, Int symbol, BigInt rank) const
    {
        return LabelsDispatcher::dispatchRtn(label_num, this, SelectBwFn(), start, symbol, rank);
    }


    SelectResult h_selectBw(Int label_num, Int symbol, BigInt rank) const
    {
        return HiddenLabelsDispatcher::dispatchRtn(label_num, this, SelectBwFn(), symbol, rank);
    }

    SelectResult selectBw(Int label_num, Int symbol, BigInt rank) const
    {
        return LabelsDispatcher::dispatchRtn(label_num, this, SelectBwFn(), symbol, rank);
    }


    template <typename Entry>
    struct GetEntryLabelsFn {
        template <Int LabelIdx, typename LabelsObj>
        void stream(const LabelsObj* labels, Int idx, Entry& entry)
        {
            std::get<LabelIdx>(entry.labels()) = labels->symbol(idx);
        }
    };

    template <typename Entry>
    struct GetEntryHiddenLabelsFn {
        template <Int LabelIdx, typename LabelsObj>
        void stream(const LabelsObj* labels, Int idx, Entry& entry)
        {
            std::get<LabelIdx>(entry.hidden_labels()) = labels->symbol(idx);
        }
    };

    template <typename Entry>
    void fillLabels(Int idx, Entry& entry) const
    {
        HiddenLabelsDispatcher::dispatchAll(this, GetEntryHiddenLabelsFn<Entry>(), idx, entry);
        LabelsDispatcher::dispatchAll(this, GetEntryLabelsFn<Entry>(), idx, entry);
    }


    template <typename T>
    struct Sums0Fn {
        Int idx_;
        Sums0Fn(Int start): idx_(start) {}

        template <Int StreamIdx, typename Stream>
        void stream(const Stream* stream, T& sums)
        {
            sums.sumAt(idx_, stream->sums());
            idx_ += Stream::Indexes;
        }
    };


    template <typename T>
    void sumsLabels(T& sums) const
    {
        Sums0Fn<T> fn(Blocks + 1);

        HiddenLabelsDispatcher::dispatchAll(this, fn, sums);
        LabelsDispatcher::dispatchAll(this, fn, sums);
    }


    template <typename T>
    struct Sums2Fn {
        Int idx_;
        Sums2Fn(Int start): idx_(start) {}

        template <Int StreamIdx, typename Stream>
        void stream(const Stream* stream, Int from, Int to, T& sums)
        {
            sums.sumAt(idx_, stream->sums(from, to));
            idx_ += Stream::AlphabetSize;
        }
    };


    template <typename T>
    void sumsLabels(Int from, Int to, T& sums) const
    {
        Sums2Fn<T> fn(Blocks + 1);

        HiddenLabelsDispatcher::dispatchAll(this, fn, from, to, sums);
        LabelsDispatcher::dispatchAll(this, fn, from, to, sums);
    }


    template <typename T>
    struct Sums1Fn {
        Int idx_;
        Sums1Fn(Int start): idx_(start) {}

        template <Int StreamIdx, typename Stream>
        void stream(const Stream* stream, Int idx, T& sums)
        {
            sums.sumAt(idx_, stream->sumsAt(idx));
            idx_ += Stream::AlphabetSize;
        }
    };

    template <typename T>
    void sumsLabels(Int idx, T& sums) const
    {
        Sums1Fn<T> fn(Blocks + 1);

        HiddenLabelsDispatcher::dispatchAll(this, fn, idx, sums);
        LabelsDispatcher::dispatchAll(this, fn, idx, sums);
    }

    // ============================ IO =============================================== //


    template <typename DataSource>
    struct InsertLabelsBatchFn {
    	DataSource* src_;
    	InsertLabelsBatchFn(DataSource* src): src_(src) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, SizeT start, Int idx, Int size)
    	{
    		src_->reset(start);

    		stream->insert(idx, size, [=](){
    			return std::get<StreamIdx>(src_->get().labels());
    		});
    	}
    };

    template <typename DataSource>
    struct InsertHiddenLabelsBatchFn {
    	DataSource* src_;
    	InsertHiddenLabelsBatchFn(DataSource* src): src_(src) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, SizeT start, Int idx, Int size)
    	{
    		src_->reset(start);

    		stream->insert(idx, size, [=](){
    			return std::get<StreamIdx>(src_->get().hidden_labels());
    		});
    	}
    };


    template <typename DataSource>
    void insertLabels(DataSource* src, SizeT start, Int idx, Int size)
    {
    	HiddenLabelsDispatcher::dispatchAll(this, InsertHiddenLabelsBatchFn<DataSource>(src), start, idx, size);
    	LabelsDispatcher::dispatchAll(this, InsertLabelsBatchFn<DataSource>(src), start, idx, size);
    }


    template <typename DataSource>
    struct UpdateLabelsBatchFn {
    	DataSource* src_;
    	UpdateLabelsBatchFn(DataSource* src): src_(src) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, SizeT pos, Int start, Int end)
    	{
    		src_->reset(pos);

    		stream->update(start, end, [=](){
    			return std::get<StreamIdx>(src_->get().labels());
    		});
    	}
    };

    template <typename DataSource>
    struct UpdateHiddenLabelsBatchFn {
    	DataSource* src_;
    	UpdateHiddenLabelsBatchFn(DataSource* src): src_(src) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, SizeT pos, Int start, Int end)
    	{
    		src_->reset(pos);

    		stream->update(start, end, [=](){
    			return std::get<StreamIdx>(src_->get().hidden_labels());
    		});
    	}
    };



    template <typename DataSource>
    void updateLabels(DataSource* src, SizeT pos, Int start, Int end)
    {
    	HiddenLabelsDispatcher::dispatchAll(this, UpdateHiddenLabelsBatchFn<DataSource>(src), pos, start, end);
    	LabelsDispatcher::dispatchAll(this, UpdateLabelsBatchFn<DataSource>(src), pos, start, end);
    }





    template <typename DataTarget>
    struct ReadLabelsBatchFn {
    	DataTarget* tgt_;
    	ReadLabelsBatchFn(DataTarget* tgt): tgt_(tgt) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, SizeT pos, Int start, Int end)
    	{
    		tgt_->reset(pos);
    		stream->read(start, end, [=](typename Stream::Value label)
    		{
    			auto current = tgt_->peek();
    			std::get<StreamIdx>(current.labels()) = label;
    			tgt_->put(current);
    		});
    	}
    };


    template <typename DataTarget>
    struct ReadHiddenLabelsBatchFn {
    	DataTarget* tgt_;
    	ReadHiddenLabelsBatchFn(DataTarget* tgt): tgt_(tgt) {}

    	template <Int StreamIdx, typename Stream>
    	void stream(const Stream* stream, SizeT pos, Int start, Int end)
    	{
    		tgt_->reset(pos);
    		stream->read(start, end, [=](typename Stream::Value label)
    		{
    			auto current = tgt_->peek();
    			std::get<StreamIdx>(current.hidden_labels()) = label;
    			tgt_->put(current);
    		});
    	}
    };



    template <typename DataTarget>
    void readLabels(DataTarget* tgt, SizeT pos, Int start, Int end) const
    {
    	HiddenLabelsDispatcher::dispatchAll(this, ReadHiddenLabelsBatchFn<DataTarget>(tgt), pos, start, end);
    	LabelsDispatcher::dispatchAll(this, ReadLabelsBatchFn<DataTarget>(tgt), pos, start, end);
    }



    // ============================ Serialization ==================================== //

    struct GenerateDataEventsFn {
        template <Int StreamIdx, typename Stream>
        void stream(const Stream* stream, IPageDataEventHandler* handler)
        {
            stream->generateDataEvents(handler);
        }
    };


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        HiddenLabelsDispatcher::dispatchAll(this, GenerateDataEventsFn(), handler);
        LabelsDispatcher::dispatchAll(this, GenerateDataEventsFn(), handler);
    }

    struct SerializeFn {
        template <Int StreamIdx, typename Stream>
        void stream(const Stream* stream, SerializationData& buf)
        {
            stream->serialize(buf);
        }
    };

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        HiddenLabelsDispatcher::dispatchAll(this, SerializeFn(), buf);
        LabelsDispatcher::dispatchAll(this, SerializeFn(), buf);
    }

    struct DeserializeFn {
        template <Int StreamIdx, typename Stream>
        void stream(Stream* stream, DeserializationData& buf)
        {
            stream->deserialize(buf);
        }
    };

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        HiddenLabelsDispatcher::dispatchAll(this, DeserializeFn(), buf);
        LabelsDispatcher::dispatchAll(this, DeserializeFn(), buf);
    }
};



}

#endif
