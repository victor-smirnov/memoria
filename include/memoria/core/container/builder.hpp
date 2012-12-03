
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_BUILDER_HPP
#define _MEMORIA_CORE_CONTAINER_BUILDER_HPP

#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/types/typelist.hpp>

#include <memoria/core/container/profile.hpp>

namespace memoria    {

using memoria::TypeList;
template <
    typename List,
    template <typename, typename> class Element,
    typename Base
>
struct Builder;


template <
    typename T1,
    typename ... T2,
    template <typename, typename> class Element,
    typename Base
>
struct Builder<TypeList<T1, T2...>, Element, Base>:
            public Element<
                        T1,
                        Builder<
                            TypeList<T2...>,
                            Element,
                            Base
                        >
            >
{

private:
    typedef Element<
                T1,
                Builder<
                    TypeList<T2...>,
                    Element,
                    Base
                >
            >                                                               BaseType;

public:
    
    Builder(): BaseType() {}
};

template <
    typename T,
    template <typename, typename> class Element,
    typename Base
>
struct Builder<TypeList<T>, Element, Base>: public Element<T, Base> {
private:
    typedef Element<T, Base>                                                BaseType;

public:
    Builder(): BaseType() {}
};


template <
    template <typename, typename> class Element,
    typename Base
>
struct Builder<TypeList<>, Element, Base>: public Base {


    typedef Base                                                            BaseType;


    Builder(): BaseType() {}
};



}

#endif
