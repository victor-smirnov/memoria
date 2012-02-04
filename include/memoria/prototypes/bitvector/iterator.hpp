
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_BITVECTOR_ITERATOR_HPP
#define _MEMORIA_BITVECTOR_ITERATOR_HPP

template <int Index, typename Key = csize_t, typename IdxType = index_t>
class index_walker {
    Key _sum;
    Key _limit;
public:
    index_walker(const Key &k0, const Key &l0): _sum(k0), _limit(l0) {}
    index_walker(const Key &l0): _sum(0), _limit(l0) {}

    bool operator()(node_base *node, IdxType idx) {
        BV_NODE_CAST2(node, {
            Key tmp = _sum + __node->map().key(Index, idx);
            if (tmp > _limit) {
                return false;
            }
            else {
                _sum = tmp;
                return true;
            }
        });
    }

    Key &sum() const {
        return (Key&) _sum;
    }

    Key &limit() const {
        return (Key&) _limit;
    }
};

template <typename Key = csize_t, typename IdxType = index_t>
class count_bit_index_walker {
    Key _sum;
    bool _inserted;
    bool _hasnt_label;
public:
    count_bit_index_walker(const Key &k0): _sum(k0), _inserted(false), _hasnt_label(false) {}
    count_bit_index_walker(): _sum(0), _inserted(false), _hasnt_label(false) {}

    bool operator()(node_base *node, IdxType idx) {
        BV_NODE_CAST2(node, {
            Key rank = __node->map().key(1, idx);
            Key size = __node->map().key(0, idx);

            if (rank != size) {
                return false;
            }
            else {
                if (!_inserted) {
                    _inserted = __node->inserted_idx() == idx;
                }
                _sum += rank;
                return true;
            }
        });
    }

    Key &sum() const {
        return (Key&) _sum;
    }

    bool &inserted() const {
        return (bool&)_inserted;
    }

    bool &hasnt_label() {
        return (bool&)_hasnt_label;
    }
};


template <typename Key = csize_t, typename IdxType = index_t>
class count_one_walker {
    Key _sum;
    bool _inserted;
    bool _hasnt_label;
public:
    count_one_walker(const Key &k0): _sum(k0), _inserted(false), _hasnt_label(false) {}
    count_one_walker(): _sum(0), _inserted(false), _hasnt_label(false) {}

    bool operator()(node_base *node, IdxType idx) {
        BV_NODE_CAST2(node, {
            Key rank = __node->map().key(1, idx);
            Key size = __node->map().key(0, idx);

            if (rank != size) {
                return false;
            }
            else {
                if (!_inserted) {
                    _inserted = __node->inserted_idx() == idx;
                }
                _sum += rank;
                return true;
            }
        });
    }

    Key &sum() const {
        return (Key&) _sum;
    }

    bool &inserted() const {
        return (bool&)_inserted;
    }

    bool &hasnt_label() {
        return (bool&)_hasnt_label;
    }
};

template <typename Key = csize_t, typename IdxType = index_t>
class count_zero_walker {
    Key _sum;
    bool _inserted;
    bool _hasnt_label;
public:
    count_zero_walker(const Key &k0): _sum(k0), _inserted(false), _hasnt_label(false) {}
    count_zero_walker(): _sum(0), _inserted(false), _hasnt_label(false) {}

    bool operator()(node_base *node, IdxType idx) {
        BV_NODE_CAST2(node, {
            Key size = __node->map().key(0, idx);
            Key rank = __node->map().key(1, idx);
            Key bitsize = __node->map().key(2, idx);

            if (rank != 0 || size != bitsize) {
                return false;
            }
            else {
                if (!_inserted) {
                    _inserted = __node->inserted_idx() == idx;
                }
                _sum += size;
                return true;
            }
        });
    }

    Key &sum() const {
        return (Key&) _sum;
    }

    bool &inserted() const {
        return (bool&)_inserted;
    }

    bool &hasnt_label() {
        return (bool&)_hasnt_label;
    }
};

template <typename Key = csize_t, typename IdxType = index_t>
class counter_one_fw {
    Key _sum;
public:
    counter_one_fw(const Key &k0): _sum(k0) {}
    counter_one_fw(): _sum(0) {}

    bool operator()(bitmap_node *data, IdxType from, IdxType limit) {
        _sum += memoria::count_one_fw(data->bits(), from, limit);
        return true;
    }

    bool operator()(bitmap_node *data, IdxType c) {
        index_t rank = data->header().get_rank(c);

        if (rank == data->get_block_size(c)) {
            _sum += rank;
            return false;
        }
        else {
            _sum += memoria::count_one_fw(data->bits(),
                                         data->header().get_block_start(c),
                                         data->header().get_block_end(c)
                                        );

            return true;
        }
    }

    Key &sum() const {
        return (Key&) _sum;
    }
};

template <typename Key = csize_t, typename IdxType = index_t>
class counter_zero_fw {
    Key _sum;
public:
    counter_zero_fw(const Key &k0): _sum(k0) {}
    counter_zero_fw(): _sum(0) {}

    bool operator()(bitmap_node *data, IdxType from, IdxType limit) {
        _sum += memoria::count_zero_fw(data->bits(), from, limit);
        return true;
    }

    bool operator()(bitmap_node *data, IdxType c) {
        index_t rank = data->header().get_rank(c);
        index_t bitsize = data->header().get_bitsize(c);

        if (rank == 0 && bitsize == data->get_block_size(c)) {
            _sum += rank;
            return false;
        }
        else {
            _sum += memoria::count_zero_fw(data->bits(),
                        data->header().get_block_start(c),
                        data->header().get_block_end(c));
            return true;
        }
    }

    Key &sum() const {
        return (Key&) _sum;
    }
};

template <typename Key = csize_t, typename IdxType = index_t>
class counter_one_bw {
    Key _sum;
public:
    counter_one_bw(const Key &k0): _sum(k0) {}
    counter_one_bw(): _sum(0) {}

    bool operator()(bitmap_node *data, IdxType from, IdxType limit) {
        _sum +=memoria::count_one_bw(data->bits(), from, limit);
        return true;
    }

    bool operator()(bitmap_node *data, IdxType c) {
        index_t rank = data->header().get_rank(c);

        if (rank == data->get_block_size(c)) {
            _sum += rank;
            return false;
        }
        else {
            _sum += memoria::count_one_bw(data->bits(),
                        data->header().get_block_end(c),
                        data->header().get_block_start(c));

            return true;
        }
    }

    Key &sum() const {
        return (Key&) _sum;
    }
};

template <typename Key = csize_t, typename IdxType = index_t>
class counter_zero_bw {
    Key _sum;
public:
    counter_zero_bw(const Key &k0): _sum(k0) {}
    counter_zero_bw(): _sum(0) {}

    Key operator()(bitmap_node *data, IdxType from, IdxType limit) {
        return memoria::count_one_fw(data->bits(), from, limit);
    }

    bool operator()(bitmap_node *data, IdxType c) {
        index_t rank = data->header().get_rank(c);
        index_t bitsize = data->header().get_bitsize(c);

        if (rank == 0 && bitsize == data->get_block_size(c)) {
            _sum += rank;
            return false;
        }
        else {
            _sum += memoria::count_zero_fw(data->bits(),
            data->header().get_block_end(c),
            data->header().get_block_start(c));

            return true;
        }
    }

    Key &sum() const {
        return (Key&) _sum;
    }
};


template <typename IdxType>
class iterator {

    static const int END   = 1;
    static const int START    = 2;
    static const int EMPTY  = 4;
    static const int START  = 8;

    Allocator     &_allocator;
    txn_t       &_txn;

    int state;
    bitmap_node *_data;
    node_base   *_page;
    IdxType      _idx;

    static const char STEP_UP        = 0;
    static const char STEP_DOWN      = 1;
    static const char STEP_BREAK     = 2;

public:
    iterator(Allocator &allocator, txn_t &txn): _allocator(allocator), _txn(txn) {
        state = 0;
        setEmpty(true);
        _data = NULL;
        _page = NULL;
        _idx = 0;
    }

    iterator(Allocator &allocator, txn_t &txn, bitmap_node *node, IdxType idx): _allocator(allocator), _txn(txn) {
        state = 0;
        _page = get_parent(node);
        _data = node;
        _idx  = idx;
    }

    const iterator<IdxType> operator=(const iterator<IdxType> &other) {
        state = 0;
        _allocator = other._allocator;
        _txn = other._txn;
        _data = other._data;
        _page = other._page;
        _idx = other._idx;
        return *this;
    }

    void resetState() {
        state = 0;
    }

    bool isEof() {
        return (state & END) != 0;
    }

    void setStateBit(int flag, bool bit) {
        if (bit) {
            state |= flag;
        }
        else {
            state &= ~flag;
        }
    }

    void setEof(bool eof) {
        setStateBit(END, eof);
    }

    bool isBof() {
        return (state & START) != 0;
    }

    void setBof(bool bof) {
        setStateBit(START, bof);
    }

    bool isEmpty() {
        return (state & EMPTY) != 0;
    }

    void setEmpty(bool empty) {
        setStateBit(EMPTY, empty);
    }

    bool isStart() {
        return (state & START) != 0;
    }

    void setStart(bool start) {
        setStateBit(START, start);
    }

    Allocator &allocator() {
        return _allocator;
    }

    txn_t &txn() {
        return _txn;
    }

    IdxType &idx() {
        return _idx;
    }

    bitmap_node *&data() {
        return _data;
    }

    node_base *&page() {
        return _page;
    }

    csize_t current_bitmap_index() {
        
    }

    bool next_bitmap() {
        index_t count = get_children_count(_page);
        if (_data->parent_idx() < count - 1) {
            _data = get_bitmap(_page, _data->parent_idx() + 1);
            idx() = 0;
        }
        else {
            node_base *node = get_next_leaf(_page);
            if (node != NULL) {
                _page = node;
                _data = get_bitmap(_page, 0);
                idx() = 0;
            }
            else {
                return false;
            }
        }
        return true;
    }

    bool prev_bitmap() {
        if (_data->parent_idx() > 0) {
            _data = get_bitmap(_page, _data->parent_idx() - 1);
            idx() = _data->header().get_size() - 1;
        }
        else {
            node_base *node = get_prev_leaf(_page);
            if (node != NULL) {
                _page = node;
                _data = get_bitmap(_page, get_children_count(_page) - 1);
                idx() = _data->header().get_size() - 1;
            }
            else {
                return false;
            }
        }
        return true;
    }

    bool next_leaf() {
        node_base* node = get_next_leaf(_page);
        if (node != NULL) {
            _page = node;
            return true;
        }
        return false;
    }

    bool prev_leaf() {
        node_base* node = get_prev_leaf(_page);
        if (node != NULL) {
            _page = node;
            return true;
        }
        return false;
    }

    bool prev_leaf(node_base *root) {
        node_base* node = get_prev_leaf(_page);
        if (node != NULL) {
            _page = node;
            return true;
        }
        return false;
    }

    node_base *get_next_leaf(node_base* page) {
        if (page->is_root()) {
            return NULL;
        }
        else {
            IdxType parent_idx = page->parent_idx();
            return __get_next_leaf(get_parent(page), parent_idx);
        }
    }

    node_base *get_prev_leaf(node_base* page) {
        if (page->is_root()) {
            return NULL;
        }
        else {
            IdxType parent_idx = page->parent_idx();
            return __get_prev_leaf(get_parent(page), parent_idx);
        }
    }

    bool move(csize_t i) {
        if (i > 0) {
            return move_fw(i);
        }
        else {
            return move_bw(-i);
        }
    }

    bool move_fw(csize_t i) {
        if (idx() + i < _data->header().get_size()) {
            idx() += i;
            return true;
        }
        else {
            IdxType idx0 = _data->parent_idx() + 1;

            csize_t prefix = _data->header().get_size() - idx();
            csize_t limit =  i - prefix;

            index_walker<0> main_walker(limit);
            bool down;
            node_base *node = __get_from_index_fw(_page, idx0, main_walker, false, down);

            if (node != NULL) {
                _page = node;
                _data = get_bitmap(node, idx0);
                idx() = limit - main_walker.sum();
                return true;
            }
            else {
                return false;
            }
        }
    }

    bool move_bw(csize_t i) {
        if (idx() - i >= 0) {
            idx() -= i;
            return true;
        }
        else {
            IdxType idx0 = _data->parent_idx();

            csize_t prefix = _data->header().get_size() - idx();
            csize_t limit =  i - prefix;

            index_walker<0> main_walker(limit);
            bool down;
            node_base *node = __get_from_index_bw(_page, idx0, main_walker, false, down);

            if (node != NULL) {
                _page = node;
                _data = get_bitmap(node, idx0);
                idx() = _data->header().get_size() - (limit - main_walker.sum());
                return true;
            }
            else {
                return false;
            }
        }
    }

    CountData count_one_fw() {
        counter_one_fw<> counter;
        count_one_walker<> walker;

        return count_fw(counter, walker);
    }

    CountData count_zero_fw() {
        counter_zero_fw<> counter;
        count_zero_walker<> walker;

        return count_fw(counter, walker);
    }

    template <typename Counter, typename Walker>
    CountData count_fw(Counter &counter, Walker &walker) {

        bool inserted = false;
        bool hasnt_label = false;

        index_t limit = _data->header().get_block_end(bitmap_node::head::get_block(idx()));

        counter(_data, (index_t)idx(), limit);

        index_t inserted_idx = _data->header().get_inserted();
        bool hasnt_label0 = _data->header().hasnt_label();

        if (counter.sum() < limit - idx()) {
            inserted = inserted_idx >= idx() && inserted_idx < idx() + counter.sum();
            hasnt_label = inserted & hasnt_label0;
            idx() += counter.sum();

            return CountData(counter.sum(), inserted, hasnt_label);
        }
        else {
            for (index_t c = _data->header().get_block(idx()) + 1; c < _data->header().get_blocks(); c++) {
                if (counter(_data, c)) {
                    inserted = inserted_idx >= idx() && inserted_idx < idx() + counter.sum();
                    hasnt_label = inserted & hasnt_label0;

                    idx() += counter.sum();

                    return CountData(counter.sum(), inserted, hasnt_label);
                }
            }

            inserted = inserted_idx >= idx() && inserted_idx < idx() + counter.sum();
            hasnt_label = inserted & hasnt_label0;

            IdxType idx0 = _data->parent_idx() + 1;
            bool down;
            _page = __get_from_index_fw(_page, idx0, walker, true, down);
            _data = get_bitmap(_page, idx0);
            csize_t count = walker.sum() + counter.sum();

            inserted = inserted || walker.inserted();
            hasnt_label = inserted && (hasnt_label || walker.hasnt_label());

            if (down) {
                idx() = _data->header().get_size();
            }
            else {
                counter.sum() = 0;

                inserted_idx = _data->header().get_inserted();
                hasnt_label0 = _data->header().hasnt_label();

                for (index_t c = 0; c < _data->header().get_blocks(); c++) {
                    if (counter(_data, c)) {
                        break;
                    }
                }

                inserted = inserted || (inserted_idx >= 0 && inserted_idx < counter.sum());
                hasnt_label = inserted && (hasnt_label || hasnt_label0);

                count += counter.sum();
                idx() = counter.sum();
            }

            return CountData(count, inserted, hasnt_label);
        }
    }

    CountData count_one_bw() {
        counter_one_bw<> counter;
        count_one_walker<> walker;

        return count_bw(counter, walker);
    }

    CountData count_zero_bw() {
        counter_zero_bw<> counter;
        count_zero_walker<> walker;

        return count_bw(counter, walker);
    }

    template <typename Counter, typename Walker>
    CountData count_bw(Counter &counter, Walker &walker) {
        bool inserted = false, hasnt_label = false;

        index_t limit = _data->header().get_block_start(bitmap_node::head::get_block(idx()));

        counter(_data, (index_t)idx(), limit);

        index_t inserted_idx = _data->header().get_inserted();
        bool hasnt_label0 = _data->header().hasnt_label();

        if (counter.sum() < idx() - limit) {
            inserted = inserted_idx > idx() - counter.sum() && inserted_idx <= idx();
            hasnt_label = inserted & hasnt_label0;

            idx() -= counter.sum();
            return CountData(counter.sum(), inserted, hasnt_label);
        }
        else {
            for (index_t c = _data->header().get_block(idx()) - 1; c >= 0; c--) {
                if (counter(_data, c)) {
                    inserted = inserted_idx > idx() - counter.sum() && inserted_idx <= idx();
                    hasnt_label = inserted & hasnt_label0;

                    idx() -= counter.sum();

                    return CountData(counter.sum(), inserted, hasnt_label);
                }
            }

            inserted = inserted_idx <= idx() && inserted_idx >= 0;
            hasnt_label = inserted & hasnt_label0;

            IdxType idx0 = _data->parent_idx() - 1;
            bool down;
            _page = __get_from_index_bw(_page, idx0, walker, true, down);
            _data = get_bitmap(_page, idx0);
            csize_t count = walker.sum() + counter.sum();

            inserted = inserted || walker.inserted();
            hasnt_label = inserted && (hasnt_label || walker.hasnt_label());

            if (down) {
                idx() = _data->header().get_size();
            }
            else {
                counter.sum() = 0;

                inserted_idx = _data->header().get_inserted();

                for (index_t c = _data->header().get_blocks() - 1; c >= 0; c++) {
                    if (counter(_data, c)) {
                        break;
                    }
                }

                inserted = inserted || (inserted_idx > _data->header().get_size() - counter.sum() && inserted_idx < _data->header().get_size());
                hasnt_label = inserted && (hasnt_label || hasnt_label0);

                count += counter.sum();
                idx() = _data->header().get_size() - counter.sum() - 1;
            }

            return CountData(count, inserted, hasnt_label);
        }
    }



    template <typename Value>
    void write(const Value &value) {
        value_buffer<Value> buffer(value);
        write_bits(buffer);
    }

    template <typename Buffer>
    void write_bits(const Buffer &buffer) {
        index_t count = sizeof(buffer)*8l;
        write_bits(buffer, 0l, count);
    }

    template <typename Buffer>
    void write_bits(const Buffer &buffer, index_t from, index_t count) {
        index_t limit = from + count;

        while (from < limit) {
            index_t size = _data->header().get_size();
            index_t cnt = (idx() + count <= size) ?
                count :
                size - idx();

            copy_bits(buffer, _data->bits(), from, (index_t)idx(), cnt);
            idx() += cnt;

            from += cnt;
            count -= cnt;
            if (from < limit) {
                if (!next_bitmap()) {
                    return;
                }
            }
        }

        if (idx() > _data->header().get_size()) {
            next_bitmap();
        }
    }

    template <typename Value>
    void read(Value &value) {
        value_buffer<Value> buffer(value);
        read_bits(buffer);
        value = buffer.value();
    }

    template <typename Buffer>
    void read_bits(Buffer &buffer) {
        index_t count = sizeof(buffer)*8l;
        read_bits(buffer, 0l, count);
    }

    template <typename Buffer>
    void read_bits(Buffer &buffer, index_t start, index_t count) {
        index_t limit = start + count;

        while (start < limit) {
            index_t size = _data->header().get_size();
            index_t cnt = (idx() + count <= size) ?
                count :
                size - idx();

            copy_bits(_data->bits(), buffer, (index_t)idx(), start, cnt);
            idx() += cnt;

            start += cnt;
            count -= cnt;
            if (start < limit) {
                if (!next_bitmap()) {
                    return;
                }
            }
        }

        if (idx() > _data->header().get_size()) {
            next_bitmap();
        }
    }

    CountData get_prefix() {
        iterator copy = *this;
        if (copy.move_bw(1)) {
            return copy.count_one_bw();
        }
        else {
            return CountData(0, false, false);
        }
    }

    CountData get_base_prefix() {
        if (data()->header().get_offset() == 0) {
            iterator copy = *this;
            if (copy.prev_bitmap()) {
                return copy.count_one_bw();
            }
            else {
                return CountData(0, false, false);
            }
        }
        else {
            return CountData(0, false, false);
        }
    }

protected:

    template <typename Walker>
    node_base* __get_from_index_fw(node_base *index0, IdxType &idx, Walker &walker, const bool is_down, bool &down) {
        char step_type = STEP_UP;
        down = false;
        while (step_type != STEP_BREAK) {
            index_t c, stop = get_children_count(index0);

            for (c = idx; c < stop; c++) {
                if (!walker(index0, c)) {
                    if (index0->is_leaf()) {
                        step_type = STEP_BREAK;
                        idx = c;
                    }
                    else {
                        step_type = STEP_DOWN;
                    }

                    goto nextStep;
                }
            }

            nextStep:
            if (step_type == STEP_UP) {
                if (index0->is_root()) {
                    if (is_down) {
                        down = true;
                        while (!index0->is_leaf()) {
                            index0 = get_last_child(index0);
                        }
                        idx = get_children_count(index0) - 1;
                    }
                    else {
                        index0 = NULL; // nothing is found
                        idx = -1;
                    }

                    break;
                }
                else {
                    node_base *parent = get_parent(index0);
                    idx = index0->parent_idx() + 1;
                    index0 = parent;
                }
            }
            else if (step_type == STEP_DOWN) {
                index0 = get_child(index0, c);
                idx = 0;
            }
        }

        return index0;
    }

    template <typename Walker>
    node_base* __get_from_index_bw(node_base *index0, IdxType &idx, Walker &walker, const bool is_down, bool &down) {
        int step_type = STEP_UP;

        while (step_type != STEP_BREAK) {
            index_t c = idx;
            for (index_t c = idx; c >= 0; c--) {
                if (!walker(index0, c)) {
                    if (index0->is_leaf()) {
                        step_type = STEP_BREAK;
                        idx = c;
                    }
                    else {
                        step_type = STEP_DOWN;
                    }

                    goto nextStep;
                }
            }

            nextStep:
            if (step_type == STEP_UP) {
                if (index0->is_root()){
                    if (is_down) {
                        down = true;
                        while (!index0->is_leaf()) {
                            index0 = get_child(index0, 0);
                        }
                        idx= 0;
                        step_type = STEP_BREAK;
                    }
                    else {
                        index0 = NULL; // nothing is found
                        idx = -1;
                    }
                }
                else {
                    node_base *parent = get_parent(index0);
                    idx = index0->parent_idx() - 1;
                    index0 = parent;
                }
            }
            else if (step_type == STEP_DOWN) {
                index0 = get_child(index0, c);
                idx = get_children_count(index0) - 1;
            }
        }

        return index0;
    }


    node_base *__get_next_leaf(node_base* page, IdxType &idx1) {
        if (idx1 < get_children_count(page) - 1) {
            node_base *page0 = get_child(page, idx1 + 1);

            while (!page0->is_leaf()) {
                page0 = get_child(page0, 0);
            }

            idx1++;
            return page0;
        }
        else {
            if (!page->is_root()) {
                node_base *parent = get_parent(page);
                IdxType idx0 = page->parent_idx();
                return __get_next_leaf(parent, idx0);
            }
        }

        return NULL;
    }

    node_base *__get_prev_leaf(node_base* page, IdxType &idx1) {
        if (idx1 > 0) {
            node_base *page0 = get_child(page, idx1 - 1);

            while (!page0->is_leaf()) {
                page0 = get_last_child(page0);
            }

            --idx1;
            return page0;
        }
        else {
            if (!page->is_root()) {
                node_base *parent = get_parent(page);
                IdxType idx0 = page->parent_idx();
                return __get_prev_leaf(parent, idx0);
            }
        }

        return NULL;
    }

    node_base* get_child(node_base *node, index_t idx0) {
        BV_NODE_CAST2(node, return static_cast<node_base*>(_allocator.get(_txn, __node->map().data(idx0))));
    }

    bitmap_node* get_bitmap(node_base *node, index_t idx0) {
        BV_NODE_CAST2(node, return static_cast<bitmap_node*>(_allocator.get(_txn, __node->map().data(idx0))));
    }

    node_base* get_last_child(node_base *node) {
        BV_NODE_CAST2(node,
            return static_cast<node_base*>(_allocator.get(_txn, __node->map().data(__node->children_count() - 1)))
        );
    }

    node_base* get_parent(tree_node *node) {
        return static_cast<node_base*>(_allocator.get(_txn, node->parent_id()));
    }

    static index_t get_children_count(node_base *page) {
        BV_NODE_CAST2(page, return __page->children_count());
    }

    static csize_t get_key(node_base *page, index_t i, index_t idx) {
        BV_NODE_CAST2(page, return __page->map().key(i, idx));
    }

    static void dump(node_base *page) {
        BV_NODE_CAST2(page, return __page->map().dump(std::cout));
    }
};

#endif
