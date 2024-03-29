
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/types.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/prototypes/bt/tools/bt_tools_tree_path.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_BT_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTBlockIteratorStateBase)
public:
    using Container = typename Base::Container;
    using Types     = typename Container::Types;

    using TreeNodePtr       = typename Types::TreeNodePtr;
    using TreeNodeConstPtr  = typename Types::TreeNodeConstPtr;
    using Position          = typename Types::Position;
    using CtrSizeT          = typename Types::CtrSizeT;

    using BlockIteratorState    = typename Base::Container::BlockIteratorState;
    using BlockIteratorStatePtr = typename Base::Container::BlockIteratorStatePtr;

    using CtrT = Ctr<Types>;

    using TreePathT = TreePath<TreeNodeConstPtr>;

private:

    TreePathT path_;

public:
    BTBlockIteratorStateBase():
        Base()
    {}


    BTBlockIteratorStateBase(const ThisType& other):
        Base(other),
        path_(other.path_)
    {}

    void prepare_ride(const ThisType& other) {
        path_ = other.path_;
    }


    void reset_state() {
        path_.reset_state();
        Base::reset_state();
    }

    void assign(const ThisType& other)
    {
        path_ = other.path_;
        Base::assign(other);
    }

    TreePathT& path() {
        return path_;
    }

    const TreePathT& path() const {
        return path_;
    }

    void set_path(const TreePathT& path) {
        path_ = path;
    }


    void dump(std::ostream& out = std::cout, const char* header = nullptr) const
    {
        auto& self = this->self();

        out << (header != NULL ? header : self.iter_get_dump_header()) << std::endl;

        return self.iter_dump_blocks(out);
    }

    U8String iter_get_dump_header() const {
        return self().ctr().type_name_str() + " Block Iterator State";
    }

    void dumpPath(std::ostream& out = std::cout, const char* header = nullptr) const
    {
        auto& self  = this->self();
        out << (header != NULL ? header : self.iter_get_dump_header()) << std::endl;
        self.ctr().ctr_dump_path(self.path(), 0, out);
        out << "======================================================================" << std::endl;
    }


    void iter_dump_header(std::ostream& out = std::cout) const
    {
        self().iter_dump_cache(out);
        self().iter_dump_keys(out);
        if (self().iter_leaf().node().isSet()) {
            std::cout << "Node ID: " << self().iter_leaf()->id() << std::endl;
        }
    }

    void iter_dump_blocks(std::ostream& out) const
    {
        auto& self = this->self();
        return self.ctr().ctr_dump_node(path_.leaf(), out);
    }

MEMORIA_BT_ITERATOR_BASE_CLASS_END

}
