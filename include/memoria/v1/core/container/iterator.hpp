
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/typelist.hpp>

#include <memoria/v1/core/container/names.hpp>

#include <memoria/v1/core/container/logs.hpp>

#include <memory>

namespace memoria {
namespace v1 {



template <typename Types> class Ctr;
template <typename Types> class Iter;
template <typename Name, typename Base, typename Types> class IterPart;


template <int Idx, typename Types>
class IterHelper: public IterPart<
                            SelectByIndex<Idx,typename Types::List>,
                            IterHelper<Idx - 1, Types>, Types
                         >
{
    using MyType    = Iter<Types>;
    using ThisType  = IterHelper<Idx, Types>;
    using BaseType  = IterPart<
                SelectByIndex<Idx, typename Types::List>,
                IterHelper<Idx - 1, Types>, Types
    >;

public:
    IterHelper(): BaseType() {}
    IterHelper(ThisType&& other): BaseType(std::move(other)) {}
    IterHelper(const ThisType& other): BaseType(other) {}
};

template <typename Types>
class IterHelper<-1, Types>: public Types::template BaseFactory<Types>::Type {

    typedef Iter<Types>                                                             MyType;
    typedef IterHelper<-1, Types>                                                   ThisType;

    typedef typename Types::template BaseFactory<Types>::Type BaseType;

public:
    IterHelper(): BaseType() {}
    IterHelper(ThisType&& other): BaseType(std::move(other)) {}
    IterHelper(const ThisType& other): BaseType(other) {}
};

template <typename Types>
class IterStart: public IterHelper<ListSize<typename Types::List>::Value - 1, Types> {

    using MyType    = Iter<Types>;
    using ThisType  = IterStart<Types>;
    using Base      = IterHelper<ListSize<typename Types::List>::Value - 1, Types>;
    using ContainerType = Ctr<typename Types::CtrTypes>;

    using CtrPtr    = std::shared_ptr<ContainerType>;

    CtrPtr ctr_ptr_;
    ContainerType* model_;


public:
    IterStart(const CtrPtr& ptr): Base(), ctr_ptr_(ptr), model_(ptr.get()) {}
    IterStart(ThisType&& other): Base(std::move(other)), ctr_ptr_(std::move(other.ctr_ptr_)), model_(other.model_) {}
    IterStart(const ThisType& other): Base(other), ctr_ptr_(other.ctr_ptr_), model_(other.model_) {}

    ContainerType& model() {
        return *model_;
    }

    const ContainerType& model() const {
        return *model_;
    }

    ContainerType& ctr() {
        return *model_;
    }

    const ContainerType& ctr() const {
        return *model_;
    }
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
    Logger logger_;

    Int type_;

public:
    IteratorBase():
        logger_("Iterator", Logger::DERIVED, &v1::logger),
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

    v1::Logger& logger()
    {
          return logger_;
    }

    const char* typeName() const {
        return me()->model().typeName();
    }
};



template <typename Container> class IteratorFactoryName {};

/*
template<
        typename Types
>
class Iter: public IterStart <Types>
{
    typedef IterStart<Types>                                                        Base;

    typedef Iter<Types>                                                             ThisIteratorType;
    typedef Iter<Types>                                                             MyType;
public:


    typedef Ctr<typename Types::CtrTypes>                                           ContainerType;


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

    ContainerType& ctr() {
        return model_;
    }

    const ContainerType& ctr() const {
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
*/


}}