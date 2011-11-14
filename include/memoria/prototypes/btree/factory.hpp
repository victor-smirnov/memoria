
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_FACTORY_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_FACTORY_HPP

#include <memoria/prototypes/btree/names.hpp>

#include <memoria/prototypes/btree/pages/pages.hpp>

#include <memoria/prototypes/btree/container/base.hpp>
#include <memoria/prototypes/btree/container/tools.hpp>
#include <memoria/prototypes/btree/container/checks.hpp>
#include <memoria/prototypes/btree/container/init.hpp>
#include <memoria/prototypes/btree/container/insert.hpp>
#include <memoria/prototypes/btree/container/remove.hpp>
#include <memoria/prototypes/btree/container/model_api.hpp>
#include <memoria/prototypes/btree/container/stubs.hpp>
#include <memoria/prototypes/btree/container/find.hpp>

#include <memoria/prototypes/btree/iterator.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>


namespace memoria    {

using namespace memoria::btree;

template <typename Profile, typename ContainerTypeSelector>
struct BTreeTypes {

    typedef TL<BigInt>                                                    		KeysList;

    static const Int Indexes                                                    = 1;

    static const bool MapType                                                   = MapTypes::Value;

    typedef typename TLTool<
    		memoria::btree::ToolsName,
    		memoria::btree::StubsName,
    		memoria::btree::ChecksName,
    		memoria::btree::InsertName,
    		memoria::btree::RemoveName,
    		memoria::btree::FindName
    >::List                                                                     ContainerPartsList;

    typedef NullType                                                            BasePagePartsList;
    
    typedef NullType                                                            RootPagePartsList;

    typedef NullType                                                            InternalPagePartsList;

    typedef NullType                                                            LeafPagePartsList;

    typedef NullType                                                            DataPagesList;

    typedef typename TLTool<
    		memoria::btree::IteratorToolsName,
    		memoria::btree::IteratorWalkName,
    		memoria::btree::IteratorAPIName,
    		memoria::btree::IteratorMultiskipName,
    		memoria::btree::IteratorContainerAPIName
    >::List                                                                     IteratorPartsList;

    typedef EmptyType                                            				ContainerInterface;
    typedef EmptyType                                    						IteratorInterface;

    typedef typename ContainerCollectionCfg<Profile>::Types::AbstractAllocator	Allocator;
    typedef typename BTreeRootMetadataTypeFactory<
    			Profile,
    			BTreeRootMetadataFactory<ContainerTypeSelector>
    >::Type 																	Metadata;


    template <
    	typename Types_
    >
    struct CtrBaseFactory {
    	typedef BTreeContainerBase<Types_>  									Type;
    };

    template <
    	typename Types_
    >
    struct IterBaseFactory {
    	typedef BTreeIteratorBase<Types_> 										Type;
    };
};


template <
        typename Profile,
        typename ContainerTypeName_
>
class CtrTF<Profile, memoria::BTree, ContainerTypeName_> {

	typedef CtrTF<Profile, memoria::BTree, ContainerTypeName_> 						MyType;

public:


	typedef BTreeTypes<Profile, ContainerTypeName_> 								ContainerTypes;


    typedef TL<
                RootNodeMetadataName<typename ContainerTypes::Metadata>,
                typename ContainerTypes::RootPagePartsList
    >                                                                          		RootPagePartsList;
    
    typedef typename ContainerTypes::Allocator::Page::ID                        	ID;

    //TAGS: #IF_THEN_ELSE_EXAMPLE
    typedef typename memoria::IfThenElse<
                IfTypesEqual<
                	typename ContainerTypes::Value,
                	memoria::btree::IDType
                >::Value,
                ID,
                typename ContainerTypes::Value
    >::Result                                                                   	Value;


    typedef BTreeCountersBase<BigInt>                                               BTreeCounters;

    struct BasePartsTypes{
    	typedef TreePage<typename ContainerTypes::Allocator> 				NodePageBase;
    	typedef typename ContainerTypes::BasePagePartsList 					List;
    };

    typedef PageStart<BasePartsTypes>												BasePageParts;
    typedef NodePageBase<BTreeCounters,  BasePageParts>                				NodePageBase0;

    struct NodePageContainerTypes: public NodePageBase0 {};


    struct NodeTypesBase {
    	typedef NodePageContainerTypes 						NodePageBase;
    	typedef ContainerTypeName_ 							Name;
    	static const Int 									Indexes 			= ContainerTypes::Indexes;
    	static const bool 									PackedMapType 		= ContainerTypes::MapType;
    	typedef typename ContainerTypes::BasePagePartsList 	BasePartsList;
    };


    typedef typename ContainerTypes::KeysList::Head 						NodeKey;

    template <int Level> struct RootLeafTypes: NodeTypesBase {
    	typedef NodeKey 													Key;
    	typedef typename MyType::Value 										Value;
    	typedef typename AppendTool<
    			RootPagePartsList,
    			typename ContainerTypes::LeafPagePartsList
    	>::Result 															List;
    	typedef NodeDescriptor<true, true, Level> 							Descriptor;
    };

    template <int Level> struct LeafTypes: NodeTypesBase {
    	typedef NodeKey 													Key;
    	typedef typename MyType::Value 										Value;
    	typedef typename ContainerTypes::LeafPagePartsList 					List;
    	typedef NodeDescriptor<false, true, Level> 							Descriptor;
    };

    template <int Level> struct RootTypes: NodeTypesBase {
    	typedef NodeKey 													Key;
    	typedef ID 															Value;
    	typedef typename AppendTool<
    			RootPagePartsList,
    			typename ContainerTypes::InternalPagePartsList
        >::Result 															List;
    	typedef NodeDescriptor<true, false, Level> 							Descriptor;
    };

    template <int Level> struct InternalTypes: NodeTypesBase {
    	typedef NodeKey 													Key;
    	typedef ID 															Value;
    	typedef typename ContainerTypes::InternalPagePartsList 				List;
    	typedef NodeDescriptor<false, false, Level> 						Descriptor;
    };

    typedef typename NodeTLBuilder <
    			MyType,
                typename ContainerTypes::KeysList
    >::List                                                                 NodeTypesList;

    MEMORIA_STATIC_ASSERT(IsNonemptyList<NodeTypesList>::Value);


    typedef NodePageContainerTypes													NodeContainerTypes;

    struct DispatcherTypes {
    	typedef NodeTypesList 								NodeList;
    	typedef NodeContainerTypes 							NodeBase;
    	typedef typename ContainerTypes::Allocator			Allocator;
    };

    typedef BTreeDispatchers<DispatcherTypes>                                   	PageDispatchers;


public:
    struct Types: ContainerTypes
    {
    	typedef ContainerTypeName_ 												ContainerTypeName;
    	typedef typename MyType::Value                                      	Value;
    	typedef typename MyType::PageDispatchers                                Pages;

    	typedef typename BTreeCountersTypeFactory<
    			Profile,
    			BTreeCountersFactory<ContainerTypeName_>
    	>::Type      															Counters;

    	typedef NodeContainerTypes                                              NodeBase;
    	typedef NullType                                      					EmbeddedContainersList;

    	typedef typename ContainerTypes::Allocator								Allocator;
    	typedef typename ContainerTypes::Metadata								Metadata;

//    	typedef typename ReverseTool<
//    			typename ContainerTypes::ContainerPartsList
//    	>::Result 																CtrList;
//
//    	typedef typename ReverseTool<
//    			typename ContainerTypes::IteratorPartsList
//    	>::Result																IterList;


    	typedef typename ContainerTypes::ContainerPartsList						CtrList;
    	typedef typename ContainerTypes::IteratorPartsList						IterList;


    	typedef CtrTypesT<Types> 												CtrTypes;
    	typedef BTreeIterTypes<IterTypesT<Types> >								IterTypes;
    };


    typedef typename Types::CtrTypes 											CtrTypes;
    typedef Ctr<CtrTypes>														Type;
};



}

#endif
