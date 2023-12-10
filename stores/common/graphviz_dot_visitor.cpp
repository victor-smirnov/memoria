
// Copyright 2021 Victor Smirnov
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

#include <memoria/api/store/swmr_store_api.hpp>
#include <memoria/profiles/core_api/core_api_profile.hpp>

#include <stack>
#include <fstream>

namespace memoria {

class GraphvizDotWritingVisitor: public SWMRStoreGraphVisitor<CoreApiProfile> {

    enum class NodeType {COMMIT, CONTAINER, BLOCK};

    class StyleMap {
        StyleMap* parent_;
        std::unordered_map<U8String, U8String> styles_;
        U8String text_;
        bool rendered_{false};
    public:
        StyleMap() noexcept:
            parent_()
        {}

        StyleMap(StyleMap* parent) noexcept:
            parent_(parent)
        {}

        void set(U8String name, U8String value) {
            styles_[name] = value;
        }

        void write_to(std::ostream& out)
        {
            parent_->write_basic_to(out);

            for (const auto& pp: styles_) {
                out << pp.first << "=" << pp.second << " ";
            }
        }

    private:
        void write_basic_to(std::ostream& out)
        {
            if (MMA_UNLIKELY(!rendered_))
            {
                std::stringstream ss;

                for (const auto& pp: styles_) {
                    ss << pp.first << "=" << pp.second << " ";
                }

                if (parent_) {
                    for (const auto& pp: parent_->styles_) {
                        if (styles_.find(pp.first) == styles_.end()) {
                            ss << pp.first << "=" << pp.second << " ";
                        }
                    }
                }

                text_ = ss.str();
            }

            out << text_;
        }
    };


    SequenceID current_snapshot_seq_id_{};

    U8String file_name_;
    std::fstream file_;

    StyleMap node_style_;
    StyleMap snapshot_style_;
    StyleMap allocator_style_;
    StyleMap blockmap_style_;
    StyleMap history_style_;
    StyleMap directory_style_;
    StyleMap ctr_style_;
    StyleMap block_style_;

    std::stack<U8String> node_stack_;

    std::unordered_map<U8String, std::unique_ptr<StyleMap>> nodes_;

    std::unordered_map<U8String, U8String> id_remapping_;

    std::unordered_set<U8String> same_rank_nodes_;

public:
    GraphvizDotWritingVisitor(U8StringView file_name):
        file_name_(file_name),
        node_style_(create_node_style()),
        snapshot_style_(create_snapshot_style(&node_style_)),
        allocator_style_(create_allocator_style(&node_style_)),
        blockmap_style_(create_blockmap_style(&node_style_)),
        history_style_(create_history_style(&node_style_)),
        directory_style_(create_directory_style(&node_style_)),
        ctr_style_(create_ctr_style(&node_style_))
    {
        file_.open(file_name_.data(), std::ios_base::out);

        id_remapping_[format_u8("{}", UUID::parse("cc2cd24f-6518-4977-81d3-dad21d4f45cc"))] = "DirectoryCtrID";
        id_remapping_[format_u8("{}", UUID::parse("a64d654b-ec9b-4ab7-870f-83816c8d0ce2"))] = "AllocationMapCtrID";
        id_remapping_[format_u8("{}", UUID::parse("0bc70e1f-adaf-4454-afda-7f6ac7104790"))] = "HistoryCtrID";
        id_remapping_[format_u8("{}", UUID::parse("177b946a-f700-421f-b5fc-0a177195b82f"))] = "BlockMapCtrID";
    }

    ~GraphvizDotWritingVisitor() noexcept {
        file_.close();
    }

    void start_graph() override {
        println(file_, "digraph {{");
    }

    void end_graph() override {
        println(file_, "}}");
    }

    void start_snapshot(const SnapshotID& snapshot_id, const SequenceID& sequence_id) override
    {
        same_rank_nodes_.clear();

        current_snapshot_seq_id_ = sequence_id;

        U8String id = format_u8("{}", snapshot_id);
        auto style = make_style(&snapshot_style_);
        style->set("label", format_u8("\"{}|{}\"", sequence_id, snapshot_id));

        file_ << "\"" << id << "\" [";
        style->write_to(file_);
        file_ << "]\n";

        nodes_[id] = std::move(style);
        node_stack_.push(id);

        same_rank_nodes_.insert(id);
    }

    void end_snapshot() override {
        node_stack_.pop();

        println(file_, "subgraph cluster_sg_{} {{", nodes_.size());
        //println(file_, "rank = \"same\"");
        //println(file_, "rankdir=\"TB\"");

        for (const U8String& id: same_rank_nodes_)
        {
            println(file_, "\"{}\"", id);
        }

        println(file_, "}}");
    }

    U8String escape_angle_brackets(U8String text) {
        U8String rr;

        for (auto cc: text.to_std_string()) {
            if (cc == '<') {
                rr += "\\<";
            }
            else if (cc == '>') {
                rr += "\\>";
            }
            else {
                rr.to_std_string().push_back(cc);
            }
        }

        return rr;
    }

    void start_ctr(CtrPtrT ctr, bool, CtrType ctr_type) override
    {
        U8String id = format_u8("{}__{}", ctr->name(), current_snapshot_seq_id_);

        switch (ctr_type) {
            case CtrType::ALLOCATOR : nodes_[id] = make_allocator_style(ctr); break;
            case CtrType::BLOCKMAP :  nodes_[id] = make_blockmap_style(ctr); break;
            case CtrType::DIRECTORY : nodes_[id] = make_directory_style(ctr); break;
            case CtrType::HISTORY :   nodes_[id] = make_history_style(ctr); break;
            case CtrType::DATA :      nodes_[id] = make_ctr_style(ctr); break;
        }

        file_ << "\"" << id << "\" [";
        nodes_[id]->write_to(file_);
        file_ << "]\n";

        const auto& snapshot_id = node_stack_.top();
        file_ << "\"" << snapshot_id << "\" -> \"" << id << "\"\n";

        node_stack_.push(id);
        same_rank_nodes_.insert(id);
    }

    void end_ctr() override {
        node_stack_.pop();
    }

    void start_block(BlockPtrT block, bool, uint64_t counters) override
    {
        U8String id = format_u8("{}", block->block_id());
        auto style = make_style(&block_style_);
        style->set("label", format_u8("\"{}|{}\"", id, counters));

        file_ << "\"" << id << "\" [";
        style->write_to(file_);
        file_ << "]\n";

        const auto& parent_id = node_stack_.top();
        file_ << "\"" << parent_id << "\" -> \"" << id << "\"\n";

        nodes_[id] = std::move(style);
        node_stack_.push(id);
        same_rank_nodes_.insert(id);
    }

    void end_block() override {
        node_stack_.pop();
    }

private:
    void set_style_label(StyleMap* style, CtrPtrT ctr) {
        auto ii = id_remapping_.find(format_u8("{}", ctr->name()));
        if (ii == id_remapping_.end()) {
            style->set("label", format_u8("\"{}|{}\"", ctr->name(), escape_angle_brackets(ctr->describe_datatype())));
        }
        else {
            style->set("label", format_u8("\"{}\"", ii->second));
        }
    }


    std::unique_ptr<StyleMap> make_allocator_style(CtrPtrT ctr)
    {
        auto style = std::make_unique<StyleMap>(&allocator_style_);

        set_style_label(style.get(), ctr);

        return style;
    }

    std::unique_ptr<StyleMap> make_blockmap_style(CtrPtrT ctr) {
        auto style = std::make_unique<StyleMap>(&blockmap_style_);

        set_style_label(style.get(), ctr);

        return style;
    }

    std::unique_ptr<StyleMap> make_directory_style(CtrPtrT ctr) {
        auto style = std::make_unique<StyleMap>(&directory_style_);

        set_style_label(style.get(), ctr);

        return style;
    }

    std::unique_ptr<StyleMap> make_history_style(CtrPtrT ctr) {
        auto style = std::make_unique<StyleMap>(&history_style_);

        set_style_label(style.get(), ctr);

        return style;
    }

    std::unique_ptr<StyleMap> make_ctr_style(CtrPtrT ctr)
    {
        auto style = std::make_unique<StyleMap>(&ctr_style_);

        auto val = ctr->get_ctr_property("CtrName");
        if (!val) {
            style->set("label", format_u8("\"{}|{}\"", ctr->name(), escape_angle_brackets(ctr->describe_datatype())));
        }
        else {
            style->set("label", format_u8("\"{}\"", val.value()));
        }

        return style;
    }


    std::unique_ptr<StyleMap> make_style(StyleMap* parent) {
        return std::make_unique<StyleMap>(parent);
    }

    static StyleMap create_node_style() {
        StyleMap map;
        map.set("shape", "record");
        return map;
    }

    static StyleMap create_snapshot_style(StyleMap* parent) {
        StyleMap map(parent);
        map.set("shape", "Mrecord");
        return map;
    }

    static StyleMap create_allocator_style(StyleMap* parent) {
        StyleMap map(parent);
        return map;
    }

    static StyleMap create_blockmap_style(StyleMap* parent) {
        StyleMap map(parent);
        return map;
    }

    static StyleMap create_history_style(StyleMap* parent) {
        StyleMap map(parent);
        return map;
    }

    static StyleMap create_directory_style(StyleMap* parent) {
        StyleMap map(parent);
        return map;
    }

    static StyleMap create_ctr_style(StyleMap* parent) {
        StyleMap map(parent);
        return map;
    }
};

std::unique_ptr<SWMRStoreGraphVisitor<CoreApiProfile>> create_graphviz_dot_visitor(U8StringView path) {
    auto rr = std::make_unique<GraphvizDotWritingVisitor>(path);
    return std::move(rr);
}


}
