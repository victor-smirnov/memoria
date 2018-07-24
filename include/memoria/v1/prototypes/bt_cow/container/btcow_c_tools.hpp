
// Copyright 2017 Victor Smirnov
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


#include <memoria/v1/prototypes/bt_cow/btcow_names.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>
#include <memoria/v1/core/tools/assert.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_CONTAINER_PART_BEGIN(btcow::ToolsName)

	using TreePath = typename TypesType::TreePath;

	using typename Base::NodeBaseG;
	using typename Base::BranchDispatcher;
	using typename Base::ID;

public:


void dumpPath(const TreePath& path, int32_t level, std::ostream& out = std::cout, int32_t depth = 100) const
{
    auto& self = this->self();

    out << "Path:" << std::endl;

    for (int32_t c = level; c < path.size(); c++)
    {
    	self.dump(path[c], out);
    }
}


protected:

NodeBaseG getNodeParent(const TreePath& path, int32_t level) const
{
    auto& self = this->self();

    if (level < path.size())
    {
    	return path[level - 1];
    }
    else {
    	return NodeBaseG();
    }
}

NodeBaseG getNodeParentForUpdate(const TreePath& path, int32_t level) const
{
    auto& self = this->self();

    if (level < path.size())
    {
    	path[level - 1].update();
    	return path[level - 1];
    }
    else {
    	return NodeBaseG();
    }
}

public:


bool getNextNodeP(TreePath& path, TreePath& next, int32_t level)
{
	auto& self = this->self();

	if (level < path.size() - 1)
	{
		NodeBaseG parent = path[level + 1];

		int32_t parent_size = self.getBranchNodeSize(parent);
		int32_t parent_idx  = self.findChildIdx(parent, path[level]->id());

		MEMORIA_V1_ASSERT(parent_idx, >=, 0);

		if (parent_idx < parent_size - 1)
		{
			next[level] = self.getChild(parent, parent_idx + 1);
			return true;
		}
		else if (getNextNodeP(path, next, level + 1))
		{
			next[level] = self.getChild(next[level + 1], 0);
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}


bool getPrevNodeP(TreePath& path, TreePath& prev, int32_t level)
{
	if (level < path.size() - 1)
	{
		NodeBaseG parent = path[level + 1];
		int32_t parent_idx  = self.findChildIdx(parent, path[level]->id());

		MEMORIA_V1_ASSERT(parent_idx, >=, 0);

		if (parent_idx > 0)
		{
			prev[level] = self.getChild(parent, parent_idx - 1);

			return true;
		}
		else if (getPrevNodeP(path, prev, level + 1))
		{
			prev[level] = self.getLastChild(prev[level + 1]);
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}


protected:

    MEMORIA_V1_DECLARE_NODE_FN_RTN(FindChildIdx, find_child_idx, int32_t);
    int32_t findChildIdx(const NodeBaseG& node, const ID& child_id)
    {
    	return BranchDispatcher::dispatch(node, FindChildIdx(), child_id);
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btcow::ToolsName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS





//M_PARAMS
//typename M_TYPE::NodeBaseG M_TYPE::getNextNodeP(NodeBaseG& node) const
//{
//    auto& self = this->self();
//
//    if (!node->is_root())
//    {
//        NodeBaseG parent = self.getNodeParent(node);
//
//        int32_t size = self.getNodeSize(parent, 0);
//
//        int32_t parent_idx = node->parent_idx();
//
//        if (parent_idx < size - 1)
//        {
//            return self.getChild(parent, parent_idx + 1);
//        }
//        else {
//            NodeBaseG target_parent = getNextNodeP(parent);
//
//            if (target_parent.isSet())
//            {
//                return self.getChild(target_parent, 0);
//            }
//            else {
//                return target_parent;
//            }
//        }
//    }
//    else {
//        return NodeBaseG();
//    }
//}
//
//
//M_PARAMS
//typename M_TYPE::NodeBaseG M_TYPE::getPrevNodeP(NodeBaseG& node) const
//{
//    auto& self = this->self();
//
//    if (!node->is_root())
//    {
//        NodeBaseG parent = self.getNodeParent(node);
//
//        int32_t parent_idx = node->parent_idx();
//
//        if (parent_idx > 0)
//        {
//            return self.getChild(parent, parent_idx - 1);
//        }
//        else {
//            NodeBaseG target_parent = getPrevNodeP(parent);
//
//            if (target_parent.isSet())
//            {
//                int32_t node_size = self.getNodeSize(target_parent, 0);
//                return self.getChild(target_parent, node_size - 1);
//            }
//            else {
//                return target_parent;
//            }
//        }
//    }
//    else {
//        return NodeBaseG();
//    }
//}

#undef M_TYPE
#undef M_PARAMS

}}
