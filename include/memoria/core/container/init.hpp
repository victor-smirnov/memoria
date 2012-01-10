
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_INIT_HPP
#define	_MEMORIA_CORE_CONTAINER_INIT_HPP

#include <memoria/core/types/types.hpp>

namespace memoria {

template <Int Order = 200>
struct RootCtrListBuilder {
	typedef typename RootCtrListBuilder<Order - 1>::Type 	Type;
};


template <>
struct RootCtrListBuilder<-1> {
	typedef NullType 										Type;
};


#define MEMORIA_DECLARE_ROOT_CTR(CtrType, Order) 								\
template <>																		\
struct RootCtrListBuilder<Order> {												\
	typedef TL<CtrType, typename RootCtrListBuilder<Order - 1>::Type> 	Type;	\
};


template <typename T = void>
struct RootCtrListProvider;


template <Int Order = 20>
struct ProfileListBuilder {
	typedef typename ProfileListBuilder<Order - 1>::Type 	Type;
};


template <>
struct ProfileListBuilder<-1> {
	typedef NullType 										Type;
};


#define MEMORIA_DECLARE_PROFILE(Profile, Order) 								\
template <>																		\
struct ProfileListBuilder<Order> {												\
	typedef TL<Profile, typename ProfileListBuilder<Order - 1>::Type> 	Type;	\
}



template <typename Profile, typename Rootlist>
class ContainerTypesCollectionMetadataInit;




template <typename Profile>
class ContainerCollectionCfg;

template <typename Profile, typename List, typename Result = NullType>
class ContainerName2TypeMapBuilder;

template <typename Profile>
class ContainerTypesCollection;

template <typename Profile, typename SelectorType, typename ContainerTypeName = SelectorType> class CtrTF;





template <typename Profile, typename Head, typename Tail, typename ResultList>
class ContainerName2TypeMapBuilder<Profile, TL<Head, Tail>, ResultList> {
	typedef typename CtrTF<Profile, Head>::Type                 		Type;
    typedef TL<Pair<Head, Type>, ResultList>                            NewResult;
public:
    typedef typename ContainerName2TypeMapBuilder<Profile, Tail, NewResult>::Result Result;
};

template <typename Profile, typename FinalResult>
class ContainerName2TypeMapBuilder<Profile, NullType, FinalResult> {
public:
    typedef FinalResult                                                         Result;
};


template <typename Profile, typename RootList>
class ContainerTypesCollectionMetadataInit {

	typedef typename ContainerCollectionCfg<Profile>::Types		ContainerTypesType;

	typedef typename ContainerName2TypeMapBuilder<
			Profile,
			RootList
	>::Result   												Name2TypeMapList;

public:

	typedef ContainerDispatcher<
			Profile,
			typename ContainerTypesType::AllocatorType::AbstractAllocator,
			Name2TypeMapList
			>         											ContainerDispatcherType;

	static Int Init()
	{
		if (ContainerTypesCollection<Profile>::reflection_ == NULL) {
			MetadataList list;
			ContainerDispatcherType::BuildMetadataList(list);
			ContainerTypesCollection<Profile>::reflection_ = new ContainerCollectionMetadataImpl(TypeNameFactory<Profile>::name(), list);
		}

		return ContainerTypesCollection<Profile>::reflection_->Hash();
	}

	static void PrintContainerHashes() {
		ContainerDispatcherType::PrintContainerHashes();
	}

};



template <typename ProfileList, typename CtrList>
class Memoria {
public:
	static Int Init() {
		ContainerTypesCollectionMetadataInit<typename ProfileList::Head, CtrList>::Init();
		return Memoria<typename ProfileList::Tail, CtrList>::Init();
	}
};

template <typename CtrList>
class Memoria<NullType, CtrList> {
public:
	static Int Init() {
		return 1;
	}
};


#define MEMORIA_INIT()													\
const int MEMORIA_INITIALIZED = ::memoria::Memoria<						\
	::memoria::ProfileListBuilder<>::Type,								\
	::memoria::RootCtrListBuilder<>::Type								\
>::Init()




}

#endif

