
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP

#include <memoria/core/tools/static_array.hpp>

#include <ostream>

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

    explicit StaticVector(const ElementType& value, Int idx = 0)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = -1;
        }

        values_[idx] 	= value;
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

    StaticVector(const ElementType* keys)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = keys[c];
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

