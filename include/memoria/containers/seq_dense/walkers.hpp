
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_CONTAINERS_SEQ_DENSE_WALKERS_HPP
#define _MEMORIA_CONTAINERS_SEQ_DENSE_WALKERS_HPP


namespace memoria {

template <typename Key, bool Forward = true>
class SelectWalker {
public:
    Key sum_;
    Key target_;
    Int key_num_;

    Key distance_;

public:
    SelectWalker(Key target, Key distance, Int key_num = 0):
    	sum_(0),
    	target_(target),
    	key_num_(key_num),
    	distance_(distance) {}

    Key remainder() const
    {
        return target_ - sum_;
    }

    Key rank() const
    {
        return sum_;
    }

    Key distance() const
    {
    	return distance_;
    }

    template <typename Node>
    Int operator()(Node *node, Int idx)
    {
        if (Forward)
        {
            Int position = node->map().findSumPositionFwLE(key_num_, idx, target_ - sum_, sum_);

            node->map().sum(0, idx, position, distance_);

            if (position < node->children_count())
            {
            	return position;
            }
            else {
                return -1;
            }
        }
        else
        {
        	Int position = node->map().findSumPositionBwLE(key_num_, idx, target_ - sum_, sum_);

        	node->map().sum(0, position + 1, idx + 1, distance_);

        	return position;
        }
    }
};



template <typename TreeType, Int size_block = 0>
class BSTreeCountFWWalker {

	typedef typename TreeType::Key 			Key;
	typedef typename TreeType::IndexKey 	IndexKey;

	Int 			block_num_;
	IndexKey 		rank_;

	const Key* 		keys_;
	const IndexKey* indexes_;

	const Key* 		size_keys_;
	const IndexKey* size_indexes_;



public:
	BSTreeCountFWWalker(const TreeType& me, Int block_num): block_num_(block_num), rank_(0)
	{
		keys_ 			= me.keys(block_num);
		indexes_ 		= me.indexes(block_num);

		size_keys_ 		= me.keys(size_block);
		size_indexes_ 	= me.indexes(size_block);
	}

	void prepareIndex() {}

	Int walkIndex(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			IndexKey block_rank = indexes_[c];
			IndexKey size 		= size_indexes_[c];

			if (block_rank == size)
			{
				rank_  += block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}


	Int walkKeys(Int start, Int end)
	{
		for (Int c = start; c < end; c++)
		{
			IndexKey block_rank = keys_[c];
			IndexKey size 		= size_keys_[c];

			if (block_rank == size)
			{
				rank_  += block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}

	IndexKey rank() const {
		return rank_;
	}
};




template <typename TreeType, Int size_block = 0>
class BSTreeCountBWWalker {

	typedef typename TreeType::Key 			Key;
	typedef typename TreeType::IndexKey 	IndexKey;

	Int 			block_num_;
	IndexKey 		rank_;

	const Key* 		keys_;
	const IndexKey* indexes_;

	const Key* 		size_keys_;
	const IndexKey* size_indexes_;

public:
	BSTreeCountBWWalker(const TreeType& me, Int block_num): block_num_(block_num), rank_(0)
	{
		keys_ 			= me.keys(block_num);
		indexes_ 		= me.indexes(block_num);

		size_keys_ 		= me.keys(size_block);
		size_indexes_ 	= me.indexes(size_block);
	}

	void prepareIndex() {}

	Int walkIndex(Int start, Int end)
	{
		for (Int c = start; c > end; c--)
		{
			IndexKey block_rank = indexes_[c];
			IndexKey size 		= size_indexes_[c];

			if (block_rank == size)
			{
				rank_  += block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}


	Int walkKeys(Int start, Int end)
	{
		for (Int c = start; c > end; c--)
		{
			IndexKey block_rank = keys_[c];
			IndexKey size 		= size_keys_[c];

			if (block_rank == size)
			{
				rank_  += block_rank;
			}
			else {
				return c;
			}
		}

		return end;
	}

	IndexKey rank() const {
		return rank_;
	}
};


template <typename Key, bool Forward = true>
class CountWalker {
public:
    Key sum_;
    Int key_num_;


public:
    CountWalker(Int key_num):
    	sum_(0),
    	key_num_(key_num)
    {}

    Key sum() const
    {
        return sum_;
    }

    Key rank() const
    {
        return sum_;
    }


    template <typename Node>
    Int operator()(Node *node, Int idx)
    {
        if (Forward)
        {
            BSTreeCountFWWalker<typename Node::Map> walker(node->map(), key_num_);

        	Int position = node->map().walkFw(idx, walker);

        	sum_ += walker.rank();

            if (position < node->children_count())
            {
            	return position;
            }
            else {
                return -1;
            }
        }
        else
        {
        	BSTreeCountBWWalker<typename Node::Map> walker(node->map(), key_num_);

        	Int position = node->map().walkBw(idx, walker);

        	sum_ += walker.rank();

        	return position;
        }
    }
};





}

#endif
