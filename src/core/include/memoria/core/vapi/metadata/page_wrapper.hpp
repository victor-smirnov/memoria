
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CORE_API_METADATA_PAGE_WRAPPER_HPP
#define _MEMORIA_CORE_API_METADATA_PAGE_WRAPPER_HPP

#include <memoria/metadata/metadata.hpp>



namespace memoria { namespace vapi {

template <typename Interface>
class PageImplT: public Interface {
	typedef PageImplT<Interface> 				Me;
public:

    virtual IDValue GetId() const               = 0;
    virtual Int GetContainerHash() const            = 0;
    virtual Int GetPageTypeHash() const         = 0;
    virtual BigInt GetFlags() const             = 0;
    virtual void* Ptr()                         = 0;
    virtual const void* Ptr() const             = 0;

    virtual Int Size() const                    = 0;
    virtual Int GetByte(Int idx) const          = 0;
    virtual void SetByte(Int idx, Int value)    = 0;
};

typedef PageImplT<Page> 							PageImpl;


}}


#endif
