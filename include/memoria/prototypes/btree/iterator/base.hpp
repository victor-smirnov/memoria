
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_BASE_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_BASE_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/btree/names.hpp>
#include <memoria/prototypes/btree/macros.hpp>

#include <memoria/core/tools/hash.hpp>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_BTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBase)
public:
	typedef typename Base::Container::Types                                        		Types;
	typedef typename Base::Container::TreePath                                        	TreePath;
	typedef typename Base::Container::TreePath::Element                               	TreePathItem;
    typedef typename Base::Container::NodeBase                                        	NodeBase;
    typedef typename Base::Container::NodeBaseG                                       	NodeBaseG;
    typedef typename Base::Container::Allocator										  	Allocator;

    typedef typename Types::template IteratorCacheFactory<
    		MyType,
    		typename Base::Container
    >::Type																				IteratorCache;

    static const Int Indexes															= Base::Container::Indexes;

private:

    TreePath           	path_;
    Int                 key_idx_;

    bool				found_;

    IteratorCache 		cache_;

public:
    BTreeIteratorBase(): Base(), path_(), key_idx_(0)
    {
    	cache_.init(me());
    }

    BTreeIteratorBase(ThisType&& other): Base(std::move(other)), path_(std::move(other.path_)), key_idx_(other.key_idx_), cache_(std::move(other.cache_))
    {
    	cache_.init(me());
    }

    BTreeIteratorBase(const ThisType& other): Base(other), path_(other.path_), key_idx_(other.key_idx_), cache_(other.cache_)
    {
    	cache_.init(me());
    }

    void Assign(ThisType&& other)
    {
        path_       = other.path_;
        key_idx_    = other.key_idx_;
        found_		= other.found_;

        cache_		= other.cache_;

        cache_.init(me());

        Base::Assign(std::move(other));
    }

    void Assign(const ThisType& other)
    {
    	path_       = other.path_;
    	key_idx_    = other.key_idx_;
    	found_		= other.found_;

    	cache_		= other.cache_;

    	cache_.init(me());

    	Base::Assign(other);
    }

    bool IsFound() const {
    	return found_;
    }

    void setFound(bool found)
    {
    	found_ = found;
    }

    bool IsEqual(const ThisType& other) const
    {
    	return page() == other.page() && key_idx_ == other.key_idx_ && Base::IsEqual(other);
    }

    bool IsNotEqual(const ThisType& other) const
    {
    	return page() != other.page() || key_idx_ != other.key_idx_ || Base::IsNotEqual(other);
    }

    void setNode(NodeBaseG& node, Int parent_idx)
    {
    	path_[node->level()].node() 		= node;
    	path_[node->level()].parent_idx() 	= parent_idx;
    }

    Int &key_idx()
    {
        return key_idx_;
    }

    const Int key_idx() const
    {
        return key_idx_;
    }

    NodeBaseG& page()
    {
        return path_.leaf().node();
    }

    const NodeBaseG& page() const
    {
    	return path_.leaf().node();
    }

    TreePathItem& leaf()
    {
    	return path_.leaf();
    }

    const TreePathItem& leaf() const
    {
    	return path_.leaf();
    }

    TreePath& path()
    {
    	return path_;
    }

    const TreePath& path() const
    {
    	return path_;
    }

    IteratorCache& cache() {
    	return cache_;
    }

    const IteratorCache& cache() const {
    	return cache_;
    }


    bool IsBegin() const
    {
    	return key_idx() < 0;
    }

    bool IsEnd() const
    {
    	return page().is_set() ? key_idx() >= page()->children_count() : true;
    }

    bool IsNotEnd() const
    {
    	return !IsEnd();
    }

    bool IsEmpty() const
    {
    	return page().is_empty() || page()->children_count() == 0;
    }

    bool IsNotEmpty() const
    {
    	return !IsEmpty();
    }

    BigInt KeyNum() const
    {
    	return cache_.key_num();
    }

    BigInt& KeyNum()
    {
    	return cache_.key_num();
    }


    void Dump(ostream& out = cout, const char* header = NULL)
    {
    	out<<(header != NULL ? header : me()->getDumpHeader())<<endl;

    	me()->dumpKeys(out);

    	me()->DumpBeforePath(out);
    	me()->dumpPath(out);

    	me()->DumpBeforePages(out);
    	me()->dumpPages(out);
    }

    String getDumpHeader()
    {
    	return String(me()->model().type_name()) + " Iterator State";
    }

    void dumpPath(ostream& out)
    {
    	out<<"Path:"<<endl;

    	TreePath& path0 = me()->path();
    	for (int c = me()->path().getSize() - 1; c >= 0; c--)
    	{
    		out<<"Node("<<c<<"): "<<IDValue(path0[c]->id())<<" idx="<<(c > 0 ? ToString(path0[c - 1].parent_idx()) : "")<<endl;
    	}
    }

    void dumpKeys(ostream& out)
    {
    	out<<"KeyIdx:  "<<me()->key_idx()<<endl;
    }

    void DumpBeforePath(ostream& out){}
    void DumpBeforePages(ostream& out){}

    void dumpPages(ostream& out)
    {
    	me()->model().Dump(me()->leaf().node(), out);
    }

    void init()
    {
    	Base::init();
    	cache_.initState();
    }

MEMORIA_BTREE_ITERATOR_BASE_CLASS_END

} //memoria



#endif
