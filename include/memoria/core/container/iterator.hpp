
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
	typedef Iter<Types> 				MyType;
	typedef IterHelper<Idx, Types> 		ThisType;
	typedef IterPart<typename SelectByIndexTool<Idx, typename Types::List>::Result, IterHelper<Idx - 1, Types>, Types> BaseType;

public:
	IterHelper(): BaseType() {}
	IterHelper(ThisType&& other): BaseType(std::move(other)) {}
	IterHelper(const ThisType& other): BaseType(other) {}

	void operator=(const ThisType& other) {
		BaseType::operator=(other);
	}
};

template <typename Types>
class IterHelper<-1, Types>: public Types::template BaseFactory<Types>::Type {
	typedef Iter<Types> 				MyType;
	typedef IterHelper<-1, Types> 		ThisType;

	typedef typename Types::template BaseFactory<Types>::Type BaseType;

public:
	IterHelper(): BaseType() {}
	IterHelper(ThisType&& other): BaseType(std::move(other)) {}
	IterHelper(const ThisType& other): BaseType(other) {}

	void operator=(const ThisType& other) {
		BaseType::operator=(other);
	}
};

template <typename Types>
class IterStart: public IterHelper<ListSize<typename Types::List>::Value - 1, Types> {
	typedef Iter<Types> 				MyType;
	typedef IterStart<Types> 			ThisType;

	typedef IterHelper<ListSize<typename Types::List>::Value - 1, Types> Base;
public:
	IterStart(): Base() {}
	IterStart(ThisType&& other): Base(std::move(other)) {}
	IterStart(const ThisType& other): Base(other) {}

	void operator=(const ThisType& other) {
		Base::operator=(other);
	}
};


template <
	typename TypesType
>
class IteratorBase: public TypesType::IteratorInterface {
	typedef IteratorBase<TypesType>													ThisType;

public:

	typedef Ctr<typename TypesType::CtrTypes>                                       Container;
    typedef typename Container::Allocator                                           Allocator;
    typedef typename Allocator::Page::ID                                          	PageId;

    typedef Iter<TypesType>															MyType;

    enum {END = 1, START = 2, EMPTY = 3};
    
private:
    Int     state_;
    Int 	hash_;
    Logger 	logger_;

public:
    IteratorBase():
    	state_(0), hash_(0),
    	logger_("Iterator", Logger::DERIVED, &memoria::vapi::logger)
    {}

    IteratorBase(ThisType&& other): state_(other.state_), hash_(other.hash_), logger_(other.logger_) {}
    IteratorBase(const ThisType& other): state_(other.state_), hash_(other.hash_), logger_(other.logger_) {}

    bool operator==(const MyType& other) const {
    	return true;//state_ == other.state_;
    }

    bool operator!=(const MyType& other) const {
    	return state_ != other.state_;
    }

    void operator=(const ThisType& other)
    {
    	state_ 	= other.state_;
    	hash_ 	= other.hash_;
    	logger_ = other.logger_;
    }

    void operator=(ThisType&& other)
    {
    	state_ 	= other.state_;
    	hash_ 	= other.hash_;
    	logger_ = other.logger_;
    }

    Int hash() const {
    	return hash_;
    }

    Int BuildHash() const {
    	// threre is no meaningful fields in this class for hash;
    	return 0;
    }

    MyType* me() {
    	return static_cast<MyType*>(this);
    }

    const MyType* me() const {
    	return static_cast<const MyType*>(this);
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

    void SetStateBit(int flag, bool bit)
    {
        if (bit) {
            state_ |= flag;
        }
        else {
            state_ &= ~flag;
        }
    }

    bool is_log(Int level)
    {
    	return logger_.IsLogEnabled(level);
    }

    memoria::vapi::Logger& logger()
    {
    	  return logger_;
    }

    const char* type_name() const {
        return me()->model().type_name();
    }

    void ReHash() {
    	hash_ = me()->BuildHash();
    }

    void Init() {}
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

    typedef typename Types::IteratorData										IteratorData;


protected:
    ContainerType&      model_;

    IteratorData		iter_data_;

public:
    
    Iter(ContainerType &model, const IteratorData& iter_data): model_(model), iter_data_(iter_data) {}

    IteratorData& iter_data()
    {
    	return iter_data_;
    }

    const IteratorData& iter_data() const
    {
    	return iter_data_;
    }

    MyType* me() {
    	return this;
    }

    const MyType* me() const {
    	return this;
    }

    ContainerType& model() {
        return model_;
    }

    const ContainerType& model() const {
        return model_;
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

    ThisIteratorType& operator=(ThisIteratorType&& other)
    {
    	Base::operator=(std::move(other));
    	Base::ReHash();
    	return *this;
    }
};

}

#endif
