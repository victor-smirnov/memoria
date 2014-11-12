
#ifndef MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_
#define MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder_internal.hpp>


namespace memoria {
namespace bt {



template <typename List, Int Idx = 0>
class PackedLeafStructListBuilder;

template <typename List, Int Idx = 0>
class PackedNonLeafStructListBuilder;



template <
    typename StructsTF,
    typename... Tail,
    Int Idx
>
class PackedLeafStructListBuilder<TypeList<StructsTF, Tail...>, Idx> {

    typedef TypeList<StructsTF, Tail...> List;

    typedef typename internal::LinearLeafListHelper<
    		typename StructsTF::LeafType,
    		Idx
    >::Type																		LeafList;

public:

    typedef typename MergeLists<
                LeafList,
                typename PackedLeafStructListBuilder<
                    TypeList<Tail...>,
                    Idx + ListSize<LeafList>::Value
                >::StructList
    >::Result                                                                   StructList;

    typedef typename internal::SubstreamSizeListBuilder<
                typename StructsTF::LeafType
    >::Type                                                                   	SubstreamSizeList;
};


template <
    typename StructsTF,
    typename... Tail,
    Int Idx
>
class PackedNonLeafStructListBuilder<TypeList<StructsTF, Tail...>, Idx> {
public:
	typedef typename MergeLists<
            StreamDescr<
                typename StructsTF::NonLeafType,
                Idx
            >,
            typename PackedNonLeafStructListBuilder<
                TypeList<Tail...>,
                Idx + 1
            >::StructList
    >::Result                                                                   StructList;
};





template <Int Idx>
class PackedLeafStructListBuilder<TypeList<>, Idx> {
public:
    typedef TypeList<>                                                          StructList;
    typedef TypeList<>                                                          SubstreamSizeList;
};


template <Int Idx>
class PackedNonLeafStructListBuilder<TypeList<>, Idx> {
public:
    typedef TypeList<>                                                          StructList;
};




}
}



#endif /* MEMORIA_BT_PACKED_STRUCT_LIST_BUILDER_HPP_ */
