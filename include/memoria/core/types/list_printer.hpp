
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_TYPES_LIST_PRINTER_HPP
#define _MEMORIA_CORE_TOOLS_TYPES_LIST_PRINTER_HPP

#include <iostream>
#include <typeinfo>
#include <memoria/core/types/hierarchy.hpp>
#include <memoria/core/tools/type_name.hpp>

namespace memoria    {

class TypePrinterBase
{
public:
    static void dump() {}
};

template<typename T, typename Base>
class TypePrinter : public Base
{
public:

    static void dump() {
        std::cout << memoria::vapi::TypeNameFactory<T, 4096*20>::name() << std::endl;
    }
};

template <typename List>
class TypePrinterTool : public SimpleHierarchy <List, TypePrinter, TypePrinterBase>
{
};

template <>
class TypePrinterTool<VTL<> >
{
public:
    static void dump()
    {
        std::cout << "Empty List" << std::endl;
    }
};

}
#endif  //_MEMORIA_CORE_TOOLS_TYPES_LIST_PRINTER_HPP
