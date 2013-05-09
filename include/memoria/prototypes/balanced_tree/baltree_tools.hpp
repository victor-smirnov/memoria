
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/vector_tuple.hpp>
#include <memoria/core/packed2/packed_dispatcher.hpp>

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
            values_[c] = value;
        }
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

    static MyType create(Int idx, const ElementType& value)
    {
    	MyType me;

    	me[idx] = value;

    	return me;
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

    bool lteAll( const MyType& other ) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] > other.values_[c])
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

    bool lteAll(const ElementType& other) const
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		if (values_[c] > other)
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

    MyType& operator+=(const ElementType& other)
    {
    	for (Int c = 0; c < Indexes; c++)
    	{
    		values_[c] += other;
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

    MyType operator/(const ElementType& divisor) const
    {
    	MyType result = *this;

    	for (Int c = 0; c < Indexes; c++)
    	{
    		result.values_[c] = values_[c] / divisor;
    	}

    	return result;
    }

    UBigInt gtZero() const
    {
    	UBigInt result = 0;

    	for (Int c = 0; c < Indexes; c++)
    	{
    		result += (UBigInt(values_[c] > 0)) << c;
    	}

    	return result;
    }
};





template <typename Tuple, Int Index = std::tuple_size<Tuple>::value> struct TupleDispatcher;

template <typename... Types, Int Index>
struct TupleDispatcher<std::tuple<Types...>, Index> {
	typedef std::tuple<Types...> Tuple;

	static const Int SIZE = std::tuple_size<Tuple>::value;

	template <typename Fn, typename... Args>
	static void dispatch(const Tuple& tuple, Fn&& fn, Args... args)
	{
		fn.template operator()<Index>(std::get<SIZE - Index>(tuple), args...);
		TupleDispatcher<Tuple, Index - 1>::dispatch(tuple, std::move(fn), args...);
	}
};

template <typename... Types>
struct TupleDispatcher<std::tuple<Types...>, 0> {
	typedef std::tuple<Types...> Tuple;

	template <typename Fn, typename... Args>
	static void dispatch(const Tuple& tuple, Fn&& fn, Args... args){}
};


template <typename Element, Int Size>
Element GetElement(const StaticVector<Element, Size>& v, Int idx)
{
	return v[idx];
}


template <typename Value>
struct GetElementFn {

	Value value_ = 0;

	template <Int Index, typename Element>
	void operator()(const Element& element, Int tuple_index, Int value_index)
	{
		if (Index == tuple_index)
		{
			value_ = GetElement(element, value_index);
		}
	}
};

template <typename Element, typename... Types>
Element GetValue(const std::tuple<Types...>& v, Int element_idx, Int value_idx)
{
	GetElementFn<Element> fn;
	TupleDispatcher<std::tuple<Types...>>::dispatch(v, fn, element_idx, value_idx);
	return fn.value_;
}








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








template <typename Types, typename List, Int Idx = 0>
class PackedStructListBuilder;


template <
	typename Types,
	template <typename, Int> class NonLeafStructTF,
	template <typename, Int> class LeafStructTF,
	Int Indexes,
	typename... Tail,
	Int Idx
>
class PackedStructListBuilder<
	Types,
	TypeList<
		StreamDescr<NonLeafStructTF, LeafStructTF, Indexes>,
		Tail...
	>,
	Idx
> {
	typedef TypeList<StreamDescr<NonLeafStructTF, LeafStructTF, Indexes>, Tail...> List;

public:
	typedef typename MergeLists<
			StructDescr<
				typename NonLeafStructTF<Types, Idx>::Type,
				Idx
			>,
			typename PackedStructListBuilder<
				Types,
				TypeList<Tail...>,
				Idx + 1
			>::NonLeafStructList
	>::Result																	NonLeafStructList;

	typedef typename MergeLists<
				StructDescr<
					typename LeafStructTF<Types, Idx>::Type,
					Idx
				>,
				typename PackedStructListBuilder<
					Types,
					TypeList<Tail...>,
					Idx + 1
				>::LeafStructList
	>::Result																	LeafStructList;
};


template <typename Types, Int Idx>
class PackedStructListBuilder<Types, TypeList<>, Idx> {
public:
	typedef TypeList<>															NonLeafStructList;
	typedef TypeList<>															LeafStructList;
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

