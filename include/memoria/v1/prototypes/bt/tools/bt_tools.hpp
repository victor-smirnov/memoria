
// Copyright 2012 Victor Smirnov
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



#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/vector_tuple.hpp>
#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/core/tools/tuple_dispatcher.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools_core.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_streamdescr_factory.hpp>




#include <ostream>
#include <tuple>

namespace memoria {
namespace v1 {
namespace bt {

template <Int StreamIdx> struct StreamTag {};

template <Int Streams, Int Idx = 0>
struct ForEachStream {
    template <typename Fn, typename... Args>
    static auto process(Int stream, Fn&& fn, Args&&... args)
    {
        if (stream == Idx)
        {
            return fn.template process<Idx>(std::forward<Args>(args)...);
        }
        else {
            return ForEachStream<Streams, Idx + 1>::process(stream, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }
};


template <Int Idx>
struct ForEachStream<Idx, Idx> {
    template <typename Fn, typename... Args>
    static auto process(Int stream, Fn&& fn, Args&&... args)
    {
        if (stream == Idx)
        {
            return fn.template process<Idx>(std::forward<Args>(args)...);
        }
        else {
            throw Exception(MA_SRC, SBuf()<<"Requested stream "<<stream<<" not found");
        }
    }
};





template <typename Types>
class PageUpdateManager {

    using MyType = PageUpdateManager<Types>;

    typedef Ctr<Types>                                                          CtrT;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef std::tuple<NodeBaseG, void*, Int>                                   TxnRecord;

    CtrT& ctr_;

    struct ClearFn {
        void operator()(TxnRecord& rec)
        {
            rec = TxnRecord(NodeBaseG(), nullptr, 0);
        }
    };

    core::StaticArray<TxnRecord, 4, ClearFn> pages_;

public:

    struct Remover {
        MyType& mgr_;
        const NodeBaseG& page_;

        Remover(MyType& mgr, const NodeBaseG& page):
            mgr_(mgr), page_(page)
        {}

        ~Remover() {
            mgr_.remove(page_);
        }
    };


    PageUpdateManager(CtrT& ctr): ctr_(ctr) {}

    ~PageUpdateManager() noexcept
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            void* backup_buffer = std::get<1>(pages_[c]);
            ctr_.allocator().freeMemory(backup_buffer);
        }
    }

    bool contains(const NodeBaseG& node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        for (Int c = pages_.getSize() - 1; c >= 0; c--)
        {
            if (node->id() == std::get<0>(pages_[c])->id())
            {
                return true;
            }
        }

        return false;
    }

    void add(NodeBaseG& node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        if (pages_.capacity() > 0)
        {
            Int page_size = node->page_size();

            void* backup_buffer = ctr_.allocator().allocateMemory(page_size);

            CopyByteBuffer(node.page(), backup_buffer, page_size);

            pages_.append(TxnRecord(node, backup_buffer, page_size));
        }
        else {
            throw Exception(MA_SRC, "No space left for new pages in the PageUpdateMgr");
        }
    }

    void remove(const NodeBaseG& node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        for (Int c = pages_.getSize() - 1; c >= 0; c--)
        {
            if (node->id() == std::get<0>(pages_[c])->id())
            {
                void* backup_buffer = std::get<1>(pages_[c]);
                ctr_.allocator().freeMemory(backup_buffer);

                pages_.remove(c);
            }
        }
    }

    void checkpoint(NodeBaseG& node)
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            if (std::get<0>(pages_[c])->id() == node->id())
            {
                void* backup_buffer = std::get<1>(pages_[c]);
                Int page_size       = std::get<2>(pages_[c]);

                CopyByteBuffer(node.page(), backup_buffer, page_size);

                return;
            }
        }
    }



    // FIXME: unify with rollback()
    void restoreNodeState()
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            NodeBaseG& node     = std::get<0>(pages_[c]);
            void* backup_buffer = std::get<1>(pages_[c]);
            Int page_size       = std::get<2>(pages_[c]);

            CopyByteBuffer(backup_buffer, node.page(), page_size);
        }
    }

    void rollback()
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            NodeBaseG& node     = std::get<0>(pages_[c]);
            void* backup_buffer = std::get<1>(pages_[c]);
            Int page_size       = std::get<2>(pages_[c]);

            CopyByteBuffer(backup_buffer, node.page(), page_size);

            ctr_.allocator().freeMemory(backup_buffer);
        }

        pages_.clear();
    }
};






template <typename Iterator, typename Container>
class BTreeIteratorPrefixCache {

    using IteratorPrefix = typename Container::Types::IteratorBranchNodeEntry;
    using SizePrefix = core::StaticVector<BigInt, Container::Types::Streams>;

    using MyType = BTreeIteratorPrefixCache<Iterator, Container>;

    IteratorPrefix  prefix_;
    IteratorPrefix  leaf_prefix_;
    SizePrefix      size_prefix_;

public:

    void reset() {
        prefix_ = IteratorPrefix();
        leaf_prefix_ = IteratorPrefix();
        size_prefix_ = SizePrefix();
    }

    const SizePrefix& size_prefix() const
    {
        return size_prefix_;
    }

    SizePrefix& size_prefix()
    {
        return size_prefix_;
    }

    const IteratorPrefix& prefixes() const
    {
        return prefix_;
    }

    IteratorPrefix& prefixes()
    {
        return prefix_;
    }

    const IteratorPrefix& leaf_prefixes() const
    {
        return leaf_prefix_;
    }

    IteratorPrefix& leaf_prefixes()
    {
        return leaf_prefix_;
    }

    void initState() {}

    bool operator==(const MyType& other) const
    {
        return prefix_ == other.prefix_ && leaf_prefix_ == other.leaf_prefix_ && size_prefix_ == other.size_prefix_;
    }

    bool operator!=(const MyType& other) const
    {
        return prefix_ != other.prefix_ || leaf_prefix_ != other.leaf_prefix_ || size_prefix_ != other.size_prefix_;
    }
};

template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const BTreeIteratorPrefixCache<I, C>& cache)
{
    out<<"IteratorPrefixCache[";
    out<<"Branch prefixes: "<<cache.prefixes()<<", Leaf Prefixes: "<<cache.leaf_prefixes()<<", Size Prefixes: "<<cache.size_prefix();
    out<<"]";

    return out;
}



template <Int...> struct Path;

template <Int Head, Int... Tail>
struct Path<Head, Tail...> {
    template <typename T>
    static auto get(T&& tuple)
    {
        return std::get<Head>(Path<Tail...>::get(tuple));
    }
};

template <Int Head>
struct Path<Head> {
    template <typename T>
    static auto get(T&& tuple) {
        return std::get<Head>(tuple);
    }
};



template<Int Idx>
struct TupleEntryAccessor {
    template <typename Entry>
    static auto get(Entry&& entry) {
        return std::get<Idx>(entry);
    }
};







template <typename T> struct DefaultBranchStructTF;

template <typename KeyType>
struct DefaultBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};

template <typename KeyType>
struct DefaultBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};

template <typename KeyType, Int Indexes>
struct DefaultBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, Indexes>>
{
    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    //FIXME: Extend KeyType to contain enough space to represent practically large sums
    //Should be done systematically on the level of BT

    using Type = IfThenElse <
            HasFieldFactory<KeyType>::Value,
            PkdFQTreeT<KeyType, Indexes>,
            PkdVQTreeT<KeyType, Indexes>
    >;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, Int Indexes>
struct DefaultBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    static_assert(
            IsExternalizable<KeyType>::Value,
            "Type must either has ValueCodec or FieldFactory defined"
    );

    using Type = IfThenElse<
            HasFieldFactory<KeyType>::Value,
            PkdFMOTreeT<KeyType, Indexes>,
            PkdVBMTreeT<KeyType>
    >;

    static_assert(IndexesSize<Type>::Value == Indexes, "Packed struct has different number of indexes than requested");
};



template <typename Fn>
class SubstreamReadLambdaAdapter {
    Fn& fn_;
public:
    SubstreamReadLambdaAdapter(Fn& fn): fn_(fn) {}

    template <Int StreamIdx, Int SubstreamIdx, typename T>
    void put(const StreamTag<StreamIdx>&, const StreamTag<SubstreamIdx>&, T&& value)
    {
        fn_(value);
    }
};

template <typename Fn>
class SubstreamReadLambdaAdapter<Fn&&> {
    Fn&& fn_;
public:
    SubstreamReadLambdaAdapter(Fn&& fn): fn_(fn) {}

    template <Int StreamIdx, Int SubstreamIdx, typename T>
    void put(const StreamTag<StreamIdx>&, const StreamTag<SubstreamIdx>&, T&& value)
    {
        fn_(value);
    }
};

template <typename Fn>
class SubstreamReadLambdaAdapter<const Fn&> {
    const Fn& fn_;
public:
    SubstreamReadLambdaAdapter(const Fn& fn): fn_(fn) {}

    template <Int StreamIdx, Int SubstreamIdx, typename T>
    void put(const StreamTag<StreamIdx>&, const StreamTag<SubstreamIdx>&, T&& value)
    {
        fn_(value);
    }
};



template <typename Profile, typename CtrName>
struct ContainerExtensionsTF {
	using Type = TL<>;
};

template <typename Profile, typename CtrName>
struct IteratorExtensionsTF {
	using Type = TL<>;
};



template <Int Size, Int Idx = 0>
struct ForAllTuple {
	template <typename InputBuffer, typename Fn, typename... Args>
	static void process(InputBuffer&& tuple, Fn&& fn, Args&&... args)
	{
		fn.template process<Idx>(std::get<Idx>(tuple), std::forward<Args>(args)...);
		ForAllTuple<Size, Idx + 1>::process(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
	}
};

template <Int Idx>
struct ForAllTuple<Idx, Idx> {
	template <typename InputBuffer, typename Fn, typename... Args>
	static void process(InputBuffer&& tuple, Fn&& fn, Args&&... args)
	{}
};


template <typename IOBuffer>
struct BufferConsumer {
	virtual IOBuffer& buffer() = 0;
	virtual Int process(IOBuffer& buffer, Int entries) = 0;

	virtual ~BufferConsumer() noexcept {}
};

template <typename IOBuffer>
struct BufferProducer {
	virtual IOBuffer& buffer() = 0;
	virtual Int populate(IOBuffer& buffer) = 0;

	virtual ~BufferProducer() noexcept {}
};



template <Int Stream, typename CtrSizeT>
class EntryFnBase {
	CtrSizeT one_;
public:
	EntryFnBase(): one_(1) {}

	const auto& get(const StreamTag<Stream>& , const StreamTag<0>&, Int block) const
	{
		return one_;
	}
};


template <Int Stream, typename T, typename CtrSizeT>
struct SingleValueEntryFn: EntryFnBase<Stream, CtrSizeT> {

	using EntryFnBase<Stream, CtrSizeT>::get;

	const T& value_;

	SingleValueEntryFn(const T& value): value_(value) {}

	const auto& get(const StreamTag<Stream>& , const StreamTag<1>&, Int block) const
	{
		return value_;
	}
};



}
}}
