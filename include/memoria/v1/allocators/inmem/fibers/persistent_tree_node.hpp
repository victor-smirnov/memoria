
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>

#include <dumbo/v1/tools/recursive_mutex.hpp>
#include <dumbo/v1/tools/aio_string.hpp>
#include <dumbo/v1/tools/aio_uuid.hpp>

#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/md5.hpp>
#include <memoria/v1/core/tools/assert.hpp>


#include <boost/range.hpp>

#include <atomic>
#include <mutex>

namespace dumbo {
namespace v1 {
namespace inmem {

using namespace memoria::v1;

enum class NodeType {LEAF, BRANCH};

template <typename Key_, Int NodeSize, Int NodeIndexSize, typename NodeId_, typename TxnId_>
class NodeBase {
public:

    using TypedAsyncOutputStreamPtr = ::shared_ptr<TypedAsyncOutputStream>;
    using TypedAsyncInputStreamPtr 	= ::shared_ptr<TypedAsyncInputStream>;


	using MutexT = RecursiveMutex;
	using LockGuardT = std::lock_guard<MutexT>;

    using Key       = Key_;
    using NodeId    = NodeId_;
    using TxnId     = TxnId_;
    using RCType	= BigInt;

    class RootMetadata {
        BigInt size_ = 0;
    public:
        BigInt& size() {
            return size_;
        }
        const BigInt& size() const {
            return size_;
        }

        void add_size(BigInt delta) {
            size_ += delta;
        }
    };

    template <typename, typename, typename, typename>
    friend class PersistentTree;

    template <typename Profile, typename PageType>
    friend class PersistentInMemAllocatorT;

    template <typename, Int, Int, typename, typename>
    friend class NodeBase;

private:

    RootMetadata metadata_;

    NodeType node_type_;

    NodeId node_id_;
    TxnId  txn_id_;

    Int size_ = 0;

    Int cpu_id_;

protected:
    BigInt refs_;
    mutable MutexT mutex_;

private:

    static constexpr Int Indexes =  NodeSize / NodeIndexSize + ((NodeSize % NodeIndexSize == 0)?  0 : 1);

    Key index_[Indexes];
    Key keys_ [NodeSize];

public:
    NodeBase() {}

    NodeBase(const TxnId& txn_id, const NodeId& node_id, NodeType node_type, Int cpu_id):
        node_type_(node_type),
        node_id_(node_id),
        txn_id_(txn_id),
		refs_(0),
		cpu_id_(cpu_id)
    {}

    MutexT& mutex() {return mutex_;}
    MutexT& mutex() const {return mutex_;}

    Int cpu_id() const {return cpu_id_;}

    void lock() {
    	mutex_.lock();
    }

    void unlock() {
    	mutex_.unlock();
    }

    auto try_lock() {
    	return mutex_.try_lock();
    }

    template <typename Base>
    void populate_as_buffer(const Base* node)
    {
        node_type_  = node->node_type();
        node_id_    = node->node_id();
        txn_id_     = node->txn_id();

        metadata_   = node->metadata();
        size_       = node->size();
        refs_       = node->references();

        for (Int c = 0; c < NodeIndexSize; c++)
        {
            index_[c] = node->index_[c];
        }

        for (Int c = 0; c < size_; c++)
        {
            keys_[c] = node->key(c);
        }
    }

    RootMetadata& metadata() {
        return metadata_;
    }

    const RootMetadata& metadata() const {
        return metadata_;
    }

    const NodeId& node_id() const {
        return node_id_;
    }

    NodeType node_type() const {
        return node_type_;
    }

    bool is_leaf() const {
        return node_type_ == NodeType::LEAF;
    }

    bool is_branch() const {
        return node_type_ == NodeType::BRANCH;
    }

    const TxnId& txn_id() const {
        return txn_id_;
    }

    Int size() const {
        return size_;
    };

    Int max_size() const {
        return NodeSize;
    }

    Int capacity() const {
        return max_size() - size();
    }

    bool should_merge() const {
        return size_ <= max_size() / 2;
    }

    const Key& max_key() const {
        return keys_[size_ - 1];
    }

    const Key& min_key() const {
        return keys_[0];
    }

    bool is_full() const {
        return size_ == max_size();
    }

    bool has_space() const {
        return size_ < max_size();
    }

    RCType references() const {
    	LockGuardT lk(mutex_);
        return refs_;
    }

    void ref()
    {
    	LockGuardT lk(mutex_);
    	++refs_;
    }

    RCType unref()
    {
    	LockGuardT lk(mutex_);
        auto r = --refs_;
        MEMORIA_V1_ASSERT(r, >=, 0);
        return r;
    }

    const Key& key(Int idx) const {
        return keys_[idx];
    }

    Key& key(Int idx) {
        return keys_[idx];
    }

    Int find_key(const Key& key) const
    {
        Int last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);

        Int idx = -1;
        for (Int c = 0; c < last_idx; c++)
        {
            if (key <= index_[c])
            {
                idx = c;
                break;
            }
        }

        if (idx >= 0)
        {
            idx *= NodeIndexSize;

            for (Int c = idx; c < idx + NodeIndexSize; c++)
            {
                if (key <= keys_[c])
                {
                    return c;
                }
            }
        }

        return size_;
    }

    Int find_key2(const Key& key) const
    {
        Int last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);

        Int idx = 0;
        for (Int c = 0; c < last_idx; c++)
        {
            idx += (key > index_[c]);
        }

        if (idx < last_idx)
        {
            idx *= NodeIndexSize;

            Int max = (idx + NodeIndexSize) < size_ ? idx + NodeIndexSize : size_;

            for (Int c = idx; c < max; c++)
            {
                idx += (key > keys_[c]);
            }

            return idx;
        }

        return size_;
    }


    Int find_key3(const Key& key) const
    {
        return std::distance(keys_, std::lower_bound(keys_, keys_ + size_, key));
    }

    void reindex()
    {
        for (auto& k: index_) k = Key();

        Int max = (size_ % NodeIndexSize == 0) ? size_ : (size_ - size_ % NodeIndexSize);

        for (Int idx = 0; idx < max; idx += NodeIndexSize)
        {
            index_[idx / NodeIndexSize] = keys_[idx + NodeIndexSize - 1];
        }

        Int last_element = size_ - 1;

        index_[last_element / NodeIndexSize] = keys_[last_element];
    }

    void insert_key(Int idx, const Key& key)
    {
        shift_right(keys_, idx, idx + 1, size_ - idx);
        size_ ++;

        keys_[idx] = key;

        reindex();
    }

    void remove_keys(Int start, Int end)
    {
        shift_left(keys_, end, start, size_ - end);

        for (Int c = size() - (end - start); c < size(); c++) {
            keys_[c] = Key();
        }

        size_ -= (end - start);
        reindex();
    }

    void split_keys_to(Int start, NodeBase* other)
    {
        copy_to(keys_, start, other->keys_, 0, size_ - start);

        for (Int c = start; c < size(); c++)
        {
            keys_[c] = Key();
        }

        other->size_ += size_ - start;
        this->size_  -= size_ - start;

        this->reindex();
        other->reindex();
    }

    void merge_keys_from(NodeBase* other)
    {
        copy_to(other->keys_, 0, keys_, size_, other->size_);

        this->size_  += other->size_;
        other->size_ = 0;

        this->reindex();
        other->reindex();
    }

    bool can_merge_with(const NodeBase* other) const {
        return size_ + other->size_ <= NodeSize;
    }

    void dump(std::ostream& out) const
    {
        out<<"NodeType: "<<node_type()<<endl;
        out<<"Size: "<<size_<<endl;
        out<<"NodeId: "<<node_id_<<endl;
        out<<"TxnId: "<<txn_id_<<endl;
        out<<"Refs: "<<refs_<<endl;
        out<<"cpuId: "<<cpu_id_<<endl;

        out<<"Index: "<<endl;

        Int last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);

        for (Int c = 0; c < last_idx; c++)
        {
            out<<c<<": "<<index_[c]<<endl;
        }
    }

    future<> write(const TypedAsyncOutputStreamPtr& out) const
    {
        return ready(
        out << this->metadata().size()
        	<< (Int)this->node_type()
			<< this->node_id()
			<< this->txn_id()
			<< this->size()
			<< this->references()
        ).then([=] {
        	Int last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);
        	return do_with(boost::irange(0, last_idx, 1), [=](auto range){
        		return do_for_each(range, [=](auto idx){
        			return ready(out << index_[idx]);
        		});
        	});
        }).then([=] {
        	return do_with(boost::irange(0, size_, 1), [=](auto range){
        		return do_for_each(range, [=](auto idx){
        			return ready(out << keys_[idx]);
        		});
        	});
        });
    }

    future<> read(TypedAsyncInputStreamPtr in)
    {
    	Int node_type;
        ready (
        in >> this->metadata().size()
           >> node_type
		   >> node_id_
		   >> txn_id_
           >> size_
		   >> refs_)
		.then([=, &node_type](){
        	 this->node_type_ = (NodeType)node_type;

        	 Int last_idx = size_ / NodeIndexSize + (size_ % NodeIndexSize == 0 ? 0 : 1);
        	 return do_with(boost::irange(0, last_idx, 1), [=](auto& range){
        		 return do_for_each(range, [=](auto idx){
        			 return ready(in >> index_[idx]);
        		 });
        	 });
        }).then([=]() {
        	return do_with(boost::irange(0, size_, 1), [=](auto& range){
        		return do_for_each(range, [=](auto idx){
        			return ready(in >> keys_[idx]);
        		});
        	});
        }).get();

        return ::now();
    }


protected:
    template <typename T>
    void shift_right(T* data, Int from, Int to, Int size) const
    {
        CopyBuffer(data + from, data + to, size);
    }

    template <typename T>
    void shift_left(T* data, Int from, Int to, Int size) const
    {
        CopyBuffer(data + from, data + to, size);
    }

    template <typename T>
    void copy_to(const T* src, Int from, T* dst, Int to, Int size) const
    {
        CopyBuffer(src + from, dst + to, size);
    }

    void set_txn_id(const TxnId& txn_id) {
        this->txn_id_ = txn_id;
    }

    void set_node_id(const NodeId& id) {
        this->node_id_ = id;
    }

    void clear_refs() {
        refs_ = 0;
    }

    void set_refs(Int refs) {
        this->refs_ = refs;
    }

    void hash(MD5Hash& md5) const
    {
        md5.add((UInt)node_type_);
        md5.add_ubi(metadata_.size());
        md5.add_ubi(node_id_);
        md5.add_ubi(txn_id_);
        md5.add_ubi(size_);

        for (Int c = 0; c < size_; c++) {
            md5.add_ubi(keys_[c]);
        }
    }
};


template <typename Key, typename Data, Int NodeSize, Int NodeIndexSize, typename NodeId, typename TxnId>
class Node: public NodeBase<Key, NodeSize, NodeIndexSize, NodeId, TxnId> {
    using Base = NodeBase<Key, NodeSize, NodeIndexSize, NodeId, TxnId>;

protected:
    Data data_[NodeSize];

public:
    using typename Base::TypedAsyncOutputStreamPtr;
    using typename Base::TypedAsyncInputStreamPtr;

    using MyType    = Node<Key, Data, NodeSize, NodeIndexSize, NodeId, TxnId>;
    using NodeBaseT = Base;

    Node() {}

    Node(const TxnId& txn_id, const NodeId& node_id, NodeType node_type, Int cpu_id):
        Base(txn_id, node_id, node_type, cpu_id)
    {}

    const Data& data(Int idx) const {
        return data_[idx];
    }

    Data& data(Int idx) {
        return data_[idx];
    }

    void insert(Int idx, const Key& key, const Data& child)
    {
        this->shift_right(data_, idx, idx + 1, this->size() - idx);

        data_[idx] = child;

        this->insert_key(idx, key);
    }

    void remove(Int idx) {
        remove(idx, idx + 1);
    }

    void remove(Int start, Int end)
    {
        auto size = this->size();

        this->shift_left(data_, end, start, size - end);

        for (Int c = size - (end - start); c < size; c++) {
            data_[c] = Data();
        }

        this->remove_keys(start, end);
    }

    void split_to(Int start, MyType* other)
    {
        this->copy_to(data_, start, other->data_, 0, this->size() - start);

        for (Int c = start; c < this->size(); c++)
        {
            data_[c] = Data();
        }

        this->split_keys_to(start, other);
    }

    void merge_from(MyType* other)
    {
        this->copy_to(other->data_, 0, data_, this->size(), other->size());
        this->merge_keys_from(other);
    }

    UBigInt hash() const
    {
        MD5Hash md5;

        md5.add_ubi((UBigInt)this);

        Base::hash(md5);

        for (Int c = 0; c < this->size(); c++) {
            md5.add_ubi((UBigInt)this->data(c));
        }

        return md5.result().hash64();
    }

    future<> write(const TypedAsyncOutputStreamPtr& out) const
    {
        return Base::write(out).then([=](){
        	return do_with(boost::irange(0, this->size(), 1), [=](auto& range){
        		return do_for_each(range, [=](auto idx){
        			return ready(out << data_[idx]);
        		});
        	});
        });
    }

    future<> read(TypedAsyncInputStreamPtr in)
    {
        return Base::read(in).then([=](){
        	return do_with(boost::irange(0, this->size(), 1), [=](auto& range){
        		return do_for_each(range, [=](auto idx){
        			return ready(in >> data_[idx]);
        		});
        	});
        });
    }
};


template <
    typename Key,
    Int NodeSize,
    Int NodeIndexSize,
    typename NodeId,
    typename TxnId,
    typename ChildPtrType = NodeBase<Key, NodeSize, NodeIndexSize, NodeId, TxnId>*
    >
class BranchNode: public Node<Key, ChildPtrType, NodeSize, NodeIndexSize, NodeId, TxnId> {
    using Base = Node<Key, ChildPtrType, NodeSize, NodeIndexSize, NodeId, TxnId>;

public:
    using NodeBaseT = NodeBase<Key, NodeSize, NodeIndexSize, NodeId, TxnId>;

    BranchNode() {}

    BranchNode(const TxnId& txn_id, const NodeId& node_id, Int cpu_id):
        Base(txn_id, node_id, NodeType::BRANCH, cpu_id)
    {}

    ~BranchNode() {}


    void del(Int owner_cpu_id) const
    {
    	Int cpu_id = this->cpu_id();
    	if (cpu_id == owner_cpu_id) {
    		delete this;
    	}
    	else {
    		delete_on(cpu_id, this);
    	}
    }

    NodeBaseT* find_child(const Key& key) const
    {
        Int idx = Base::find_key(key);

        if (idx == this->size())
        {
            idx = this->size() - 1;
        }

        return this->data_[idx];
    }

    Int find_child_node(const NodeBaseT* child) const
    {
        for (Int c = 0; c < this->size(); c++)
        {
            if (this->data_[c] == child)
            {
                return c;
            }
        }

        return -1;
    }

    NodeBaseT* first_child() {
        return this->data(0);
    }

    const NodeBaseT* first_child() const {
        return this->data(0);
    }

    NodeBaseT* last_child() {
        return this->data(this->size() - 1);
    }

    const NodeBaseT* last_child() const {
        return this->data(this->size() - 1);
    }

    void dump(std::ostream& out = std::cout) const
    {
        Base::dump(out);

        out<<"Data: "<<endl;

        for (Int c = 0; c < this->size(); c++)
        {
            auto node = this->data(c);
            out<<c<<": "<<this->key(c)<<" = "<<node<<" ("<<node->node_id()<<", "<<node->txn_id()<<")"<<endl;
        }

        out<<endl;
    }


};

template <typename Key, typename Value_, Int NodeSize, Int NodeIndexSize, typename NodeId, typename TxnId>
class LeafNode: public Node<Key, Value_, NodeSize, NodeIndexSize, NodeId, TxnId> {
    using Base = Node<Key, Value_, NodeSize, NodeIndexSize, NodeId, TxnId>;

public:
    using Value = Value_;

    LeafNode() {}
    LeafNode(const TxnId& txn_id, const NodeId& node_id, Int cpu_id):
        Base(txn_id, node_id, NodeType::LEAF, cpu_id)
    {}


    ~LeafNode() {}


    void del(Int owner_cpu_id) const
    {
    	Int cpu_id = this->cpu_id();
    	if (cpu_id == owner_cpu_id) {
    		delete this;
    	}
    	else {
    		delete_on(cpu_id, this);
    	}
    }

    Int find(const Key& key) const
    {
        return this->find_key(key);
    }

    void dump(std::ostream& out = std::cout) const
    {
        Base::dump(out);

        out<<"Data: "<<endl;

        for (Int c = 0; c < this->size(); c++)
        {
            out<<c<<": "<<this->key(c)<<" = "<<this->data(c)<<endl;
        }

        out<<endl;
    }
};


static std::ostream& operator<<(std::ostream& out, NodeType node_type) {
    if (node_type == NodeType::LEAF) {
        out<<"LEAF";
    }
    else if (node_type == NodeType::BRANCH)
    {
        out<<"BRANCH";
    }
    else {
        out<<"UNKNOWN["<<(int)node_type<<"]";
    }

    return out;
}

}
}}
