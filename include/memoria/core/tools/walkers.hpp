
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CORE_TOOLS_WALKERS_HPP_
#define MEMORIA_CORE_TOOLS_WALKERS_HPP_

namespace memoria {

template <typename Container, typename Key = typename Container::KeyType, bool Forward = true>
class NodeTreeWalker {
public:
	Key sum_;
	Key target_;
	Int key_num_;

	Container& me_;

public:
	NodeTreeWalker(Key target, Container& me, Int key_num = 0):sum_(0), target_(target), key_num_(key_num), me_(me) {}

	Key remainder() const
	{
		return target_ - sum_;
	}

	Key sum() const
	{
		return sum_;
	}

	template <typename Node>
	Int operator()(Node *node, Int idx)
	{
		if (Forward)
		{
			for (Int c = idx; c < node->children_count(); c++)
			{
				Key key = node->map().key(key_num_, c);
				if (key + sum_ <= target_)
				{
					sum_ = sum_ + key;
				}
				else {
					return c;
				}
			}
		}
		else
		{
			for (Int c = idx; c >= 0; c--)
			{
				Key key = node->map().key(key_num_, c);
				if (key + sum_ < target_)
				{
					sum_ = sum_ + key;
				}
				else {
					return c;
				}
			}
		}
		return -1;
	}
};





template <typename Container, bool Forward = true, typename CountType = BigInt>
class KeyCounterWalker {

	typedef typename Container::ID 			ID;
	typedef typename Container::NodeBase 	NodeBase;
	typedef typename Container::NodeBaseG 	NodeBaseG;

public:
	CountType sum_;
	CountType target_;

	Container& me_;

public:
	KeyCounterWalker(CountType target, Container& me):sum_(0), target_(target), me_(me) {}

	CountType remainder() const
	{
		return target_ - sum_;
	}

	CountType sum() const
	{
		return sum_;
	}

	template <typename Node>
	Int operator()(Node *node, Int idx)
	{
		if (Forward)
		{
			if (node->is_leaf())
			{
				for (Int c = idx; c < node->children_count(); c++)
				{
					CountType count = 1;
					if (count + sum_ <= target_)
					{
						sum_ = sum_ + count;
					}
					else {
						return c;
					}
				}
			}
			else {
				for (Int c = idx; c < node->children_count(); c++)
				{
					NodeBaseG child = me_.allocator().getPage(node->map().data(c), Container::Allocator::READ);

					CountType count = 1;//child->counters().key_count();
					if (count + sum_ <= target_)
					{
						sum_ = sum_ + count;
					}
					else {
						return c;
					}
				}
			}
		}
		else
		{
			if (node->is_leaf())
			{
				for (Int c = idx; c >= 0; c--)
				{
					CountType count = 1;
					if (count + sum_ <= target_)
					{
						sum_ = sum_ + count;
					}
					else {
						return c;
					}
				}
			}
			else {
				for (Int c = idx; c >= 0; c--)
				{
					NodeBaseG child = me_.allocator().getPage(node->map().data(c), Container::Allocator::READ);

					CountType count = 1;//child->counters().key_count();
					if (count + sum_ <= target_)
					{
						sum_ = sum_ + count;
					}
					else {
						return c;
					}
				}
			}
		}
		return -1;
	}
};





template <typename Container, bool Forward = true, typename CountType = BigInt, Int KEYS = 1>
class KeyCounterWithSumWalker {

	typedef typename Container::ID 			ID;
	typedef typename Container::NodeBase 	NodeBase;
	typedef typename Container::NodeBaseG 	NodeBaseG;

public:
	CountType sum_;
	CountType target_;

	CountType keys_[KEYS];

	Container& me_;



public:
	KeyCounterWithSumWalker(CountType target, Container& me):sum_(0), target_(target), me_(me)
	{
		for (CountType& key: keys_) key = 0;
	}

	CountType remainder() const
	{
		return target_ - sum_;
	}

	CountType sum() const
	{
		return sum_;
	}

	CountType keys(Int idx) const
	{
		return keys_[idx];
	}

	template <typename Node>
	Int operator()(Node *node, Int idx)
	{
		if (Forward)
		{
			if (node->is_leaf())
			{
				for (Int c = idx; c < node->children_count(); c++)
				{
					CountType count = 1;
					if (count + sum_ <= target_)
					{
						sum_ = sum_ + count;
						for (Int d = 0; d < KEYS; d++)
						{
							keys_[d] += node->map().key(d, c);
						}
					}
					else {
						return c;
					}
				}
			}
			else {
				for (Int c = idx; c < node->children_count(); c++)
				{
					NodeBaseG child = me_.allocator().getPage(node->map().data(c), Container::Allocator::READ);

					CountType count = 1;//child->counters().key_count();
					if (count + sum_ <= target_)
					{
						sum_ = sum_ + count;
						for (Int d = 0; d < KEYS; d++)
						{
							keys_[d] += node->map().key(d, c);
						}
					}
					else {
						return c;
					}
				}
			}
		}
		else
		{
			if (node->is_leaf())
			{
				for (Int c = idx; c >= 0; c--)
				{
					CountType count = 1;
					if (count + sum_ <= target_)
					{
						sum_ = sum_ + count;
						for (Int d = 0; d < KEYS; d++)
						{
							keys_[d] += node->map().key(d, c);
						}
					}
					else {
						return c;
					}
				}
			}
			else {
				for (Int c = idx; c >= 0; c--)
				{
					NodeBaseG child = me_.allocator().getPage(node->map().data(c), Container::Allocator::READ);

					CountType count = 1;//child->counters().key_count();
					if (count + sum_ <= target_)
					{
						sum_ = sum_ + count;
						for (Int d = 0; d < KEYS; d++)
						{
							keys_[d] += node->map().key(d, c);
						}
					}
					else {
						return c;
					}
				}
			}
		}
		return -1;
	}
};









}

#endif
