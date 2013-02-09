
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



template <typename Key, bool Forward = true>
class CountWalker {
public:
    Key sum_;
    Key target_;
    Int key_num_;

    Key distance_;

public:
    CountWalker(Key target, Key distance, Int key_num = 0):
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




}

#endif
