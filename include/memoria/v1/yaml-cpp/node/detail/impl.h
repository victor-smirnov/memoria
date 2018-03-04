
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

#include <memoria/v1/yaml-cpp/node/detail/node.h>
#include <memoria/v1/yaml-cpp/node/detail/node_data.h>
#include <type_traits>

namespace memoria {
namespace v1 {
namespace YAML {
namespace detail {
template <typename Key, typename Enable = void>
struct get_idx {
  static node* get(const std::vector<node*>& /* sequence */,
                   const Key& /* key */, shared_memory_holder /* pMemory */) {
    return 0;
  }
};

template <typename Key>
struct get_idx<Key,
               typename std::enable_if<std::is_unsigned<Key>::value &&
                                       !std::is_same<Key, bool>::value>::type> {
  static node* get(const std::vector<node*>& sequence, const Key& key,
                   shared_memory_holder /* pMemory */) {
    return key < sequence.size() ? sequence[key] : 0;
  }

  static node* get(std::vector<node*>& sequence, const Key& key,
                   shared_memory_holder pMemory) {
   if (key > sequence.size() || (key > 0 && !sequence[key-1]->is_defined()))
      return 0;
    if (key == sequence.size())
      sequence.push_back(&pMemory->create_node());
    return sequence[key];
  }
};

template <typename Key>
struct get_idx<Key, typename std::enable_if<std::is_signed<Key>::value>::type> {
  static node* get(const std::vector<node*>& sequence, const Key& key,
                   shared_memory_holder pMemory) {
    return key >= 0 ? get_idx<std::size_t>::get(
                          sequence, static_cast<std::size_t>(key), pMemory)
                    : 0;
  }
  static node* get(std::vector<node*>& sequence, const Key& key,
                   shared_memory_holder pMemory) {
    return key >= 0 ? get_idx<std::size_t>::get(
                          sequence, static_cast<std::size_t>(key), pMemory)
                    : 0;
  }
};

template <typename T>
inline bool node::equals(const T& rhs, shared_memory_holder pMemory) {
  T lhs;
  if (convert<T>::decode(Node(*this, pMemory), lhs)) {
    return lhs == rhs;
  }
  return false;
}

inline bool node::equals(const char* rhs, shared_memory_holder pMemory) {
  return equals<std::string>(rhs, pMemory);
}

// indexing
template <typename Key>
inline node* node_data::get(const Key& key,
                            shared_memory_holder pMemory) const {
  switch (m_type) {
    case NodeType::Map:
      break;
    case NodeType::Undefined:
    case NodeType::Null:
      return NULL;
    case NodeType::Sequence:
      if (node* pNode = get_idx<Key>::get(m_sequence, key, pMemory))
        return pNode;
      return NULL;
    case NodeType::Scalar:
      throw BadSubscript();
  }

  for (node_map::const_iterator it = m_map.begin(); it != m_map.end(); ++it) {
    if (it->first->equals(key, pMemory)) {
      return it->second;
    }
  }

  return NULL;
}

template <typename Key>
inline node& node_data::get(const Key& key, shared_memory_holder pMemory) {
  switch (m_type) {
    case NodeType::Map:
      break;
    case NodeType::Undefined:
    case NodeType::Null:
    case NodeType::Sequence:
      if (node* pNode = get_idx<Key>::get(m_sequence, key, pMemory)) {
        m_type = NodeType::Sequence;
        return *pNode;
      }

      convert_to_map(pMemory);
      break;
    case NodeType::Scalar:
      throw BadSubscript();
  }

  for (node_map::const_iterator it = m_map.begin(); it != m_map.end(); ++it) {
    if (it->first->equals(key, pMemory)) {
      return *it->second;
    }
  }

  node& k = convert_to_node(key, pMemory);
  node& v = pMemory->create_node();
  insert_map_pair(k, v);
  return v;
}

template <typename Key>
inline bool node_data::remove(const Key& key, shared_memory_holder pMemory) {
  if (m_type != NodeType::Map)
    return false;

  for (kv_pairs::iterator it = m_undefinedPairs.begin();
       it != m_undefinedPairs.end();) {
    kv_pairs::iterator jt = std::next(it);
    if (it->first->equals(key, pMemory))
      m_undefinedPairs.erase(it);
    it = jt;
  }

  for (node_map::iterator it = m_map.begin(); it != m_map.end(); ++it) {
    if (it->first->equals(key, pMemory)) {
      m_map.erase(it);
      return true;
    }
  }

  return false;
}

// map
template <typename Key, typename Value>
inline void node_data::force_insert(const Key& key, const Value& value,
                                    shared_memory_holder pMemory) {
  switch (m_type) {
    case NodeType::Map:
      break;
    case NodeType::Undefined:
    case NodeType::Null:
    case NodeType::Sequence:
      convert_to_map(pMemory);
      break;
    case NodeType::Scalar:
      throw BadInsert();
  }

  node& k = convert_to_node(key, pMemory);
  node& v = convert_to_node(value, pMemory);
  insert_map_pair(k, v);
}

template <typename T>
inline node& node_data::convert_to_node(const T& rhs,
                                        shared_memory_holder pMemory) {
  Node value = convert<T>::encode(rhs);
  value.EnsureNodeExists();
  pMemory->merge(*value.m_pMemory);
  return *value.m_pNode;
}
}
}}}

