
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_DYNVECTOR_FACTORY_HPP


#include <memoria/prototypes/bstree/factory.hpp>

#include <memoria/prototypes/dynvector/names.hpp>
#include <memoria/prototypes/dynvector/pages/data_page.hpp>

#include <memoria/prototypes/dynvector/tools.hpp>

#include <memoria/prototypes/dynvector/container/insert.hpp>
#include <memoria/prototypes/dynvector/container/remove.hpp>
#include <memoria/prototypes/dynvector/container/tools.hpp>
#include <memoria/prototypes/dynvector/container/checks.hpp>
#include <memoria/prototypes/dynvector/container/find.hpp>
#include <memoria/prototypes/dynvector/container/read.hpp>

#include <memoria/prototypes/dynvector/iterator/api.hpp>
#include <memoria/prototypes/dynvector/iterator.hpp>

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/tools/assert.hpp>



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
                    TypeList<
                        DataPage_
                    >,
                    typename Base::DataPagesList
    >::Result                                                                   DataPagesList;

    typedef DataPage_                                                           DataPage;
    typedef PageGuard<DataPage, typename Base::Allocator>                       DataPageG;
    typedef Buffer_                                                             Buffer;
    typedef BufferContentDescriptor_                                            BufferContentDescriptor;
    typedef CountData_                                                          CountData;
};


template <typename Profile, typename ElementType_>
struct BTreeTypes<Profile, memoria::DynVectorCtr<ElementType_> >: public BTreeTypes<Profile, memoria::BSTreeCtr> {

    typedef IDType                                                              Value;
    typedef BTreeTypes<Profile, memoria::BSTreeCtr>                             Base;

    typedef TypeList<>                                                              DataPagePartsList;

    static const bool MapType                                                   = MapTypes::Sum;

    typedef typename AppendTool<
                    typename Base::ContainerPartsList,
                    TypeList<
                        memoria::dynvector::ToolsName,
                        memoria::dynvector::RemoveName,
                        memoria::dynvector::InsertName,
                        memoria::dynvector::checksName,
                        memoria::dynvector::ReadName,
                        memoria::dynvector::SeekName
                    >
    >::Result                                                                   ContainerPartsList;

    typedef typename AppendTool<
                    typename Base::IteratorPartsList,
                    TypeList<
                       memoria::dynvector::IteratorAPIName
                    >
    >::Result                                                                   IteratorPartsList;


    typedef NullType                                                            Buffer;
    typedef NullType                                                            BufferContentDescriptor;
    typedef NullType                                                            CountData;
    typedef ElementType_                                                        ElementType;


    template <Int Size>
    struct DataBlockTypeFactory {
        typedef NullType                                                        Type;
    };

    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef BTreeIteratorScalarPrefixCache<Iterator, Container>             Type;
    };
};






template <
        typename Profile,
        typename ContainerTypeName,
        typename Atom
>
class CtrTF<Profile, memoria::DynVectorCtr<Atom>, ContainerTypeName>: public CtrTF<Profile, memoria::BSTreeCtr, ContainerTypeName> {

    typedef CtrTF<Profile, memoria::BSTreeCtr, ContainerTypeName>                   Base1;

public:

    typedef typename Base1::ContainerTypes                                          ContainerTypes;

    typedef typename ContainerTypes::DataPagePartsList                              DataPagePartsList;

    MEMORIA_STATIC_ASSERT(IsList<DataPagePartsList>::Value);

    typedef DVDataPage<
                DataPagePartsList,
                typename ContainerTypes::DataBlock,
                memoria::btree::TreePage<typename ContainerTypes::Allocator::Page>
    >                                                                               DataPage;



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
        >                                                                           Base0;


        typedef typename Base0::ContainerPartsList                                  CtrList;
        typedef typename Base0::IteratorPartsList                                   IterList;

        typedef CtrTypesT<Types>                                                    CtrTypes;
        typedef BTreeIterTypes<IterTypesT<Types> >                                  IterTypes;

        typedef DataPath<
                typename Base0::NodeBaseG,
                typename Base0::DataPageG
        >                                                                           TreePath;

        typedef typename TreePath::DataItem                                         DataPathItem;
    };

    typedef typename Types::CtrTypes                                                CtrTypes;

    typedef Ctr<CtrTypes>                                                           Type;
};

}
#endif
