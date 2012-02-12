
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_PAGES_DATA_PAGE_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_PAGES_DATA_PAGE_HPP

#include <memoria/core/tools/reflection.hpp>


namespace memoria    {
namespace dynvector {

#pragma pack(1)

template <
        typename ComponentList,
        template <Int> class DataBlockTypeFactory,
        typename Base0
>
class DVDataPage: public PageBuilder<ComponentList, Base0>
{

public:

    typedef DVDataPage<
                ComponentList,
                DataBlockTypeFactory,
                Base0
    >                                                                           	Me;

    typedef PageBuilder<ComponentList, Base0>                      					Base;

    typedef typename Base0::Allocator                                         		Allocator;

public:
    
    typedef typename DataBlockTypeFactory<Allocator::PAGE_SIZE - sizeof(Base)>::Type PageData;

private:

    PageData data_;

    static PageMetadata *reflection_;

public:

    DVDataPage(): Base(), data_() {}

    void init() {
    	data_.init();
    	Base::init();
    }

    Short size() const {
    	return data_.size();
    }

    const PageData& data() const {
        return data_;
    }

    PageData& data() {
        return data_;
    }

    static Int hash() {
        return reflection()->Hash();
    }

    static bool is_abi_compatible() {
        return reflection()->IsAbiCompatible();
    }

    static PageMetadata *reflection()
    {
    	return reflection_;
    }

    void Reindex()
    {
        Base::Reindex();
        data_.reindex();
    }

    Int data_size() const
    {
        Me* me = NULL;
        //FIXME: strict alias ?????????
        //Use c++11 offsetof
        return ((Int)(BigInt)&me->data_) + data_.byte_size();
    }

    static Int get_max_size() {
        return PageData::max_size();
    }

    template <template <typename> class FieldFactory>
    void BuildFieldsList(MetadataList &list, Long &abi_ptr) const
    {
        Base::template BuildFieldsList<FieldFactory>(list, abi_ptr);
        FieldFactory<PageData>::create(list, data(), "DATA", abi_ptr);
    }

    static Int get_page_size(const void* buf)
    {
        const Me* me = static_cast<const Me*>(buf);
        return me->data_size();
    }

    static Int Init()
    {
        if (reflection_ == NULL)
        {
            Long abi_ptr = 0;
            Me* me = 0;
            MetadataList list;
            me->BuildFieldsList<FieldFactory>(list, abi_ptr);

            Int hash0 = 1234567;
            Int attrs = BITMAP;

            reflection_ = new PageMetadataImpl("DATA_PAGE", list, attrs, hash0, &get_page_size, Allocator::PAGE_SIZE);
        }
        else {}

        return reflection_->Hash();
    }
};


template <
        typename ComponentList,
        template <Int> class DataBlockTypeFactory,
        typename BaseType
>
PageMetadata* DVDataPage<ComponentList, DataBlockTypeFactory, BaseType>::reflection_ = NULL;

#pragma pack()

}
}

#endif
