
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
#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/v1/prototypes/bt/bt_names.hpp>
#include <memoria/v1/core/tools/tuple_dispatcher.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools_core.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools_streamdescr_factory.hpp>

#include <memoria/v1/core/packed/packed.hpp>

#include <ostream>
#include <tuple>

namespace memoria {
namespace v1 {
namespace bt {

template <int32_t StreamIdx> struct StreamTag {};

template <int32_t Streams, int32_t Idx = 0>
struct ForEachStream {
    template <typename Fn, typename... Args>
    static auto process(int32_t stream, Fn&& fn, Args&&... args)
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


template <int32_t Idx>
struct ForEachStream<Idx, Idx> {
    template <typename Fn, typename... Args>
    static auto process(int32_t stream, Fn&& fn, Args&&... args)
    {
        if (stream == Idx)
        {
            return fn.template process<Idx>(std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(format_u8("Requested stream {} not found", stream));
        }
    }
};





template <typename Types>
class BlockUpdateManager {

    using MyType = BlockUpdateManager<Types>;

    typedef Ctr<Types>                                                          CtrT;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef std::tuple<NodeBaseG, void*, int32_t>                                   TxnRecord;

    CtrT& ctr_;

    struct ClearFn {
        void operator()(TxnRecord& rec)
        {
            rec = TxnRecord(NodeBaseG(), nullptr, 0);
        }
    };

    core::StaticArray<TxnRecord, 4, ClearFn> blocks_;

public:

    struct Remover {
        MyType& mgr_;
        const NodeBaseG& block_;

        Remover(MyType& mgr, const NodeBaseG& block):
            mgr_(mgr), block_(block)
        {}

        ~Remover() {
            mgr_.remove(block_);
        }
    };


    BlockUpdateManager(CtrT& ctr): ctr_(ctr) {}

    ~BlockUpdateManager() noexcept
    {
        for (int32_t c = 0; c < blocks_.getSize(); c++)
        {
            void* backup_buffer = std::get<1>(blocks_[c]);
            ctr_.store().freeMemory(backup_buffer);
        }
    }

    bool contains(const NodeBaseG& node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        for (int32_t c = blocks_.getSize() - 1; c >= 0; c--)
        {
            if (node->id() == std::get<0>(blocks_[c])->id())
            {
                return true;
            }
        }

        return false;
    }

    void add(NodeBaseG& node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        if (blocks_.capacity() > 0)
        {
            int32_t block_size = node->header().memory_block_size();

            void* backup_buffer = ctr_.store().allocateMemory(block_size).get_or_terminate();

            CopyByteBuffer(node.block(), backup_buffer, block_size);

            blocks_.append(TxnRecord(node, backup_buffer, block_size));
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("No space left for new blocks in the BlockUpdateMgr");
        }
    }

    void remove(const NodeBaseG& node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());

        for (int32_t c = blocks_.getSize() - 1; c >= 0; c--)
        {
            if (node->id() == std::get<0>(blocks_[c])->id())
            {
                void* backup_buffer = std::get<1>(blocks_[c]);
                ctr_.store().freeMemory(backup_buffer);

                blocks_.remove(c);
            }
        }
    }

    void checkpoint(NodeBaseG& node)
    {
        for (int32_t c = 0; c < blocks_.getSize(); c++)
        {
            if (std::get<0>(blocks_[c])->id() == node->id())
            {
                void* backup_buffer = std::get<1>(blocks_[c]);
                int32_t block_size       = std::get<2>(blocks_[c]);

                CopyByteBuffer(node.block(), backup_buffer, block_size);

                return;
            }
        }
    }



    // FIXME: unify with rollback()
    void restoreNodeState()
    {
        for (int32_t c = 0; c < blocks_.getSize(); c++)
        {
            NodeBaseG& node     = std::get<0>(blocks_[c]);
            void* backup_buffer = std::get<1>(blocks_[c]);
            int32_t block_size       = std::get<2>(blocks_[c]);

            CopyByteBuffer(backup_buffer, node.block(), block_size);
        }
    }

    void rollback()
    {
        for (int32_t c = 0; c < blocks_.getSize(); c++)
        {
            NodeBaseG& node     = std::get<0>(blocks_[c]);
            void* backup_buffer = std::get<1>(blocks_[c]);
            int32_t block_size       = std::get<2>(blocks_[c]);

            CopyByteBuffer(backup_buffer, node.block(), block_size);

            ctr_.store().freeMemory(backup_buffer);
        }

        blocks_.clear();
    }
};






template <typename Iterator, typename Container>
class BTreeIteratorPrefixCache {

    using IteratorPrefix = typename Container::Types::IteratorBranchNodeEntry;
    using SizePrefix = core::StaticVector<int64_t, Container::Types::Streams>;

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
std::ostream& operator<<(std::ostream& out, const BTreeIteratorPrefixCache<I, C>& iter_cache)
{
    out<<"IteratorPrefixCache[";
    out<<"Branch prefixes: "<<iter_cache.prefixes()<<", Leaf Prefixes: "<<iter_cache.leaf_prefixes()<<", Size Prefixes: "<<iter_cache.size_prefix();
    out<<"]";

    return out;
}



template <int32_t...> struct Path;

template <int32_t Head, int32_t... Tail>
struct Path<Head, Tail...> {
    template <typename T>
    static auto get(T&& tuple)
    {
        return std::get<Head>(Path<Tail...>::get(tuple));
    }
};

template <int32_t Head>
struct Path<Head> {
    template <typename T>
    static auto get(T&& tuple) {
        return std::get<Head>(tuple);
    }
};



template<int32_t Idx>
struct TupleEntryAccessor {
    template <typename Entry>
    static auto get(Entry&& entry) {
        return std::get<Idx>(entry);
    }
};











template <typename Fn>
class SubstreamReadLambdaAdapter {
    Fn& fn_;
public:
    SubstreamReadLambdaAdapter(Fn& fn): fn_(fn) {}

    template <int32_t StreamIdx, int32_t SubstreamIdx, typename T>
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

    template <int32_t StreamIdx, int32_t SubstreamIdx, typename T>
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

    template <int32_t StreamIdx, int32_t SubstreamIdx, typename T>
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





template <int32_t Stream, typename CtrSizeT>
class EntryFnBase {
    CtrSizeT one_;
public:
    EntryFnBase(): one_(1) {}

    const auto& get(const StreamTag<Stream>& , const StreamTag<0>&, int32_t block) const
    {
        return one_;
    }
};


template <int32_t Stream, typename T, typename CtrSizeT>
struct SingleValueEntryFn: EntryFnBase<Stream, CtrSizeT> {

    using EntryFnBase<Stream, CtrSizeT>::get;

    const T& value_;

    SingleValueEntryFn(const T& value): value_(value) {}

    const auto& get(const StreamTag<Stream>& , const StreamTag<1>&, int32_t block) const
    {
        return value_;
    }
};

namespace _ {

    template <
        bool Selector,
        template <typename> class PkdStruct1,
        template <typename> class PkdStruct2,
        typename Types1,
        typename Types2
    >
    struct PkdStructSelectorH;

    template <
        template <typename> class PkdStruct1,
        template <typename> class PkdStruct2,
        typename Types1,
        typename Types2
    >
    struct PkdStructSelectorH<true, PkdStruct1, PkdStruct2, Types1, Types2>: HasType<PkdStruct1<Types1>> {};


    template <
        template <typename> class PkdStruct1,
        template <typename> class PkdStruct2,
        typename Types1,
        typename Types2
    >
    struct PkdStructSelectorH<false, PkdStruct1, PkdStruct2, Types1, Types2>: HasType<PkdStruct2<Types2>> {};
}

template <
    bool Selector,
    template <typename> class PkdStruct1,
    template <typename> class PkdStruct2,
    typename Types1,
    typename Types2
>
using PkdStructSelector = typename _::PkdStructSelectorH<Selector, PkdStruct1, PkdStruct2, Types1, Types2>::Type;



template <typename T> struct DefaultBranchStructTF;

template <typename KeyType>
struct DefaultBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::MAX>;
};

template <typename KeyType>
struct DefaultBranchStructTF<IdxSearchType<PkdSearchType::SUM, KeyType, 0>> {
    using Type = PackedEmptyStruct<KeyType, PkdSearchType::SUM>;
};


template <typename KeyType, int32_t Indexes>
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

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};

template <typename KeyType, int32_t Indexes>
struct DefaultBranchStructTF<IdxSearchType<PkdSearchType::MAX, KeyType, Indexes>> {

    static_assert(Indexes <= 1, "");

    using Type = PackedDataTypeBufferT<KeyType, Indexes == 1>;

    static_assert(PkdStructIndexes<Type> == Indexes, "Packed struct has different number of indexes than requested");
};




}

}}
