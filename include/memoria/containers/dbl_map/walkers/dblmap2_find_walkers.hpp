
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_DBLMAP2_FIND_WALKERS_HPP
#define _MEMORIA_CONTAINERS_DBLMAP2_FIND_WALKERS_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/container/container.hpp>

#include <memoria/prototypes/bt/bt_walkers.hpp>

#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <ostream>

namespace memoria       {
namespace dblmap		{


namespace outer 		{


template <typename Types>
class FindGTWalker: public bt::FindWalkerBase<Types, FindGTWalker<Types>> {

    typedef bt::FindWalkerBase<Types, FindGTWalker<Types>>   	Base;
    typedef typename Base::Key          						Key;
    typedef typename Base::Iterator         					Iterator;

    typedef typename Types::Accumulator	Accumulator;

    Accumulator prefix_;

public:
    FindGTWalker(Int stream, Int key_num, Key key): Base(stream, key_num, key)
    {}

    FindGTWalker(Int stream, Int key_num, Int index1, Key key): Base(stream, key_num, key)
    {}

    typedef Int ResultType;
    typedef Int ReturnType;

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        return node->find(Base::stream_, *this, node->level(), start);
    }

    template <Int Idx, typename Tree>
    Int stream(const Tree* tree, Int level, Int start)
    {
        Int index		= Base::index();

        auto target     = Base::target_- std::get<Idx>(prefix_)[index];

        auto result     = tree->findGTForward(index, start, target);

        tree->sums(start, result.idx(), std::get<Idx>(prefix_));

        return result.idx();
    }

    void prepare(Iterator& iter)
    {
    	prefix_ = iter.prefixes();
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;
        iter.cache().setup(prefix_);
        return idx;
    }
};



template <typename Types>
class FindGEWalker: public bt::FindWalkerBase<Types, FindGEWalker<Types>> {

    typedef bt::FindWalkerBase<Types, FindGEWalker<Types>>   	Base;
    typedef typename Base::Key          						Key;
    typedef typename Base::Iterator         					Iterator;

    typedef typename Types::Accumulator	Accumulator;

    Accumulator prefix_;

public:
    FindGEWalker(Int stream, Int key_num, Key key): Base(stream, key_num, key)
    {}

    FindGEWalker(Int stream, Int key_num, Int index1, Key key): Base(stream, key_num, key)
    {}

    typedef Int ResultType;
    typedef Int ReturnType;

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        return node->find(Base::stream_, *this, node->level(), start);
    }

    template <Int Idx, typename Tree>
    Int stream(const Tree* tree, Int level, Int start)
    {
        Int index		= Base::index();

        auto target     = Base::target_- std::get<Idx>(prefix_)[index];

        auto result     = tree->findGEForward(index, start, target);

        tree->sums(start, result.idx(), std::get<Idx>(prefix_));

        return result.idx();
    }

    void prepare(Iterator& iter)
    {
    	prefix_ = iter.prefixes();
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;
        iter.cache().setup(prefix_);
        return idx;
    }
};
}





namespace inner 		{


template <typename Types>
class FindGTWalker: public bt::FindWalkerBase<Types, FindGTWalker<Types>> {

    typedef bt::FindWalkerBase<Types, FindGTWalker<Types>>   	Base;
    typedef typename Base::Key          						Key;
    typedef typename Base::Iterator         					Iterator;

    typedef typename Types::Accumulator	Accumulator;

    Accumulator prefix_;

public:
    FindGTWalker(Int stream, Int key_num, Key key): Base(stream, key_num, key)
    {}

    FindGTWalker(Int stream, Int key_num, Int index1, Key key): Base(stream, key_num, key)
    {}

    typedef Int ResultType;
    typedef Int ReturnType;

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        return node->find(Base::stream_, *this, node->level(), start);
    }

    template <Int Idx, typename TreeTypes>
    Int stream(const PkdFTree<TreeTypes>* tree, Int level, Int start)
    {
        Int index		= Base::index();

        auto target     = Base::target_- std::get<Idx>(prefix_)[index];

        auto result     = tree->findGTForward(index, start, target);

        tree->sums(start, result.idx(), std::get<Idx>(prefix_));

        Base::sum_ += result.prefix();

        return result.idx();
    }



    template <Int Idx, typename MapTypes>
    Int stream(const PackedFSEMarkableMap<MapTypes>* tree, Int level, Int start)
    {
    	Int index		= Base::index();
    	auto& sum 		= Base::sum_;

    	if (index > 0)
    	{
    		auto target     = Base::target_- std::get<Idx>(prefix_)[index];

    		auto result     = tree->findGTForward(index - 1, start, target);

    		tree->sums(start, result.idx(), std::get<Idx>(prefix_));

    		sum += result.prefix();

    		return result.idx();
    	}
    	else {
    		BigInt offset = Base::target_ - sum;

    		Int size = tree->size();

    		if (start + offset < size)
    		{
    			sum += offset;

    			tree->sums(start, start + offset, std::get<Idx>(prefix_));

    			return start + offset;
    		}
    		else {
    			tree->sums(start, size, std::get<Idx>(prefix_));

    			sum += (size - start);

    			return size;
    		}
    	}
    }

    void prepare(Iterator& iter)
    {
    	prefix_ = iter.cache().prefixes();
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;
        iter.cache().setup(prefix_);
        return Base::sum_;
    }
};



template <typename Types>
class FindGEWalker: public bt::FindWalkerBase<Types, FindGEWalker<Types>> {

    typedef bt::FindWalkerBase<Types, FindGEWalker<Types>>   	Base;
    typedef typename Base::Key          						Key;
    typedef typename Base::Iterator         					Iterator;

    typedef typename Types::Accumulator	Accumulator;

    Accumulator prefix_;

public:
    FindGEWalker(Int stream, Int key_num, Key key): Base(stream, key_num, key)
    {}

    FindGEWalker(Int stream, Int key_num, Int index1, Key key): Base(stream, key_num, key)
    {}

    typedef Int ResultType;
    typedef Int ReturnType;

    template <typename Node>
    ReturnType treeNode(const Node* node, Int start)
    {
        return node->find(Base::stream_, *this, node->level(), start);
    }

    template <Int Idx, typename TreeTypes>
    Int stream(const PkdFTree<TreeTypes>* tree, Int level, Int start)
    {
        Int index		= Base::index();

        auto target     = Base::target_- std::get<Idx>(prefix_)[index];

        auto result     = tree->findGEForward(index, start, target);

        Base::sum_ 		+= result.prefix();

        tree->sums(start, result.idx(), std::get<Idx>(prefix_));

        return result.idx();
    }

    template <Int Idx, typename MapTypes>
    Int stream(const PackedFSEMarkableMap<MapTypes>* tree, Int level, Int start)
    {
    	Int index		= Base::index();
    	auto& sum 		= Base::sum_;

    	MEMORIA_ASSERT_TRUE(index > 0);

    	auto target     = Base::target_- std::get<Idx>(prefix_)[index];

    	auto result     = tree->findGEForward(index - 1, start, target);

    	tree->sums(start, result.idx(), std::get<Idx>(prefix_));

    	sum += result.prefix();

    	return result.idx();
    }


    void prepare(Iterator& iter)
    {
    	prefix_ = iter.cache().prefixes();
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;
        iter.cache().setup(prefix_);
        return Base::sum_;
    }
};
}





}
}

#endif
