
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_MODEL_API_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_MODEL_MODEL_API_HPP

#include <memoria/prototypes/dynvector/names.hpp>



namespace memoria    {

MEMORIA_CONTAINER_PART_BEGIN(memoria::dynvector::ContainerApiName)

		typedef typename Base::Types                                                Types;
		typedef typename Base::Allocator                                            Allocator;

		typedef typename Allocator::Page                                            Page;
		typedef typename Page::ID                                                   ID;

		typedef typename Types::NodeBase                                            NodeBase;
		typedef typename Base::Iterator                                             Iterator;

		typedef typename Types::Pages::NodeDispatcher                               NodeDispatcher;
		typedef typename Types::Pages::RootDispatcher                               RootDispatcher;
		typedef typename Types::Pages::LeafDispatcher                               LeafDispatcher;
		typedef typename Types::Pages::NonLeafDispatcher                            NonLeafDispatcher;
		typedef typename Types::Pages::NonRootDispatcher                            NonRootDispatcher;

		typedef typename Types::Pages::Node2RootMap                                 Node2RootMap;
		typedef typename Types::Pages::Root2NodeMap                                 Root2NodeMap;

		typedef typename Base::Metadata                                             Metadata;

		typedef typename Base::Key                                                  Key;
		typedef typename Base::Value                                                Value;

		typedef typename Types::DataPage                                        	DataPage;
		typedef typename Types::Buffer                                          	Buffer;
		typedef typename Types::BufferContentDescriptor                         	BufferContentDescriptor;
		typedef typename Types::CountData                                       	CountData;


		static const Int Indexes                                                    = Types::Indexes;

MEMORIA_CONTAINER_PART_END

}


#endif
