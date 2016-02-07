
// Copyright Victor Smirnov 2012-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/vector_tuple.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/tools/tuple_dispatcher.hpp>

#include <memoria/prototypes/bt/tools/bt_tools_core.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_packed_struct_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_streamdescr_factory.hpp>
#include <ostream>
#include <tuple>

namespace memoria   {
namespace bt        {


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
			throw vapi::Exception(MA_SRC, SBuf()<<"Requested stream "<<stream<<" not found");
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

    StaticArray<TxnRecord, 4, ClearFn> pages_;

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

    ~PageUpdateManager() throw()
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            void* backup_buffer = std::get<1>(pages_[c]);
            ctr_.allocator().freeMemory(backup_buffer);
        }
    }

    bool contains(const NodeBaseG& node)
    {
    	MEMORIA_ASSERT_TRUE(node.isSet());

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
    	MEMORIA_ASSERT_TRUE(node.isSet());

//    	cout<<"Register node state for "<<node->id()<<" "<<node->parent_id()<<" "<<node->parent_idx()<<endl;

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
    	MEMORIA_ASSERT_TRUE(node.isSet());

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
//    	cout<<"Checkpoint node state for "<<node->id()<<" "<<node->parent_id()<<" "<<node->parent_idx()<<endl;

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

//            cout<<"Restore (1) node state for "<<node->id()<<" "<<node->parent_id()<<" "<<node->parent_idx()<<endl;

            CopyByteBuffer(backup_buffer, node.page(), page_size);

//            cout<<"Restore (2) node state for "<<node->id()<<" "<<node->parent_id()<<" "<<node->parent_idx()<<endl;
        }
    }

    void rollback()
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            NodeBaseG& node     = std::get<0>(pages_[c]);
            void* backup_buffer = std::get<1>(pages_[c]);
            Int page_size       = std::get<2>(pages_[c]);

//            cout<<"Rollback node state for "<<node->id()<<" "<<node->parent_id()<<" "<<node->parent_idx()<<endl;

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

    IteratorPrefix 	prefix_;
    IteratorPrefix 	leaf_prefix_;
    SizePrefix		size_prefix_;

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



//template <Int Offset, bool StreamStart>
//struct BatchEntryMaxHelper {
//	template <typename T1, typename T2>
//	static void assign_max(T1&& accum, T2&&obj)
//	{
//		obj->template max<Offset>(accum);
//	}
//
//	template <typename T1, typename T2>
//	static void remove_entry(T1&& accum, T2&&obj, Int idx)
//	{
//		obj->template _remove<Offset>(idx, accum);
//	}
//
//	template <typename T1, typename T2, typename T3>
//	static void insert_entry(T1&& accum, T2&& entry, T3&&obj, Int idx)
//	{
//		obj->template _insert<Offset>(idx, entry, accum);
//	}
//};
//
//template <>
//struct BatchEntryMaxHelper<0, true> {
//	template <typename T1, typename T2>
//	static void assign_max(T1&& accum, T2&& obj)
//	{
//		accum[0] = obj->size();
//	}
//
//	template <typename T1, typename T2>
//	static void remove_entry(T1&& accum, T2&&obj, Int idx)
//	{
//		accum[0] -= 1;
//	}
//
//	template <typename T1, typename T2, typename T3>
//	static void insert_entry(T1&& accum, T2&& entry, T3&&obj, Int idx)
//	{
//		accum[0] += 1;
//	}
//};
//
//template <>
//struct BatchEntryMaxHelper<1, true> {
//	template <typename T1, typename T2>
//	static void assign_max(T1&& accum, T2&& obj)
//	{
//		accum[0] = obj->size();
//		obj->template max<1>(accum);
//	}
//
//	template <typename T1, typename T2>
//	static void remove_entry(T1&& accum, T2&&obj, Int idx)
//	{
//		obj->template _remove<1>(idx, accum);
//		accum[0] -= 1;
//	}
//
//	template <typename T1, typename T2, typename T3>
//	static void insert_entry(T1&& accum, T2&& entry, T3&&obj, Int idx)
//	{
//		obj->template _insert<1>(idx, entry, accum);
//		accum[0] += 1;
//	}
//};


}
}

#endif

