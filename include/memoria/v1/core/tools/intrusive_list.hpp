
// Copyright 2013 Victor Smirnov
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

#include <cstddef>

namespace memoria {
namespace v1 {



template <typename Node>
class IntrusiveList {
    Node* head_ = nullptr;
    Node* tail_ = nullptr;

    std::size_t size_ = 0;

    class IteratorBase {
    protected:
        Node* node_;
    public:
        IteratorBase(Node* node) noexcept : node_(node) {}

        Node& operator*() noexcept {
            return *node_;
        }

        const Node& operator*() const noexcept {
            return *node_;
        }

        Node* operator->() noexcept {
            return node_;
        }

        const Node* operator->() const noexcept {
            return node_;
        }

        Node* node() noexcept {
            return node_;
        }

        const Node* node() const noexcept {
            return node_;
        }

        bool operator==(const IteratorBase& other) const noexcept {
            return node_ == other.node_;
        }

        bool operator!=(const IteratorBase& other) const noexcept {
            return node_ != other.node_;
        }
    };

    class ConstIteratorBase {
    protected:
        const Node* node_;
    public:
        ConstIteratorBase(const Node* node) noexcept : node_(node) {}

        const Node& operator*() const {
            return *node_;
        }

        const Node* operator->() const {
            return node_;
        }

        const Node* node() const {
            return node_;
        }

        bool operator==(const ConstIteratorBase& other) const {
            return node_ == other.node_;
        }

        bool operator!=(const ConstIteratorBase& other) const {
            return node_ != other.node_;
        }
    };


    class ForwardListIterator: public IteratorBase  {
    public:
        ForwardListIterator(Node* node) noexcept : IteratorBase(node) {}

        ForwardListIterator& operator++() noexcept {
            this->node_ = this->node_->next();
            return *this;
        }

        void operator--() noexcept {
            this->node_ = this->node_->prev();
        }
    };

    class BackwardListIterator: public IteratorBase {
    public:
        BackwardListIterator(Node* node) noexcept : IteratorBase(node) {}

        void operator++() noexcept {
            this->node_ = this->node_->prev();
        }

        void operator--() noexcept {
            this->node_ = this->node_->next();
        }
    };


    class ForwardListConstIterator: public ConstIteratorBase  {
    public:
        ForwardListConstIterator(Node* node) noexcept : ConstIteratorBase(node) {}

        void operator++() noexcept {
            this->node_ = this->node_->next();
        }

        void operator--() noexcept {
            this->node_ = this->node_->prev();
        }
    };


    class BackwardListConstIterator: public ConstIteratorBase {
    public:
        BackwardListConstIterator(Node* node) noexcept : ConstIteratorBase(node) {}

        void operator++() noexcept {
            this->node_ = this->node_->prev();
        }

        void operator--() noexcept {
            this->node_ = this->node_->next();
        }
    };



public:

    typedef Node*                       value_type;
    typedef std::size_t                 size_type;
    typedef std::ptrdiff_t              difference_type;
    typedef value_type&                 reference;
    typedef const value_type&           const_reference;
    typedef Node*                       pointer;
    typedef const Node*                 const_pointer;

    typedef ForwardListIterator         iterator;
    typedef ForwardListConstIterator    const_iterator;

    typedef BackwardListIterator        reverse_iterator;
    typedef BackwardListConstIterator   const_reverse_iterator;

private:

public:
    IntrusiveList() noexcept {}

    reference front() noexcept {
        return *head_;
    }

    const_reference front() const noexcept {
        return *head_;
    }

    reference back() noexcept {
        return *tail_;
    }

    const_reference back() const noexcept {
        return *tail_;
    }

    iterator begin() noexcept {
        return iterator(head_);
    }

    const_iterator begin() const noexcept {
        return const_iterator(head_);
    }

    const_iterator cbegin() const noexcept {
        return const_iterator(head_);
    }

    iterator rbegin() noexcept {
        return iterator(tail_);
    }

    const_iterator crbegin() const noexcept {
        return const_iterator(tail_);
    }

    iterator end() noexcept {
        return iterator(nullptr);
    }

    const_iterator end() const noexcept {
        return const_iterator(nullptr);
    }

    const_iterator cend() noexcept {
        return const_iterator(nullptr);
    }

    iterator rend() noexcept {
        return iterator(nullptr);
    }

    const_iterator crend() const noexcept {
        return const_iterator(nullptr);
    }

    size_type size() const noexcept {
        return size_;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    void insert(iterator pos, Node* node) noexcept
    {
        if (pos != end())
        {
            insert(pos.node(), node);
        }
        else if (tail_)
        {
            insertAfter(tail_, node);
        }
        else {
            head_           = node;
            tail_           = node;
            node->next()    = node->prev() = nullptr;
        }

        size_++;
    }

    Node* takeTop() noexcept {
        Node* top = head_;

        erase(top);

        return top;
    }

    void erase(iterator iter) noexcept
    {
        erase(iter.node());
    }

    void erase(Node* node) noexcept
    {
        if (node->prev())
        {
            node->prev()->next() = node->next();
        }

        if (node->next())
        {
            node->next()->prev() = node->prev();
        }

        if (head_ == node)
        {
            head_ = node->next();
        }

        if (tail_ == node)
        {
            tail_ = node->prev();
        }

        node->next() = node->prev() = nullptr;

        size_--;
    }

    void clear() noexcept
    {
        head_ = tail_ = nullptr;
    }

private:
    void insert(Node* pos, Node* node) noexcept
    {
        node->prev() = pos->prev();
        node->next() = pos;

        if (pos->prev())
        {
            pos->prev()->next() = node;
        }

        pos->prev() = node;

        if (head_ == pos)
        {
            head_ = node;
        }
    }

    void insertAfter(Node* pos, Node* node) noexcept
    {
        node->prev() = pos;
        node->next() = pos->next();

        if (pos->next())
        {
            pos->next()->prev() = node;
        }

        pos->next() = node;

        if (tail_ == pos)
        {
            tail_ = node;
        }
    }
};


}}