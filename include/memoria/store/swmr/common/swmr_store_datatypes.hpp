
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


#pragma once

#include <memoria/profiles/common/common.hpp>
#include <memoria/core/tools/optional.hpp>

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/reflection.hpp>

#include <memoria/core/datatypes/core.hpp>
#include <memoria/core/datatypes/traits.hpp>

#include <memoria/core/strings/format.hpp>

#include <ostream>

namespace memoria {

template <typename Profile>
class CommitMetadata {

    using CommitID = ApiProfileSnapshotID<Profile>;
    CommitID parent_commit_id_;
    uint64_t superblock_file_pos_;
    uint64_t flags_;

public:
    static constexpr uint64_t VERSION = 1;
    enum Bits {
        PERSISTENT = 0x1
    };

    CommitMetadata() noexcept :
        superblock_file_pos_(), flags_()
    {}

    const CommitID& parent_commit_id() const noexcept {
        return parent_commit_id_;
    }

    void set_parent_commit_id(const CommitID& id) noexcept {
        parent_commit_id_ = id;
    }

    uint64_t superblock_file_pos() const noexcept {
        return superblock_file_pos_;
    }

    void set_superblock_file_pos(uint64_t pos) noexcept {
        superblock_file_pos_ = pos;
    }

    uint64_t flags() const noexcept {
        return flags_;
    }

    void set_flags(uint64_t ff) noexcept {
        flags_ = ff;
    }

    bool is_persistent() const noexcept {
        return flags_ & PERSISTENT;
    }

    void set_persistent(bool value) noexcept {
        if (value) {
            flags_ |= (uint64_t)PERSISTENT;
        }
        else {
            flags_ &= ~(uint64_t)PERSISTENT;
        }
    }

    bool operator==(const CommitMetadata& other) const noexcept {
        return parent_commit_id_ == other.parent_commit_id_ &&
                superblock_file_pos_ == other.superblock_file_pos_ &&
                flags_ == other.flags_;
    }

    bool operator!=(const CommitMetadata& other) const noexcept {
        return parent_commit_id_ != other.parent_commit_id_ ||
                superblock_file_pos_ != other.superblock_file_pos_ ||
                flags_ != other.flags_;
    }
};


template <typename Profile>
std::ostream& operator<<(std::ostream& out, const CommitMetadata<Profile>& meta) {

    out << "[";
    out << meta.parent_commit_id();
    out << ", ";
    out << meta.superblock_file_pos();
    out << ", ";
    out << meta.flags();
    out << "]";

    return out;
}


template <typename T> struct FieldFactory;

template <typename Profile>
struct FieldFactory<CommitMetadata<Profile> > {
private:
    using Type = CommitMetadata<Profile>;
    using CommitID = ApiProfileSnapshotID<Profile>;

public:
    static void serialize(SerializationData& data, const Type& field)
    {
        FieldFactory<CommitID>::serialize(data, field.parent_commit_id());
        FieldFactory<uint64_t>::serialize(data, field.superblock_file_pos());
        FieldFactory<uint64_t>::serialize(data, field.flags());
    }

    static void serialize(SerializationData& data, const Type* field, int32_t count = 1){
        for (int32_t c = 0; c < count; c++){
            serialize(data, field[c]);
        }
    }

    static void deserialize(DeserializationData& data, Type& field)
    {
        CommitID commit_id{};
        FieldFactory<CommitID>::deserialize(data, commit_id);

        uint64_t file_pos{};
        FieldFactory<uint64_t>::deserialize(data, file_pos);

        uint64_t flags{};
        FieldFactory<uint64_t>::deserialize(data, flags);

        field.set_parent_commit_id(commit_id);
        field.set_superblock_file_pos(file_pos);
        field.set_flags(flags);
    }


    static void deserialize(DeserializationData& data, Type* field, int32_t count = 1){
        for (int32_t c = 0; c < count; c++){
            deserialize(data, field[c]);
        }
    }
};


template <typename Profile>
struct TypeHash<CommitMetadata<Profile>>: UInt64Value<
    HashHelper<
        459350639468,
        TypeHashV<ApiProfileSnapshotID<Profile>>,
        TypeHashV<uint64_t>,
        TypeHashV<uint64_t>,
        CommitMetadata<Profile>::VERSION,
        TypeHashV<Profile>
    >
> {};


template <typename ProfileDT>
struct CommitMetadataDT {};

template <typename ProfileDT>
struct TypeHash<CommitMetadataDT<ProfileDT>>: UInt64Value<
    HashHelper<
        32495987114593,
        TypeHashV<CommitMetadata<ProfileFromDataType<ProfileDT>>>
    >
> {};

template <typename ProfileDT>
struct DataTypeTraits<CommitMetadataDT<ProfileDT>>: FixedSizeDataTypeTraits<CommitMetadata<ProfileFromDataType<ProfileDT>>, CommitMetadataDT<ProfileDT>> {

    using Parameters = TL<ProfileDT>;

    static void create_signature(SBuf& buf, const CommitMetadataDT<ProfileDT>&) {
        create_signature(buf);
    }

    static void create_signature(SBuf& buf) {
        buf << "CommitMetadataDT<";
        DataTypeTraits<ProfileDT>::create_signature(buf);
        buf << ">";
    }
};

}
