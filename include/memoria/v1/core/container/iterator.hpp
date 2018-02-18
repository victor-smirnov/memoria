
// Copyright 2011 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/config.hpp>
#include <memoria/v1/core/types/typelist.hpp>
#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/container/defaults.hpp>

#include <memoria/v1/core/container/logs.hpp>

#include <memoria/v1/core/tools/memory.hpp>

#include <memory>
#include "../tools/pair.hpp"

namespace memoria {
namespace v1 {



template <typename Types> class Ctr;
template <typename Types> class Iter;
template <typename Name, typename Base, typename Types> class IterPart;


template <int Idx, typename Types1>
class IterHelper: public IterPart<
                            SelectByIndex<Idx,typename Types1::List>,
                            IterHelper<Idx - 1, Types1>, Types1
                         >
{
    using MyType    = Iter<Types1>;
    using ThisType  = IterHelper<Idx, Types1>;
    using BaseType  = IterPart<
                SelectByIndex<Idx, typename Types1::List>,
                IterHelper<Idx - 1, Types1>, Types1
    >;

public:
    IterHelper(): BaseType() {}
    IterHelper(ThisType&& other): BaseType(std::move(other)) {}
    IterHelper(const ThisType& other): BaseType(other) {}
};



template <typename Types1>
class IterHelper<-1, Types1>: public Types1::template BaseFactory<Types1>::Type {

    typedef Iter<Types1>                                                             MyType;
    typedef IterHelper<-1, Types1>                                                   ThisType;

    typedef typename Types1::template BaseFactory<Types1>::Type BaseType;

public:
    IterHelper(): BaseType() {}
    IterHelper(ThisType&& other): BaseType(std::move(other)) {}
    IterHelper(const ThisType& other): BaseType(other) {}
};

template <typename Types1>
class IterStart: public IterHelper<ListSize<typename Types1::List> - 1, Types1> {

    using MyType    = Iter<Types1>;
    using ThisType  = IterStart<Types1>;
    using Base      = IterHelper<ListSize<typename Types1::List> - 1, Types1>;
    using ContainerType = Ctr<typename Types1::CtrTypes>;

    using CtrPtr    = CtrSharedPtr<ContainerType>;

    CtrPtr ctr_ptr_;
    ContainerType* model_;


public:
    IterStart(): Base(), ctr_ptr_(), model_() {}

    IterStart(CtrPtr ptr): Base(), ctr_ptr_(std::move(ptr)), model_(ctr_ptr_.get()) {
    	//std::cout << "Create Iterator: " << ctr_ptr_.use_count() << "\n";
    }
    IterStart(ThisType&& other): Base(std::move(other)), ctr_ptr_(std::move(other.ctr_ptr_)), model_(other.model_) {}
    IterStart(const ThisType& other): Base(other), ctr_ptr_(other.ctr_ptr_), model_(other.model_) {}

    virtual ~IterStart() {
    	//std::cout << "Destroy Iterator: " << ctr_ptr_.use_count() << "\n";
    }

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
    
    CtrPtr& ctr_ptr() {return ctr_ptr_;}
    const CtrPtr& ctr_ptr() const {return ctr_ptr_;}
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

    int32_t type_;

    PairPtr pair_;

public:
    IteratorBase():
        logger_("Iterator", Logger::DERIVED, &v1::logger),
        type_(NORMAL)
    {}

    IteratorBase(ThisType&& other): logger_(std::move(other.logger_)), type_(other.type_) {}
    IteratorBase(const ThisType& other): logger_(other.logger_), type_(other.type_)       {}


    PairPtr& pair() {return pair_;}
    const PairPtr& pair() const {return pair_;}


    const int32_t& type() const {
        return type_;
    }

    int32_t& type() {
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


    bool is_log(int32_t level)
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


}}
