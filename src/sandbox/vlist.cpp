/*
 * vlist.cpp
 *
 *  Created on: 30.11.2012
 *      Author: developer
 */


#include <typeinfo>
#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/append.hpp>
#include <memoria/core/types/algo/select.hpp>

using namespace std;
using namespace memoria;
using namespace memoria::vapi;

template <typename ... List> struct VTList {};

template <typename ... List> struct ListHead;

template <typename Head, typename ... Tail>
struct ListHead<VTList<Head, Tail...>> {
	typedef Head Type;
};

template <typename ... List> struct ListTail;

template <typename Head, typename ... Tail>
struct ListTail<Head, Tail...> {
	typedef VTList<Tail...> Type;
};

template <typename List1, typename List2> struct MergeLists;

template <typename ... List1, typename ... List2>
struct MergeLists<VTList<List1...>, VTList<List2...>> {
	typedef VTList<List1..., List2...> Type;
};

template <typename ... List>
struct ListSize1 {
	static const UInt Value = sizeof...(List);
};


template <typename ... List>
struct ListSize1<VTList<List...> > {
	static const UInt Value = sizeof...(List);
};

template <typename Item, typename List> struct Append;

template <typename Item, typename ... List>
struct Append<Item, VTList<List...> > {
	typedef VTList<Item, List...> Type;
};

template <typename ... List1, typename ... List2>
struct Append<VTList<List1...>, VTList<List2...> > {
	typedef VTList<List1..., List2...> Type;
};

template <typename ... List, typename Item>
struct Append<VTList<List...>, Item > {
	typedef VTList<List..., Item> Type;
};

template <Int Idx, typename List> struct Select1;

template <typename Head, typename ... Tail>
struct Select1<0, VTList<Head, Tail...> > {
	typedef Head Type;
};

template <Int Idx, typename Head, typename ... Tail>
struct Select1<Idx, VTList<Head, Tail...> > {
	typedef typename Select1<Idx - 1, VTList<Tail...> >::Type Type;
};


template <typename Item, typename List, Int Idx = 0> struct IndexOf;

template <typename Item, Int Idx>
struct IndexOf<Item, VTList<>, Idx> {
	static const Int Value = -1;
};

template <typename Item, typename ... Tail, Int Idx>
struct IndexOf<Item, VTList<Item, Tail...>, Idx> {
	static const Int Value = Idx;
};

template <typename Item, typename Head, typename ... Tail, Int Idx>
struct IndexOf<Item, VTList<Head, Tail...>, Idx> {
	static const Int Value = IndexOf<Item, VTList<Tail...>, Idx + 1 >::Value;
};


template <typename Item, typename List, bool All = false> struct RemoveItem;

template <typename Item, typename Head, typename ... Tail, bool All>
struct RemoveItem<Item, VTList<Head, Tail...>, All> {
    typedef typename Append<
    					Head,
    					typename RemoveItem<
    								Item,
    								VTList<Tail...>,
    								All
    					>::Type
    				>::Type 														Type;
};

template <typename Item, typename ... Tail>
struct RemoveItem<Item, VTList<Item, Tail...>, false> {
    typedef VTList<Tail...>															Type;
};

template <typename Item, typename ... Tail>
struct RemoveItem<Item, VTList<Item, Tail... >, true> {
    typedef typename RemoveItem<
    					Item,
    					VTList<Tail...>,
    					true
    				>::Type                       									Type;
};


template <typename Item, bool All>
struct RemoveItem<Item, VTList<>, All> {
    typedef VTList<>                                                            	Type;
};


int main(void) {

	typedef VTList<int, float, bool> List0;

	cout<<TypeNameFactory<List0>::name()<<endl;
	cout<<TypeNameFactory<ListHead<List0>::Type>::name()<<endl;

	cout<<TypeNameFactory<Append<double, List0>::Type>::name()<<endl;
	cout<<TypeNameFactory<Append<List0, double>::Type>::name()<<endl;
	cout<<TypeNameFactory<Append<List0, VTList<double, unsigned> >::Type>::name()<<endl;
	cout<<TypeNameFactory<Append<VTList<double, unsigned>, List0 >::Type>::name()<<endl;

	cout<<TypeNameFactory<RemoveItem<int, List0>::Type>::name()<<endl;

	cout<<TypeNameFactory<RemoveItem<int, Append<int, List0>::Type, true>::Type>::name()<<endl;

	cout<<IndexOf<int, List0>::Value<<endl;
	cout<<IndexOf<bool, Append<List0, double>::Type>::Value<<endl;
	cout<<IndexOf<double, Append<List0, double>::Type>::Value<<endl;
	cout<<IndexOf<unsigned, Append<List0, double>::Type>::Value<<endl;


	return 0;
}
