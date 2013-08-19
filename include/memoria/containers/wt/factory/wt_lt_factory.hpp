
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_WT_LABELEDTREE_FACTORY_HPP
#define _MEMORIA_CONTAINERS_WT_LABELEDTREE_FACTORY_HPP


#include <memoria/containers/wt/wt_names.hpp>
#include <memoria/containers/wt/container/wt_c_checks.hpp>


namespace memoria {

template <typename Profile, typename... LabelDescriptors>
struct BTTypes<Profile, wt::WTLabeledTree<LabelDescriptors...>>:
	BTTypes<Profile, memoria::LabeledTree<LabelDescriptors...>> {

	typedef BTTypes<Profile, memoria::LabeledTree<LabelDescriptors...>> 		Base;

	typedef typename MergeLists<
					typename Base::ContainerPartsList,
					wt::CtrChecksName
	>::Result                                           						ContainerPartsList;

};


template <typename Profile, typename... LabelDescriptors, typename T>
class CtrTF<Profile, wt::WTLabeledTree<LabelDescriptors...>, T>:
	public CtrTF<Profile, memoria::LabeledTree<LabelDescriptors...>, T> {
};



}
#endif
