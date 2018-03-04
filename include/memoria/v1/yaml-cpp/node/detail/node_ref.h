
// Copyright (c) 2008-2015 Jesse Beder.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <memoria/v1/yaml-cpp/dll.h>
#include <memoria/v1/yaml-cpp/node/type.h>
#include <memoria/v1/yaml-cpp/node/ptr.h>
#include <memoria/v1/yaml-cpp/node/detail/node_data.h>

namespace memoria {
namespace v1 {
namespace YAML {
namespace detail {
class node_ref {
 public:
  node_ref() : m_pData(new node_data) {}
  node_ref(const node_ref&) = delete;
  node_ref& operator=(const node_ref&) = delete;

  bool is_defined() const { return m_pData->is_defined(); }
  const Mark& mark() const { return m_pData->mark(); }
  NodeType::value type() const { return m_pData->type(); }
  const std::string& scalar() const { return m_pData->scalar(); }
  const std::string& tag() const { return m_pData->tag(); }
  EmitterStyle::value style() const { return m_pData->style(); }

  void mark_defined() { m_pData->mark_defined(); }
  void set_data(const node_ref& rhs) { m_pData = rhs.m_pData; }

  void set_mark(const Mark& mark) { m_pData->set_mark(mark); }
  void set_type(NodeType::value type) { m_pData->set_type(type); }
  void set_tag(const std::string& tag) { m_pData->set_tag(tag); }
  void set_null() { m_pData->set_null(); }
  void set_scalar(const std::string& scalar) { m_pData->set_scalar(scalar); }
  void set_style(EmitterStyle::value style) { m_pData->set_style(style); }

  // size/iterator
  std::size_t size() const { return m_pData->size(); }

  const_node_iterator begin() const {
    return static_cast<const node_data&>(*m_pData).begin();
  }
  node_iterator begin() { return m_pData->begin(); }

  const_node_iterator end() const {
    return static_cast<const node_data&>(*m_pData).end();
  }
  node_iterator end() { return m_pData->end(); }

  // sequence
  void push_back(node& node, shared_memory_holder pMemory) {
    m_pData->push_back(node, pMemory);
  }
  void insert(node& key, node& value, shared_memory_holder pMemory) {
    m_pData->insert(key, value, pMemory);
  }

  // indexing
  template <typename Key>
  node* get(const Key& key, shared_memory_holder pMemory) const {
    return static_cast<const node_data&>(*m_pData).get(key, pMemory);
  }
  template <typename Key>
  node& get(const Key& key, shared_memory_holder pMemory) {
    return m_pData->get(key, pMemory);
  }
  template <typename Key>
  bool remove(const Key& key, shared_memory_holder pMemory) {
    return m_pData->remove(key, pMemory);
  }

  node* get(node& key, shared_memory_holder pMemory) const {
    return static_cast<const node_data&>(*m_pData).get(key, pMemory);
  }
  node& get(node& key, shared_memory_holder pMemory) {
    return m_pData->get(key, pMemory);
  }
  bool remove(node& key, shared_memory_holder pMemory) {
    return m_pData->remove(key, pMemory);
  }

  // map
  template <typename Key, typename Value>
  void force_insert(const Key& key, const Value& value,
                    shared_memory_holder pMemory) {
    m_pData->force_insert(key, value, pMemory);
  }

 private:
  shared_node_data m_pData;
};
}
}}}


