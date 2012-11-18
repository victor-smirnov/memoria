
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_NAMES_HPP
#define _MEMORIA_CORE_CONTAINER_NAMES_HPP


namespace memoria    {

class ContainerCollectionCfgName                        {};

template <typename PageType, int MaxPageSize = MAX_BLOCK_SIZE>
struct AbstractAllocatorName {};

template <typename Profile, typename Params> class AbstractAllocatorFactory;


template <typename Types>  struct CtrTypesT: Types {

    typedef Types                       Base;
    typedef typename Types::CtrList     List;

    template <typename Types_> struct BaseFactory {
        typedef typename Types::template CtrBaseFactory<Types_>::Type Type;
    };
};

template <typename Types>  struct IterTypesT: Types {

    typedef Types                       Base;
    typedef typename Types::IterList    List;

    template <typename Types_> struct BaseFactory {
        typedef typename Types::template IterBaseFactory<Types_>::Type Type;
    };
};


}

#endif
