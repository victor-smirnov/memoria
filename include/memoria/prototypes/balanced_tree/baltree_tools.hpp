
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/vector_tuple.hpp>

#include <ostream>
#include <tuple>

namespace memoria       {
namespace balanced_tree {

using namespace memoria::core;

template <typename Node>
class TreePathItem {
    typedef TreePathItem<Node> MyType;
    typedef typename Node::Page Page;

    Node node_;
    Int parent_idx_;
public:

    TreePathItem(): node_(), parent_idx_(0) {}

    TreePathItem(Node& node, Int parent_idx = 0): node_(node), parent_idx_(parent_idx) {}

    TreePathItem(const MyType& other): node_(other.node_), parent_idx_(other.parent_idx_) {}
    TreePathItem(MyType&& other): node_(std::move(other.node_)), parent_idx_(other.parent_idx_) {}

    Page* operator->()
    {
        return node_.operator->();
    }

    const Page* operator->() const
    {
        return node_.operator->();
    }


    MyType& operator=(const MyType& other)
    {
        node_       = other.node_;
        parent_idx_ = other.parent_idx_;

        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        node_       = std::move(other.node_);
        parent_idx_ = other.parent_idx_;

        return *this;
    }

    bool operator==(const MyType& other) const
    {
        return node_ == other.node_;
    }

    bool operator!=(const MyType& other) const
    {
        return node_ != other.node_;
    }

    operator Node& () {
        return node_;
    }

    operator const Node& () const {
        return node_;
    }

    operator Node () const {
        return node_;
    }

    const Node& node() const {
        return node_;
    }

    Node& node() {
        return node_;
    }

    const Int& parent_idx() const {
        return parent_idx_;
    }

    Int& parent_idx() {
        return parent_idx_;
    }

    void clear() {
        node_.clear();
        parent_idx_ = 0;
    }
};


struct Valueclearing {
    template <typename Value>
    void operator()(Value& value)
    {
        value.clear();
    }
};






template <
    typename NodePage,
    Int Size = 8>
class NodePath: public StaticArray<TreePathItem<NodePage>, Size, Valueclearing> {

    typedef StaticArray<TreePathItem<NodePage>, Size, Valueclearing>    Base;
    typedef NodePath<NodePage, Size>                                    MyType;

public:

    typedef TreePathItem<NodePage>                                      PathItem;

    NodePath(): Base() {}

    NodePath(const MyType& other): Base(other) {}

    NodePath(MyType&& other): Base(std::move(other)) {}

    MyType& operator=(const MyType& other)
    {
        Base::operator=(other);
        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        Base::operator=(std::move(other));
        return *this;
    }

    void moveRight(Int level, Int from, Int count)
    {
        if (level >= 0)
        {
            PathItem& item = Base::operator[](level);
            if (item.parent_idx() >= from)
            {
                item.parent_idx() += count;
            }
        }
    }

    void moveLeft(Int level, Int from, Int count)
    {
        if (level >= 0)
        {
            PathItem& item = Base::operator[](level);

            if (item.parent_idx() >= from)
            {
                item.parent_idx() -= count;
            }
            else if (item.parent_idx() >= from)
            {
                for (Int c = level; c >= 0; c--)
                {
                    Base::operator[](c).clear();
                }
            }
        }
    }

    PathItem& leaf()
    {
        return this->operator[](0);
    }

    const PathItem& leaf() const
    {
        return this->operator[](0);
    }
};


template <typename Iterator>
class IteratorCacheBase {
    Iterator* iter_;
public:
    IteratorCacheBase() {}

    void init(Iterator* i)
    {
        iter_ = i;
    }

    void initState() {}


    void Prepare()              {}
    void nextKey(bool end)      {}
    void prevKey(bool start)    {}

    const Iterator& iterator() const {
        return *iter_;
    }

    Iterator& iterator() {
        return *iter_;
    }

    void setup(BigInt, Int) {}
};


template <typename Iterator, typename Container>
class BTreeIteratorCache: public IteratorCacheBase<Iterator> {
    typedef IteratorCacheBase<Iterator> Base;

    BigInt key_num_;
public:

    BTreeIteratorCache(): Base(), key_num_(0) {}

    BigInt& key_num()
    {
        return key_num_;
    }

    const BigInt& key_num() const
    {
        return key_num_;
    }
};


template <typename Iterator, typename Container>
class BTreeIteratorPrefixCache: public BTreeIteratorCache<Iterator, Container> {
    typedef BTreeIteratorCache<Iterator, Container> Base;
    typedef typename Container::Accumulator     Accumulator;

    Accumulator prefix_;
    Accumulator current_;

    static const Int Indexes = 1;

public:

    BTreeIteratorPrefixCache(): Base(), prefix_(), current_() {}

    const BigInt& prefix(int num = 0) const
    {
        return get<0>(prefix_)[num];
    }

    const Accumulator& prefixes() const
    {
        return prefix_;
    }

    void nextKey(bool end)
    {
        VectorAdd(prefix_, current_);

        Clear(current_);
    };

    void prevKey(bool start)
    {
        VectorSub(prefix_, current_);

        Clear(current_);
    };

    void Prepare()
    {
        if (Base::iterator().key_idx() >= 0)
        {
            current_ = Base::iterator().getRawKeys();
        }
        else {
            Clear(current_);
        }
    }

    void setup(BigInt prefix, Int key_num)
    {
        get<0>(prefix_)[key_num] = prefix;

        init_(key_num);
    }

    void setup(const Accumulator& prefix)
    {
        prefix_ = prefix;
    }

    void initState()
    {
        Clear(prefix_);

        Int idx  = Base::iterator().key_idx();

        if (idx >= 0)
        {
            typedef typename Iterator::Container::TreePath TreePath;
            const TreePath& path = Base::iterator().path();

            for (Int c = 0; c < path.getSize(); c++)
            {
                Base::iterator().model().sumKeys(path[c].node(), 0, idx, prefix_);
                idx = path[c].parent_idx();
            }
        }
    }

private:

    void init_(Int skip_num)
    {
        typedef typename Iterator::Container::TreePath TreePath;

        const TreePath& path = Base::iterator().path();
        Int             idx  = Base::iterator().key_idx();

        for (Int c = 0; c < Indexes; c++) {
            if (c != skip_num) prefix_[c] = 0;
        }

        for (Int c = 0; c < path.getSize(); c++)
        {
            for (Int block_num = 0; block_num < Indexes; block_num++)
            {
                if (block_num != skip_num)
                {
                    Base::iterator().model().sumKeys(path[c].node(), block_num, 0, idx, prefix_[block_num]);
                }
            }

            idx = path[c].parent_idx();
        }
    }

};




template <typename ElementType_, Int Indexes_>
class StaticVector
{
    typedef StaticVector<ElementType_, Indexes_> MyType;

    ElementType_ values_[Indexes_];

public:
    typedef ElementType_ ElementType;


    static const Int Indexes = Indexes_;

    StaticVector()
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = 0;
        }
    }

    explicit StaticVector(const ElementType& value)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = -1;
        }

        values_[0] = value;
    }

    ElementType get() const
    {
    	return values_[0];
    }

    StaticVector(const MyType& other)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = other.values_[c];
        }
    }

    const ElementType* values() const
    {
        return values_;
    }

    ElementType* values()
    {
        return values_;
    }

    const ElementType& value(Int idx) const
    {
        return values_[idx];
    }

    ElementType& value(Int idx)
    {
        return values_[idx];
    }

    const ElementType& operator[](Int idx) const
    {
        return values_[idx];
    }

    ElementType& operator[](Int idx)
    {
        return values_[idx];
    }

    void clear()
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = 0;
        }
    }

    bool operator==(const MyType& other) const
    {
        for (Int c = 0; c < Indexes; c++)
        {
            if (values_[c] != other.values_[c])
            {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const MyType& other) const
    {
        for (Int c = 0; c < Indexes; c++)
        {
            if (values_[c] == other.values_[c])
            {
                return false;
            }
        }

        return true;
    }

    bool operator<=(const MyType& other) const
    {
    	for (Int c = 0, mask = 1; c < Indexes; c++, mask <<= 1)
    	{
    		bool set = 1;

    		if (set && values_[c] > other.values_[c])
    		{
    			return false;
    		}
    	}

    	return true;
    }

    bool gteAll( const MyType& other ) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] < other.values_[c])
    		{
    			return false;
    		}
    	}

    	return true;
    }

    bool gteAll(const ElementType& other) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] < other)
    		{
    			return false;
    		}
    	}

    	return true;
    }

    bool gtAny( const MyType& other ) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] > other.values_[c])
    		{
    			return true;
    		}
    	}

    	return false;
    }

    bool gtAny( const ElementType& other ) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] > other)
    		{
    			return true;
    		}
    	}

    	return false;
    }

    bool eqAll( const MyType& other ) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] != other.values_[c])
    		{
    			return false;
    		}
    	}

    	return true;
    }

    bool eqAll( const ElementType_& other ) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] != other)
    		{
    			return false;
    		}
    	}

    	return true;
    }

    bool operator>(const MyType& other) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] <= other.values_[c])
    		{
    			return false;
    		}
    	}

    	return true;
    }

    bool operator>(const ElementType& other) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] <= other)
    		{
    			return false;
    		}
    	}

    	return true;
    }


    MyType& operator=(const MyType& other)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = other.values_[c];
        }
        return *this;
    }

    MyType& operator=(const ElementType* keys)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = keys[c];
        }
        return *this;
    }

    MyType& setAll(const ElementType& keys)
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		values_[c] = keys;
    	}
    	return *this;
    }

    MyType& operator+=(const MyType& other)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] += other.values_[c];
        }

        return *this;
    }

    MyType operator+(const MyType& other) const
    {
        MyType result = *this;

        for (Int c = 0; c < Indexes; c++)
        {
            result.values_[c] += other.values_[c];
        }

        return result;
    }

    MyType operator-(const MyType& other) const
    {
        MyType result = *this;

        for (Int c = 0; c < Indexes; c++)
        {
            result.values_[c] -= other.values_[c];
        }

        return result;
    }

    MyType operator-() const
    {
        MyType result = *this;

        for (Int c = 0; c < Indexes; c++)
        {
            result.values_[c] = -values_[c];
        }

        return result;
    }


    MyType& operator-=(const MyType& other)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] -= other.values_[c];
        }

        return *this;
    }
};



template <typename List> struct AccumulatorListBuilder;

template <typename Head, typename... Tail>
struct AccumulatorListBuilder<TypeList<Head, Tail...>> {
	typedef typename MergeLists<
			StaticVector<typename Head::IndexType, Head::Indexes>,
			typename AccumulatorListBuilder<TypeList<Tail...>>::Type
	>::Result																	Type;
};


template <>
struct AccumulatorListBuilder<TypeList<>> {
	typedef TypeList<> 															Type;
};


template <typename List> struct AccumulatorBuilder;

template <typename... Types>
struct AccumulatorBuilder<TypeList<Types...>> {
	typedef std::tuple<Types...> 												Type;
};


}
}

namespace std {
template <typename Key, Int Indexes>
ostream& operator<<(ostream& out, const ::memoria::balanced_tree::StaticVector<Key, Indexes>& accum)
{
    out<<"[";

    for (Int c = 0; c < Indexes; c++)
    {
        out<<accum.value(c);

        if (c < Indexes - 1)
        {
            out<<", ";
        }
    }

    out<<"]";

    return out;
}
}

#endif

