
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_SEQDENSE_PAGES_SEQ_DATAPAGE_HPP
#define _MEMORIA_CONTAINERS_SEQDENSE_PAGES_SEQ_DATAPAGE_HPP

#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/packed/packed_seq.hpp>

namespace memoria    {

#pragma pack(1)


template <
        typename ComponentList,
        typename IndexType_,
        typename ElementType_,
        Int BitsPerSymbol,
        typename Base0
>
class SequenceDataPage: public PageBuilder<ComponentList, Base0>
{
    static const UInt VERSION = 1;

public:

    typedef SequenceDataPage<
                ComponentList,
                IndexType_,
                ElementType_,
                BitsPerSymbol,
                Base0
    >                                                                           MyType;

    typedef PageBuilder<ComponentList, Base0>                                   Base;

    typedef typename MergeLists <
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                Int,
                IndexType_,
                ElementType_
    >::Result                                                                   FieldsList;

    static const UInt PAGE_HASH = md5::Md5Sum<typename TypeToValueList<FieldsList>::Type>::Result::Value32;

    typedef IndexType_                                      					IndexType;
    typedef ElementType_                                      					ElementType;

private:

    typedef PackedSeqTypes<
    		IndexType,
    		ElementType,
    		BitsPerSymbol,
    		PackedSeqBranchingFactor,
    		PackedSeqValuesPerBranch / (BitsPerSymbol * 2)
    > 																			Types;
public:

    typedef PackedSeq<Types>													Sequence;

    static const Int Bits														= BitsPerSymbol;
    static const Int Symbols													= Sequence::Symbols;
    static const Int Blocks														= Sequence::Blocks;

private:

//    Int size_;
    Sequence sequence_;

    static PageMetadata *page_metadata_;

public:

    SequenceDataPage(): Base(), sequence_() {}

    void init(Int block_size)
    {
    	Base::init();
    	MEMORIA_ASSERT(block_size, >=, 512);

    	sequence_.initByBlock(block_size - sizeof(MyType));
    }

    class SizeSetter {
    	MyType& me_;
    public:

    	SizeSetter(MyType& me): me_(me) {}

    	MyType& operator=(Int value)
    	{
    		me_.sequence_.size() = value;
    		return me_;
    	}

    	MyType& operator+=(Int value)
    	{
    		me_.sequence_.size() += value;
    		return me_;
    	}

    	MyType& operator-=(Int value)
    	{
    		me_.sequence_.size() -= value;
    		return me_;
    	}

    	operator Int() const
    	{
    		return me_.sequence_.size();
    	}
    };

    Int size() const {
        return sequence_.size();
    }

    SizeSetter size() {
    	return SizeSetter(*this);
    }

    const Int& symbols() const {
    	return sequence_.size();
    }

    Int& symbols() {
    	return sequence_.size();
    }

    static Int hash() {
        return PAGE_HASH;
    }

    static PageMetadata *page_metadata()
    {
        return page_metadata_;
    }

    void Reindex()
    {
        Base::Reindex();
        sequence_.reindex();
    }

    Int data_size() const
    {
        return sizeof(MyType) + sequence_.getDataSize();
    }

    Int getCapacity() const
    {
    	return sequence_.capacity();
    }

    Int getMaxCapacity() const
    {
        return sequence_.maxSize();
    }

    static Int getMaxPageCapacity(Int size)
    {
    	return Sequence::maxSizeFor(size - sizeof(MyType));
    }

    const ElementType* values() const {
    	return sequence_.valuesBlock();
    }

    ElementType* values() {
        return sequence_.valuesBlock();
    }

    const ElementType* addr(Int idx) const
    {
        return sequence_.cellAddr(idx);
    }

    ElementType* addr(Int idx)
    {
    	return sequence_.cellAddr(idx);
    }

    void shift(BigInt pos, BigInt length)
    {
    	if (length > 0)
    	{
    		sequence_.insertSpace(pos, length);
    		sequence_.size() -= length;
    	}
    	else {
    		sequence_.removeSpace(pos + length, -length);
    		sequence_.size() += -length;
    	}
    }

    void copyTo(MyType* other, Int from, Int to, Int length) const
    {
    	sequence_.copyTo(&other->sequence_, from, length, to);
    }

    void copyFrom(const MyType* src)
    {
    	Base::copyFrom(src);

    	src->sequence_.transferTo(&sequence_);
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);
        sequence_.generateDataEvents(handler);
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

//        FieldFactory<Int>::serialize(buf, size_);
        FieldFactory<Sequence>::serialize(buf, sequence_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

//        FieldFactory<Int>::deserialize(buf, size_);
        FieldFactory<Sequence>::deserialize(buf, sequence_);
    }


    class PageOperations: public IPageOperations
    {
        virtual Int serialize(const void* page, void* buf) const
        {
            const MyType* me = T2T<const MyType*>(page);

            SerializationData data;
            data.buf = T2T<char*>(buf);

            me->template serialize<FieldFactory>(data);

            return data.total;
        }

        virtual void deserialize(const void* buf, Int buf_size, void* page) const
        {
            MyType* me = T2T<MyType*>(page);

            DeserializationData data;
            data.buf = T2T<const char*>(buf);

            me->template deserialize<FieldFactory>(data);
        }

        virtual Int getPageSize(const void *page) const
        {
            const MyType* me = T2T<const MyType*>(page);
            return me->data_size();
        }

        virtual void resize(const void* page, void* buffer, Int new_size) const
        {
            const MyType* me = T2T<const MyType*>(page);
            MyType* tgt = T2T<MyType*>(buffer);

            tgt->copyFrom(me);
        }

        virtual void generateDataEvents(
                        const void* page,
                        const DataEventsParams& params,
                        IPageDataEventHandler* handler
                     ) const
        {
            const MyType* me = T2T<const MyType*>(page);
            handler->startPage("DATA_PAGE");
            me->generateDataEvents(handler);
            handler->startPage("DATA_PAGE");
        }

        virtual void generateLayoutEvents(
                        const void* page,
                        const LayoutEventsParams& params,
                        IPageLayoutEventHandler* handler
                     ) const
        {
            const MyType* me = T2T<const MyType*>(page);
            me->generateLayoutEvents(handler);
        }
    };

    static Int initMetadata()
    {
        if (page_metadata_ == NULL)
        {
            Int attrs = BITMAP;

            page_metadata_ = new PageMetadata("DATA_PAGE", attrs, PAGE_HASH, new PageOperations());
        }
        else {}

        return page_metadata_->hash();
    }
};


template <
        typename ComponentList,
        typename IndexType,
        typename ElementType,
        Int BitsPerSymbol,
        typename BaseType
>
PageMetadata* SequenceDataPage<ComponentList, IndexType, ElementType, BitsPerSymbol, BaseType>::page_metadata_ = NULL;

#pragma pack()

}

#endif
