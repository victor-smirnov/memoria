
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP
#define _MEMORIA_CONTAINERS_MAPX_FACTORY_HPP

#include <memoria/containers/map/container/map_c_insert.hpp>
#include <memoria/containers/map/container/mapm_c_insert.hpp>
#include <memoria/containers/map/container/map_c_remove.hpp>
#include <memoria/containers/map/iterator/map_i_nav.hpp>
#include <memoria/containers/map/iterator/mapm_i_nav.hpp>
#include <memoria/containers/map/map_iterator.hpp>
#include <memoria/containers/map/map_names.hpp>
#include <memoria/containers/map/map_tools.hpp>

#include <memoria/prototypes/bt_ss/btss_factory.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/fse_max/packed_fse_max_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
#include <memoria/core/packed/array/packed_fse_array.hpp>
#include <memoria/core/packed/array/packed_vle_dense_array.hpp>
#include <memoria/core/packed/misc/packed_sized_struct.hpp>

#include <memoria/core/tools/bignum/bigint.hpp>
#include <memoria/core/tools/strings/string.hpp>
#include <memoria/core/tools/strings/string_codec.hpp>

#include <memoria/core/tools/uuid.hpp>

#include <tuple>

namespace memoria {


template <
    typename Profile,
	typename Key_,
    typename Value_
>
struct MapBTTypesBaseBase: public BTTypes<Profile, memoria::BTSingleStream> {

    using Base = BTTypes<Profile, memoria::BTSingleStream>;

    using Key 	= Key_;
    using Value = Value_;

    using Entry = std::tuple<Key, Value>;

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                memoria::map::CtrInsertMaxName,
                memoria::map::CtrRemoveName
    >;


    using IteratorPartsList = MergeLists<
                typename Base::IteratorPartsList,
                memoria::map::ItrNavMaxName
    >;
};



template <
    typename Profile,
    typename Key,
    typename Value,
	Int Special = 0
>
struct MapBTTypesBase: public MapBTTypesBaseBase<Profile, Key, Value> {

	static_assert(
			IsExternalizable<Key>::Value ,
			"Key type must have either ValueCodec or FieldFactory defined"
	);

	static_assert(
			IsExternalizable<Value>::Value ,
			"Value type must have either ValueCodec or FieldFactory defined"
	);

	using LeafKeyStruct = typename map::MapKeyStructTF<Key, HasFieldFactory<Key>::Value>::Type;

	using LeafValueStruct = typename map::MapValueStructTF<Value, HasFieldFactory<Value>::Value>::Type;

	using StreamDescriptors = TL<
    		StreamTF<
				TL<
					TL<StreamSize>,
					TL<LeafKeyStruct>,
					TL<LeafValueStruct>
				>,
				map::MapBranchStructTF
			>
    >;
};





template <
    typename Profile,
    typename Key_,
    typename Value_
>
struct BTTypes<Profile, memoria::Map<Key_, Value_>>: public MapBTTypesBase<Profile, Key_, Value_>{};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map<Key, Value>, T>: public CtrTF<Profile, memoria::BTSingleStream, T> {
};



}

#endif
