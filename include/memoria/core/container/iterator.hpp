
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_ITERATOR_HPP
#define	_MEMORIA_CORE_CONTAINER_ITERATOR_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/typelist.hpp>

#include <memoria/core/container/names.hpp>

namespace memoria    {


template <typename Types> class Ctr;
template <typename Types> class Iter;
template <typename Name, typename Base, typename Types> class IterPart;


template <int Idx, typename Types>
class IterHelper: public IterPart<typename SelectByIndexTool<Idx, typename Types::List>::Result, IterHelper<Idx - 1, Types>, Types> {
	typedef Iter<Types> MyType;
	typedef IterHelper<Idx, Types> ThisType;
	typedef IterPart<typename SelectByIndexTool<Idx, typename Types::List>::Result, IterHelper<Idx - 1, Types>, Types> BaseType;

public:
	IterHelper(MyType& me): BaseType(me) {}
};

template <typename Types>
class IterHelper<-1, Types>: public Types::template BaseFactory<Types>::Type {
	typedef Iter<Types> MyType;
	typedef IterHelper<-1, Types> ThisType;

	typedef typename Types::template BaseFactory<Types>::Type BaseType;

public:
	IterHelper(MyType& me): BaseType(me) {}
};

template <typename Types>
class IterStart: public IterHelper<ListSize<typename Types::List>::Value - 1, Types> {
	typedef Iter<Types> MyType;

	typedef IterHelper<ListSize<typename Types::List>::Value - 1, Types> Base;
public:
	IterStart(MyType& me): Base(me) {}
};


template <
	typename TypesType
>
class IteratorBase: public TypesType::IteratorInterface {
public:

	typedef Ctr<typename TypesType::CtrTypes>                                       Container;
    typedef typename Container::Allocator                                           Allocator;
    typedef typename Allocator::Page::ID                                          	PageId;

    typedef Iter<TypesType>															MyType;

    enum {END = 1, START = 2, EMPTY = 3};
    
private:
    MyType& me_;
    Int     state_;
    
    Int hash_;

    Logger logger_;

public:
    IteratorBase(MyType &me):
    	me_(me), state_(0), hash_(0),
    	logger_("Iterator", Logger::DERIVED, &memoria::vapi::logger) //&me_.model().logger()
    {}

    bool operator==(const MyType& other) const {
    	return true;//state_ == other.state_;
    }

    bool operator!=(const MyType& other) const {
    	return state_ != other.state_;
    }

    Int hash() const {
    	return hash_;
    }

    Int BuildHash() const {
    	// threre is no meaningful fields in this class for hash;
    	return 0;
    }

    MyType &me() {
        return me_;
    }

    const MyType &me() const {
        return me_;
    }

    void setup(const MyType &other)
    {
        state_   	= other.state();
        hash_ 		= other.hash();
    }

    Int &state() {
        return state_;
    }

    void set_state(Int state) {
        state_ = state;
    }

    void reset_state() {
        state_ = 0;
    }

    const Int& state() const {
        return state_;
    }

    bool IsEnd() const {
        return (state() & END) != 0;
    }

    void SetStateBit(int flag, bool bit) {
        if (bit) {
            state_ |= flag;
        }
        else {
            state_ &= ~flag;
        }
    }

    void SetEnd(bool eof) {
        SetStateBit(END, eof);
    }

    bool IsStart() const {
        return (state_ & START) != 0;
    }

    void SetStart(bool bof) {
        SetStateBit(START, bof);
    }


    bool IsEmpty() const {
        return (state_ & EMPTY) == END + START;
    }

    bool IsNotEmpty() const {
        return (state_ & EMPTY) != END + START;
    }

//    void SetEmpty(bool empty) {
//    	SetStateBit(EMPTY, END + START);
//    }

    bool is_log(Int level)
    {
    	return logger_.IsLogEnabled(level);
    }

    memoria::vapi::Logger& logger()
    {
    	  return logger_;
    }

    const char* type_name() const {
        return me_.model().type_name();
    }

    void ReHash() {
    	hash_ = me_.BuildHash();
    }
};



template <typename Container> class IteratorFactoryName {};

template<
        typename Types
>
class Iter: public IterStart <Types>
{
    typedef IterStart<Types>                                       				Base;

    typedef Iter<Types>															ThisIteratorType;
    typedef Iter<Types>															MyType;
public:


    typedef Ctr<typename Types::CtrTypes>                                       ContainerType;
    typedef EmptyType															Txn;

protected:
    ContainerType&      model_;
public:
    
    Iter(MyType &me, ContainerType &model): Base(me), /*me_(me),*/ model_(model) {}

    ContainerType& model() {
        return model_;
    }

    const ContainerType& model() const {
        return model_;
    }

    void setup(const MyType &other) {
        Base::setup(other);
    }

    bool operator==(const MyType& other) const
    {
    	if (Base::hash() == other.hash())
    	{
    		return Base::operator==(other);
    	}
    	else {
    		return false;
    	}
    }

    bool operator!=(const MyType& other) const {
    	return !operator==(other);
    }

    const ThisIteratorType& operator=(const ThisIteratorType& other)
    {
    	setup(other);
    	Base::ReHash();
    	return *this;
    }
};

}

#endif
