
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_ITERATOR_HPP
#define _MEMORIA_CORE_CONTAINER_ITERATOR_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/typelist.hpp>

#include <memoria/core/container/names.hpp>

#include <memoria/core/container/logs.hpp>

namespace memoria    {

using namespace memoria::vapi;

template <typename Types> class Ctr;
template <typename Types> class Iter;
template <typename Name, typename Base, typename Types> class IterPart;


template <int Idx, typename Types>
class IterHelper: public IterPart<typename SelectByIndexTool<Idx, typename Types::List>::Result, IterHelper<Idx - 1, Types>, Types> {
    typedef Iter<Types>                 MyType;
    typedef IterHelper<Idx, Types>      ThisType;
    typedef IterPart<typename SelectByIndexTool<Idx, typename Types::List>::Result, IterHelper<Idx - 1, Types>, Types> BaseType;

public:
    IterHelper(): BaseType() {}
    IterHelper(ThisType&& other): BaseType(std::move(other)) {}
    IterHelper(const ThisType& other): BaseType(other) {}
};

template <typename Types>
class IterHelper<-1, Types>: public Types::template BaseFactory<Types>::Type {
    typedef Iter<Types>                 MyType;
    typedef IterHelper<-1, Types>       ThisType;

    typedef typename Types::template BaseFactory<Types>::Type BaseType;

public:
    IterHelper(): BaseType() {}
    IterHelper(ThisType&& other): BaseType(std::move(other)) {}
    IterHelper(const ThisType& other): BaseType(other) {}
};

template <typename Types>
class IterStart: public IterHelper<ListSize<typename Types::List>::Value - 1, Types> {
    typedef Iter<Types>                 MyType;
    typedef IterStart<Types>            ThisType;

    typedef IterHelper<ListSize<typename Types::List>::Value - 1, Types> Base;
public:
    IterStart(): Base() {}
    IterStart(ThisType&& other): Base(std::move(other)) {}
    IterStart(const ThisType& other): Base(other) {}
};


template <
    typename TypesType
>
class IteratorBase: public TypesType::IteratorInterface {
    typedef IteratorBase<TypesType>                                                 ThisType;

public:

    typedef Ctr<typename TypesType::CtrTypes>                                       Container;
    typedef typename Container::Allocator                                           Allocator;
    typedef typename Allocator::Page::ID                                            PageId;

    typedef Iter<TypesType>                                                         MyType;

    enum {NORMAL = 0, END = 1, START = 2, EMPTY = 3};
    
private:
    Logger  logger_;

    Int type_;

public:
    IteratorBase():
        logger_("Iterator", Logger::DERIVED, &memoria::vapi::logger),
        type_(NORMAL)
    {}

    IteratorBase(ThisType&& other): logger_(std::move(other.logger_)), type_(other.type_) {}
    IteratorBase(const ThisType& other): logger_(other.logger_), type_(other.type_)       {}

    const Int& type() const {
        return type_;
    }

    Int& type() {
        return type_;
    }

    bool isEqual(const ThisType& other) const
    {
        return true;
    }

    bool isNotEqual(const ThisType& other) const
    {
        return false;
    }

    void assign(const ThisType& other)
    {
        logger_ = other.logger_;
        type_   = other.type_;
    }

    void assign(ThisType&& other)
    {
        logger_ = std::move(other.logger_);
        type_   = other.type_;
    }

    MyType* me() {
        return static_cast<MyType*>(this);
    }

    const MyType* me() const {
        return static_cast<const MyType*>(this);
    }


    bool is_log(Int level)
    {
        return logger_.isLogEnabled(level);
    }

    memoria::vapi::Logger& logger()
    {
          return logger_;
    }

    const char* typeName() const {
        return me()->model().typeName();
    }


    void init() {}
};



template <typename Container> class IteratorFactoryName {};

template<
        typename Types
>
class Iter: public IterStart <Types>
{
    typedef IterStart<Types>                                                    Base;

    typedef Iter<Types>                                                         ThisIteratorType;
    typedef Iter<Types>                                                         MyType;
public:


    typedef Ctr<typename Types::CtrTypes>                                       ContainerType;


protected:
    ContainerType&      model_;

public:
    
    Iter(ContainerType &model): model_(model) {}

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
        return Base::operator==(other);
    }

    bool operator!=(const MyType& other) const {
        return !operator==(other);
    }

    ThisIteratorType& operator=(ThisIteratorType&& other)
    {
        if (this != &other)
        {
            Base::operator=(std::move(other));
        }

        return *this;
    }
};

}

#endif
