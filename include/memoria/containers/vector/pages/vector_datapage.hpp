
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ARRAY_PAGES_DATA_PAGE_HPP
#define _MEMORIA_MODELS_ARRAY_PAGES_DATA_PAGE_HPP

#include <memoria/core/tools/reflection.hpp>
#include <memoria/prototypes/balanced_tree/pages/tree_node.hpp>

namespace memoria    	{
namespace mvector		{



template <
	typename Types,
	bool root, bool leaf
>
class VectorDataNode: public memoria::balanced_tree::RootPage<typename Types::Metadata, typename Types::NodePageBase, root>
{
public:

    static const UInt VERSION                                                   = 1;

    typedef TreeMapNode<Types, root, leaf>                                      MyType;

    typedef memoria::balanced_tree::RootPage<
    			typename Types::Metadata,
    			typename Types::NodePageBase, root
    >  																			Base;

public:


    typedef typename Types::ElementType                                      	ElementType;

    typedef ElementType*														Sequence;

private:

    Int size_;
    ElementType sequence_[];

public:

    VectorDataNode(): Base(), sequence_() {}

    void init(Int block_size)
    {
    	Base::init();
    }

    const Int& size() const {
    	return size_;
    }

    Int& size() {
    	return size_;
    }

    void reindex() {}

    Int data_size() const
    {
    	return sizeof(MyType) + size_ * sizeof(ElementType);
    }

    Int getCapacity() const
    {
    	return getMaxCapacity() - size_;
    }

    Int capacity() const
    {
    	return getCapacity();
    }

    Int getMaxCapacity() const
    {
    	Int data_size 	 = Base::page_size() - sizeof(MyType);

    	return data_size / sizeof(ElementType);
    }

    Int max_size() const
    {
    	return getMaxCapacity();
    }

    static Int getMaxPageCapacity(Int size)
    {
    	Int data_size 	 = size - sizeof(MyType);

    	return data_size / sizeof (ElementType);
    }

    void reindexAll(Int from, Int to)
    {}

    const ElementType* values() const {
    	return sequence_;
    }

    ElementType* values() {
    	return sequence_;
    }

    Sequence sequence() {
    	return sequence_;
    }

    const Sequence sequence() const {
    	return sequence_;
    }

    ElementType& value(Int idx) {
    	return sequence_[idx];
    }

    const ElementType& value(Int idx) const {
    	return sequence_[idx];
    }

    void insertSpace(Int from, Int length)
    {
    	shift(from, length);
    }

    void shift(BigInt pos, BigInt length)
    {
    	CopyBuffer(sequence_ + pos, sequence_ + pos + length, size_ - pos);
    }

    void copyTo(MyType* other, Int from, Int to, Int length) const
    {
    	CopyBuffer(sequence_ + from, other->sequence_ + to, length);
    }

    void copyFrom(const MyType* src)
    {
    	Base::copyFrom(src);

    	CopyBuffer(src->sequence_, sequence_, src->size_);

    	size_ = src->size_;
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::generateDataEvents(handler);

    	handler->startGroup("DATA");

    	handler->value("SIZE", &size_);
    	handler->value("VALUE", sequence_, size_, IPageDataEventHandler::BYTE_ARRAY);

    	handler->endGroup();
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
    	Base::template serialize<FieldFactory>(buf);

    	FieldFactory<Int>::serialize(buf, size_);
    	FieldFactory<ElementType>::serialize(buf, sequence_[0], size_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
    	Base::template deserialize<FieldFactory>(buf);

    	FieldFactory<Int>::deserialize(buf, size_);
    	FieldFactory<ElementType>::deserialize(buf, sequence_[0], size_);
    }
};





template <
        typename ComponentList,
        typename ElementType_,
        typename Base0
>
class VectorDataPage: public PageBuilder<ComponentList, Base0>
{
    static const UInt VERSION = 1;

public:

    typedef VectorDataPage<
                ComponentList,
                ElementType_,
                Base0
    >                                                                           MyType;

    typedef PageBuilder<ComponentList, Base0>                                   Base;

    typedef typename MergeLists <
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                Int,
                ElementType_
    >::Result                                                                   FieldsList;

    static const UInt PAGE_HASH = md5::Md5Sum<typename TypeToValueList<FieldsList>::Type>::Result::Value32;

    typedef ElementType_                                      					ElementType;

    typedef ElementType*														Sequence;

private:

    Int size_;
    ElementType sequence_[];

    static PageMetadata *page_metadata_;

public:

    VectorDataPage(): Base(), sequence_() {}

    void init(Int block_size)
    {
    	Base::init();
    }

    const Int& size() const {
        return size_;
    }

    Int& size() {
    	return size_;
    }

    static Int hash() {
        return PAGE_HASH;
    }

    static PageMetadata *page_metadata()
    {
        return page_metadata_;
    }

    void reindex() {}

    Int data_size() const
    {
        return sizeof(MyType) + size_ * sizeof(ElementType);
    }

    Int getCapacity() const
    {
    	return getMaxCapacity() - size_;
    }

    Int getMaxCapacity() const
    {
    	Int data_size 	 = Base::page_size() - sizeof(MyType);

    	return data_size / sizeof(ElementType);
    }

    static Int getMaxPageCapacity(Int size)
    {
    	Int data_size 	 = size - sizeof(MyType);

    	return data_size / sizeof (ElementType);
    }

    const ElementType* values() const {
    	return sequence_;
    }

    ElementType* values() {
    	return sequence_;
    }

    Sequence sequence() {
    	return sequence_;
    }

    const Sequence sequence() const {
    	return sequence_;
    }

    ElementType& value(Int idx) {
    	return sequence_[idx];
    }

    const ElementType& value(Int idx) const {
    	return sequence_[idx];
    }

    void shift(BigInt pos, BigInt length)
    {
    	CopyBuffer(sequence_ + pos, sequence_ + pos + length, size_ - pos);
    }

    void copyTo(MyType* other, Int from, Int to, Int length) const
    {
    	CopyBuffer(sequence_ + from, other->sequence_ + to, length);
    }

    void copyFrom(const MyType* src)
    {
    	Base::copyFrom(src);

    	CopyBuffer(src->sequence_, sequence_, src->size_);

    	size_ = src->size_;
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->startGroup("DATA");

        handler->value("SIZE", &size_);
        handler->value("VALUE", sequence_, size_, IPageDataEventHandler::BYTE_ARRAY);

        handler->endGroup();
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        FieldFactory<Int>::serialize(buf, size_);
        FieldFactory<ElementType>::serialize(buf, sequence_[0], size_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        FieldFactory<Int>::deserialize(buf, size_);
        FieldFactory<ElementType>::deserialize(buf, sequence_[0], size_);
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
        typename DataBlock,
        typename BaseType
>
PageMetadata* VectorDataPage<ComponentList, DataBlock, BaseType>::page_metadata_ = NULL;

}


template <typename Types, bool root, bool leaf>
struct TypeHash<memoria::mvector::VectorDataNode<Types, root, leaf> > {

	typedef memoria::mvector::VectorDataNode<Types, root, leaf> Node;

    static const UInt Value = HashHelper<
    		TypeHash<typename Node::Base>::Value,
    		Node::VERSION,
    		root,
    		leaf,
    		TypeHash<typename Types::Name>::Value,
    		TypeHash<typename Types::ElementType>::Value
    >::Value;
};


}

#endif
