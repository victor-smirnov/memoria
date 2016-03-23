
// Copyright 2015 Victor Smirnov
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


#include <memoria/v1/core/types/types.hpp>


#include <iostream>
#include <vector>
#include <list>
#include <memory>

#include <cstdlib>

using namespace std;
using namespace memoria;

constexpr Int MAX_NODE_SIZE = 10;

class TreeNode;
using TreeNodePtr = shared_ptr<TreeNode>;

class BranchNode;
class LeafNode;


using BranchNodePtr = shared_ptr<BranchNode>;
using LeafNodePtr = shared_ptr<LeafNode>;

using DataList = vector<vector<Int>>;

template <typename T>
ostream& operator<<(ostream& out, const vector<T>& data) {

    out<<"[";

    for (const auto& v: data) {
        out<<v<<", ";
    }

    out<<"]";

    return out;
}

class Tree;
using TreePtr = shared_ptr<::Tree>;

class TreeNode {
public:
    virtual ~TreeNode() {}

    virtual void dump(Int level = 0) = 0;

    virtual Int count_leafs() const = 0;

    virtual bool checkTree(const DataList& data, Int& start) = 0;

    virtual LeafNodePtr find_leaf(Int idx, Int& current, ::Tree& tree) = 0;

    virtual bool is_leaf() const = 0;

    virtual BranchNodePtr parentPtr(TreePtr tree) = 0;
    virtual void set_parent(BranchNode* parent) = 0;
};


class Tree {
    TreeNodePtr root_;
public:
    Tree() {}

    Tree(TreeNodePtr root): root_(root) {}

    TreeNodePtr get_root() {
        return root_;
    }

    void set_root(TreeNodePtr ptr)
    {
        root_ = ptr;
    }

    void dump() {
        root_->dump();
    }

    virtual LeafNodePtr find_leaf(Int idx) {
        Int current = 0;
        return root_->find_leaf(idx, current, *this);
    }

};






template <typename Value>
class Node: public TreeNode {
protected:
    vector<Value> values_;
    BranchNode* parent_ = nullptr;

public:
    virtual ~Node() {}

    const BranchNode* parent() const {
        return parent_;
    }

    BranchNode* parent() {
        return parent_;
    }

    virtual void set_parent(BranchNode* parent) {
        parent_ = parent;
    }

    Int size() const {
        return values_.size();
    }

    Int capacity() const {
        return MAX_NODE_SIZE - values_.size();
    }

    Value& value(Int idx) {
        return values_[idx];
    }

    const Value& value(Int idx) const {
        return values_[idx];
    }

    void insert(Value val, Int at)
    {
        values_.insert(values_.begin() + at, val);
    }

    void append(Value val) {
        values_.push_back(val);
    }

    void remove(Int from, Int to)
    {
        values_.erase(values_.begin() + from, values_.begin() + to);
    }

    const vector<Value>& values() const {
        return values_;
    }
};


using TreeNodePtr = shared_ptr<TreeNode>;

class BranchNode: public Node<TreeNodePtr> {

    using Base = Node<TreeNodePtr>;

public:
    virtual ~BranchNode() {}

    virtual void dump(Int level)
    {
        for (Int c = 0; c < level * 4; c++) cout<<" ";
        cout<<"Branch "<<this<<" of "<<Base::values_.size()<<" {"<<endl;
        for (auto& e: Base::values_)
        {
            e->dump(level + 1);
        }
        for (Int c = 0; c < level * 4; c++) cout<<" ";
        cout<<"}"<<endl;
    }

    void insert(TreeNodePtr val, Int at)
    {
        values_.insert(values_.begin() + at, val);
        val->set_parent(this);
    }

    void append(TreeNodePtr val)
    {
        values_.push_back(val);
        val->set_parent(this);
    }

    virtual BranchNodePtr split(Int at, TreePtr tree)
    {
        BranchNodePtr sibling = make_shared<BranchNode>();
        auto& values = values_;

        for (Int c = at; c < values.size(); c++)
        {
            sibling->append(values[c]);
        }

        values.erase(values.begin() + at, values.end());

        if (parent_ != nullptr)
        {
            parent_->insert_child(this, sibling, tree);
        }
        else {
            shared_ptr<BranchNode> parent = make_shared<BranchNode>();
            parent_ = parent.get();

            parent->append(tree->get_root());
            parent->append(sibling);

            tree->set_root(parent);
        }

        return sibling;
    }


    Int find_child_idx(const TreeNode* child)
    {
        Int idx = -1;

        for (Int c = 0; c < this->values_.size(); c++)
        {
            if (this->values_[c].get() == child)
            {
                idx = c;
            }
        }

        if (idx < 0) {
            throw "Invalid tree structure";
        }

        return idx;
    }



    void insert_child(const TreeNode* child, TreeNodePtr new_child, TreePtr tree)
    {
        Int idx = find_child_idx(child);

        if (capacity() == 0)
        {
            int split_at = this->size() / 2;
            auto next = split(split_at, tree);

            if (idx < size())
            {
                this->insert(new_child, idx);
            }
            else {
                next->insert(new_child, idx - this->size());
            }
        }
        else {
            this->insert(new_child, idx + 1);
        }
    }

    virtual BranchNodePtr parentPtr(TreePtr tree)
    {
        if (this->parent() != nullptr)
        {
            auto target = this->parent()->parent();

            if (target != nullptr)
            {
                Int idx = target->find_child_idx(this->parent());
                return dynamic_pointer_cast<BranchNode>(target->value(idx));
            }
            else {
                return dynamic_pointer_cast<BranchNode>(tree->get_root());
            }
        }
        else {
            return BranchNodePtr();
        }
    }

    virtual Int count_leafs() const {
        Int cnt = 0;

        for (auto child: this->values_) {
            cnt += child->count_leafs();
        }

        return cnt;
    }

    virtual bool checkTree(const DataList& data, Int& start) {
        for (auto child: this->values_)
        {
            if (!child->checkTree(data, start))
            {
                return false;
            }
        }
        return true;
    }

    virtual LeafNodePtr find_leaf(Int idx, Int& current, ::Tree& tree)
    {
        for (auto child: this->values_)
        {
            auto leaf = child->find_leaf(idx, current, tree);

            if (leaf != nullptr) {
                return leaf;
            }
        }

        return LeafNodePtr();
    }

    virtual bool is_leaf() const {return false;}
};



class LeafNode: public Node<Int> {
    using Base = Node<Int>;



public:
    virtual ~LeafNode() {}



    virtual void dump(Int level)
    {
        for (Int c = 0; c < level*4; c++) cout<<" ";

        cout<<"Leaf "<<this<<" of "<<Base::values_.size()<<" [";
        for (auto e: Base::values_)
        {
            cout<<e<<", ";
        }
        cout<<"]"<<endl;
    }

    virtual BranchNodePtr parentPtr(TreePtr tree)
    {
        if (this->parent() != nullptr)
        {
            auto target = this->parent()->parent();

            if (target != nullptr)
            {
                Int idx = target->find_child_idx(this->parent());
                return dynamic_pointer_cast<BranchNode>(target->value(idx));
            }
            else {
                return dynamic_pointer_cast<BranchNode>(tree->get_root());
            }
        }
        else {
            return BranchNodePtr();
        }
    }

    virtual LeafNodePtr split(Int at, TreePtr tree)
    {
        LeafNodePtr sibling = make_shared<LeafNode>();
        auto& values = values_;

        for (Int c = 0; c < values.size() - at; c++)
        {
            sibling->append(values[c]);
        }

        values.erase(values.begin() + at, values.end());

        if (parent_ != nullptr)
        {
            parent_->insert_child(this, sibling, tree);
        }
        else {
            shared_ptr<BranchNode> parent = make_shared<BranchNode>();
            parent_ = parent.get();

            parent->append(tree->get_root());
            parent->append(sibling);

            tree->set_root(parent);
        }

        return sibling;
    }



    void insert_value(Int idx, Int value, TreePtr tree)
    {
        if (capacity() == 0)
        {
            int split_at = this->size() / 2;
            auto next = split(split_at, tree);

            if (idx < size())
            {
                this->insert(value, idx);
            }
            else {
                next->insert(value, idx - this->size());
            }
        }
        else {
            this->insert(value, idx);
        }
    }

    virtual Int count_leafs() const {
        return 1;
    }

    virtual bool checkTree(const DataList& data, Int& start)
    {
        const auto& dataL = data[start];

        if (this->size() == dataL.size())
        {
            for (Int c = 0; c < dataL.size(); c++)
            {
                if (dataL[c] != this->values_[c])
                {
                    cout<<"Leaf data content mismatch at "<<start<<": "<<dataL<<" -- "<<this->values_<<endl;

                    return false;
                }
            }

            start++;
            return true;
        }
        else {
            cout<<"Leaf data size mismatch at "<<start<<": "<<data[start].size()<<" -- "<<this->values_.size()<<endl;
            return false;
        }
    }

    virtual LeafNodePtr find_leaf(Int idx, Int& current, ::Tree& tree)
    {
        if (idx == current)
        {
            if (this->parent() != nullptr)
            {
                Int idx = this->parent()->find_child_idx(this);
                return dynamic_pointer_cast<LeafNode>(this->parent()->value(idx));
            }
            else {
                return dynamic_pointer_cast<LeafNode>(tree.get_root());
            }
        }
        else {
            current++;
            return LeafNodePtr();
        }
    }

    virtual bool is_leaf() const {return true;}
};






class Subtree {
    TreeNodePtr node_;
    Int size_;

public:
    Subtree(TreeNodePtr node, Int size): node_(node), size_(size) {}
    Subtree(): size_(0) {}

    TreeNodePtr node() {
        return node_;
    }

    const TreeNodePtr node() const {
        return node_;
    }

    Int size() const {
        return size_;
    }
};


class Checkpoint {
    using Iterator = list<LeafNodePtr>::iterator;

    Iterator iterator_;
    Int journal_size_;
public:
    Checkpoint(Iterator iter, Int size): iterator_(iter), journal_size_(size) {}

    Iterator iterator() const {return iterator_;};
    Int journal_size() const {return journal_size_;};
};


struct ILeafProvider {
    virtual LeafNodePtr get_leaf()  = 0;

    virtual Checkpoint checkpoint() = 0;

    virtual void commit()           = 0;
    virtual void rollback(const Checkpoint& chekpoint) = 0;

    virtual Int size() const        = 0;
};

Subtree BuildSubtree(ILeafProvider& leaf_provider, Int level, BranchNode* parent = nullptr)
{
    if (leaf_provider.size() > 0)
    {
        if (level > 1)
        {
            BranchNodePtr node = make_shared<BranchNode>();

            Int max = 2 + rand() % (MAX_NODE_SIZE - 2);

            Int cnt = 0;

            for (Int c = 0; c < max && leaf_provider.size() > 0; c++)
            {
                Checkpoint checkpoint = leaf_provider.checkpoint();

                Subtree subtree = BuildSubtree(leaf_provider, level - 1);

                if (rand() % MAX_NODE_SIZE != 0)
                {
                    node->append(subtree.node());
                    cnt += subtree.size();
                }
                else {
                    leaf_provider.rollback(checkpoint);
                }
            }

            return Subtree(node, cnt);
        }
        else {
            return Subtree(leaf_provider.get_leaf(), 1);
        }
    }
    else {
        return Subtree();
    }
}


class ListLeafProvider: public ILeafProvider {
    list<LeafNodePtr> leafs_;
    list<LeafNodePtr>::iterator iterator_;

    Int journal_size_ = 0;
public:
    ListLeafProvider(): iterator_(leafs_.begin()) {}

    virtual Int size() const
    {
        return leafs_.size() - journal_size_;
    }

    virtual LeafNodePtr get_leaf()
    {
        if (iterator_ != leafs_.end())
        {
            auto item = *iterator_;
            iterator_++;
            journal_size_++;
            return item;
        }
        else {
            throw "Leaf List is empty";
        }
    }

    void add(LeafNodePtr leaf)
    {
        leafs_.push_back(leaf);

        journal_size_   = 0;
        iterator_       = leafs_.begin();
    }


    virtual Checkpoint checkpoint() {
        return Checkpoint(iterator_, journal_size_);
    }


    virtual void commit()
    {
        leafs_.erase(leafs_.begin(), iterator_);

        journal_size_   = 0;
        iterator_       = leafs_.begin();
    }

    virtual void rollback(const Checkpoint& checkpoint)
    {
        journal_size_   = checkpoint.journal_size();
        iterator_       = checkpoint.iterator();
    }

    void reset() {
        leafs_.erase(leafs_.begin(), leafs_.end());
        iterator_ = leafs_.begin();
        journal_size_ = 0;
    }
};



LeafNodePtr make_random_leaf(Int size)
{
    LeafNodePtr leaf = make_shared<LeafNode>();

    for (Int c = 0; c < size; c++)
    {
        leaf->append(rand() % 100);
    }

    return leaf;
}


class InsertionState {
    Int inserted_   = 0;
    Int total_      = 0;
public:
    InsertionState(Int total): total_(total) {}

    Int& total() {
        return total_;
    }

    const Int& total() const {
        return total_;
    }

    const Int& inserted() const {
        return inserted_;
    }

    Int& inserted() {
        return inserted_;
    }

    bool shouldMoveUp() const {
        return inserted_ <= total_ / 3;
    }
};



void insert_subtree(TreePtr tree, BranchNodePtr left, BranchNodePtr right, ILeafProvider& provider, InsertionState& state, Int level = 1)
{
    bool first = true;
    while(provider.size() > 0 && left->capacity() > 0)
    {
        auto checkpoint = provider.checkpoint();
        auto subtree = BuildSubtree(provider, level);

        if (first || rand() % MAX_NODE_SIZE != 0)
        {
            left->append(subtree.node());

            state.inserted() += subtree.size();

            first = false;
        }
        else {
            provider.rollback(checkpoint);
            break;
        }
    }


    if (state.shouldMoveUp())
    {
        auto left_parent    = left->parentPtr(tree);
        auto right_parent   = right->parentPtr(tree);

        if (left_parent == right_parent)
        {
            right_parent = left_parent->split(right_parent->find_child_idx(right.get()), tree);
        }

        insert_subtree(tree, left_parent, right_parent, provider, state, level + 1);
    }
    else {
        Int idx = 0;
        first = true;
        while(provider.size() > 0 && right->capacity() > 0)
        {
            auto checkpoint = provider.checkpoint();
            auto subtree = BuildSubtree(provider, level);

            if (first || rand() % MAX_NODE_SIZE != 0)
            {
                right->insert(subtree.node(), idx);

                state.inserted() += subtree.size();
                first = true;
            }
            else {
                provider.rollback(checkpoint);
                break;
            }

            idx++;
        }
    }
}


void insert_subtree(TreePtr tree, BranchNodePtr node, Int pos, ILeafProvider& provider)
{
    auto size = provider.size();

    if (node->capacity() >= size)
    {
        for (Int c = 0; c < size; c++) {
            node->insert(provider.get_leaf(), pos + c);
        }
    }
    else {
        auto next = node->split(pos, tree);

        if (node->capacity() + next->capacity() >= size)
        {
            auto node_capacity = node->capacity();

            for (int c = 0; c < node_capacity && provider.size() > 0; c++)
            {
                node->append(provider.get_leaf());
            }

            auto next_size = provider.size();

            for (int c = 0; c < next_size; c++)
            {
                next->insert(provider.get_leaf(), c);
            }
        }
        else {
            InsertionState state(provider.size());
            auto next_size0 = next->size();
            insert_subtree(tree, node, next, provider, state);

            if (provider.size() > 0)
            {
                cout<<"Insert Next batch of size: "<<provider.size()<<endl;

                auto idx = next->size() - next_size0;

                insert_subtree(tree, next, idx, provider);
            }
        }
    }
}



using ListLeafProviderPtr = shared_ptr<ListLeafProvider>;

template <typename LeafFn>
ListLeafProviderPtr createRandomList(Int size, LeafFn fn)
{
    ListLeafProviderPtr provider = make_shared<ListLeafProvider>();

    for (Int c = 0; c < size; c++)
    {
        auto leaf = make_random_leaf(rand() % (MAX_NODE_SIZE - 1) + 1);
        fn(leaf);
        provider->add(leaf);
    }

    return provider;
}

bool checkTree(TreePtr tree, const DataList& data) {
    Int start = 0;
    if (tree->get_root()->checkTree(data, start))
    {
        if (start != data.size())
        {
            cout<<"Tree Size mismatch: "<<start<<" "<<data.size()<<endl;
            return false;
        }

        return true;
    }
    else {
        return false;
    }
}

int main()
{
    try {
        DataList data1;

        rand();
        rand();

        ListLeafProviderPtr provider1 = createRandomList(3, [&](LeafNodePtr leaf){
            data1.emplace_back(leaf->values());
        });

        auto root = BuildSubtree(*provider1.get(), 4);

        TreePtr tree = make_shared<::Tree>(root.node());

        cout<<"Subtree Size:  "<<root.size()<<endl;
        cout<<"Provider Size: "<<provider1->size()<<endl;

        DataList data2;

        ListLeafProviderPtr provider2 = createRandomList(100123, [&](LeafNodePtr leaf){
            data2.emplace_back(leaf->values());
        });

        data1.insert(data1.begin() + 2, data2.begin(), data2.end());

        auto target_leaf = tree->find_leaf(2);

        if (target_leaf != nullptr)
        {
            if (target_leaf->parent() != nullptr)
            {
                auto target_node = target_leaf->parentPtr(tree);

                Int idx = target_node->find_child_idx(target_leaf.get());

                insert_subtree(tree, dynamic_pointer_cast<BranchNode>(target_node), idx, *provider2.get());

                cout<<"Provider Size: "<<provider1->size()<<endl;
                cout<<"Tree Leafs: "<<tree->get_root()->count_leafs()<<endl;

                checkTree(tree, data1);
            }
            else {
                cout<<"Single leaf tree"<<endl;
            }
        }
        else {
            cout<<"Target leaf not found"<<endl;
        }
    }
    catch (const char* msg)
    {
        cout<<"Exception: "<<msg<<endl;
    }
    return 0;
}
