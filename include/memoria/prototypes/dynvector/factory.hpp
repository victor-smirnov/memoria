
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_FACTORY_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_FACTORY_HPP


#include <memoria/prototypes/itree/factory.hpp>

#include <memoria/prototypes/dynvector/names.hpp>
#include <memoria/prototypes/dynvector/pages/data_page.hpp>

#include <memoria/prototypes/dynvector/container/insert.hpp>
#include <memoria/prototypes/dynvector/container/remove.hpp>
#include <memoria/prototypes/dynvector/container/tools.hpp>

#include <memoria/prototypes/dynvector/iterator/api.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/vapi/models/types.hpp>

namespace memoria    {

using namespace memoria::btree;
using namespace memoria::dynvector;


template <
    typename DataPage_,
    typename Buffer_,
    typename BufferContentDescriptor_,
    typename CountData_,
    typename Base
>
struct DynVectorContainerTypes: public Base {

    typedef typename AppendTool<
                    typename Base::DataPagesList,
                    typename TLTool<
                        DataPage_
                    >::List
    >::Result                                                                   DataPagesList;

    typedef DataPage_                                                       	DataPage;
    typedef Buffer_                                                         	Buffer;
    typedef BufferContentDescriptor_                                        	BufferContentDescriptor;
    typedef CountData_                                                      	CountData;
};


template <typename Profile>
struct BTreeTypes<Profile, memoria::DynVector>: public BTreeTypes<Profile, memoria::ITree> {

	typedef IDType																Value;
    typedef BTreeTypes<Profile, memoria::ITree> 								Base;

    typedef NullType                                                            DataPagePartsList;

    static const bool MapType                                                   = MapTypes::Index;

    typedef typename AppendTool<
                    typename Base::ContainerPartsList,
                    typename TLTool<
                        memoria::dynvector::ToolsName,
                        memoria::dynvector::RemoveName,
                        memoria::dynvector::InsertName
                    >::List
    >::Result                                                                   ContainerPartsList;

    typedef typename AppendTool<
                        typename Base::IteratorPartsList,
                        typename TLTool<
                        	memoria::dynvector::IteratorAPIName
                        >::List
    >::Result                                                                   IteratorPartsList;


    typedef memoria::vapi::Data                                                 Buffer;
    typedef NullType                                                            BufferContentDescriptor;
    typedef NullType                                                            CountData;


    template <Int Size>
    struct DataBlockTypeFactory {
        typedef NullType                                                        Type;
    };
};






template <
        typename Profile,
        typename ContainerTypeName
>
class CtrTF<Profile, memoria::DynVector, ContainerTypeName>: public CtrTF<Profile, memoria::ITree, ContainerTypeName> {

    typedef CtrTF<Profile, memoria::ITree, ContainerTypeName>   					Base1;

public:

    typedef typename Base1::ContainerTypes 											ContainerTypes;

    typedef typename ContainerTypes::DataPagePartsList                              DataPagePartsList;

    MEMORIA_STATIC_ASSERT(IsList<DataPagePartsList>::Value);

    typedef DVDataPage<
                DataPagePartsList,
                ContainerTypes::template DataBlockTypeFactory,
                memoria::btree::TreePage<typename ContainerTypes::Allocator>
    >                                                                           	DataPage;

    struct Types: DynVectorContainerTypes<
                DataPage,
                typename ContainerTypes::Buffer,
                typename ContainerTypes::BufferContentDescriptor,
                typename ContainerTypes::CountData,
                typename Base1::Types
    >
    {
    	typedef DynVectorContainerTypes<
                DataPage,
                typename ContainerTypes::Buffer,
                typename ContainerTypes::BufferContentDescriptor,
                typename ContainerTypes::CountData,
                typename Base1::Types
        > 																			Base0;


    	typedef typename Base0::ContainerPartsList 									CtrList;
    	typedef typename Base0::IteratorPartsList									IterList;

    	typedef CtrTypesT<Types> 													CtrTypes;
    	typedef BTreeIterTypes<IterTypesT<Types> >									IterTypes;
    };

    typedef typename Types::CtrTypes												CtrTypes;

    typedef Ctr<CtrTypes>                                                           Type;
};

}
#endif
