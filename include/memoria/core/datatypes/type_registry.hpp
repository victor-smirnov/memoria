
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/datatypes/type_signature.hpp>
#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/datum_base.hpp>

#include <memoria/core/types/list/typelist.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/linked/document/linked_document.hpp>

#include <memoria/core/tools/optional.hpp>

#include <boost/any.hpp>
#include <memoria/context/detail/apply.hpp>

#include <unordered_map>
#include <mutex>
#include <tuple>

namespace memoria {

class DataTypeRegistry;

struct DataTypeOperations {
    virtual ~DataTypeOperations() noexcept {}

    virtual U8String full_type_name() = 0;
    virtual boost::any create_cxx_instance(const ld::LDTypeDeclarationView& typedecl) = 0;
    virtual AnyDatum from_ld_document(const ld::LDDValueView& value) = 0;
    virtual ld::LDDValueTag type_hash() = 0;

    virtual void dump(
            const ld::LDDocumentView* doc,
            ld::LDPtrHolder ptr,
            std::ostream& out,
            ld::LDDumpFormatState& state,
            ld::LDDumpState& dump_state
    ) = 0;

    virtual ld::LDPtrHolder deep_copy_to(
            const ld::LDDocumentView* src,
            ld::LDPtrHolder ptr,
            ld::LDDocumentView* tgt,
            ld::ld_::LDArenaAddressMapping& mapping
    ) = 0;

    virtual ld::LDDValueView construct_from(
            ld::LDDocumentView* doc,
            const ld::LDDValueView& value
    ) = 0;
};

template <typename T> struct DataTypeOperationsImpl;

namespace detail {
    template <typename T, typename ParamsList, typename... CtrArgsLists> struct DataTypeCreator;

    template <
        typename T,
        typename Registry,
        typename SerializerFn,
        bool SdnDeserializable = DataTypeTraits<T>::isSdnDeserializable
    >
    struct SDNSerializerFactory {
        static SerializerFn get_deserializer()
        {
            return [](const Registry& registry, const ld::LDDocument& decl)
            {
                return Datum<T>::from_sdn(decl);
            };
        }
    };

    template <typename T, typename Registry, typename SerializerFn>
    struct SDNSerializerFactory<T, Registry, SerializerFn, false> {
        static SerializerFn get_deserializer()
        {
            return SerializerFn();
        }
    };

}




class DataTypeRegistry {
    using CreatorFn   = std::function<boost::any (const DataTypeRegistry&, const ld::LDTypeDeclarationView&)>;
    using SdnParserFn = std::function<AnyDatum (const DataTypeRegistry&, const ld::LDDocument&)>;

    std::unordered_map<U8String, std::tuple<CreatorFn, SdnParserFn>> creators_;
    std::unordered_map<U8String, std::shared_ptr<DataTypeOperations>> operations_;
    std::unordered_map<uint64_t, std::shared_ptr<DataTypeOperations>> operations_by_code_;

public:
    friend class DataTypeRegistryStore;

    template <typename>
    friend void register_notctr_operations();

    template <typename>
    friend void register_operations();


    DataTypeRegistry();

    boost::any create_object(const ld::LDTypeDeclarationView& decl) const
    {
        U8String typedecl = decl.to_cxx_typedecl();
        auto ii = operations_.find(typedecl);
        if (ii != operations_.end())
        {
            auto ops = ii->second;
            return ops->create_cxx_instance(decl);
        }
        else {
            MMA_THROW(RuntimeException()) << format_ex("Datatype operations for {} are not registered", typedecl);
        }
    }

    AnyDatum from_sdn_string(U8StringView sdn_string) const;

    static DataTypeRegistry& local();

    void refresh();

    Optional<std::shared_ptr<DataTypeOperations>> get_operations(uint64_t type_code);
    Optional<std::shared_ptr<DataTypeOperations>> get_operations(U8StringView cxx_typedecl);


private:

    template <typename T>
    void register_operations(std::shared_ptr<DataTypeOperations> ops)
    {
        ld::TypeSignature ts = make_datatype_signature<T>();
        ld::LDDocument doc = ts.parse();
        operations_[doc.value().as_type_decl().to_cxx_typedecl()] = ops;

        uint64_t code = TypeHash<T>::Value & 0xFFFFFFFFFFFFFF;
        operations_by_code_[code] = ops;
    }

    template <typename T>
    void register_notctr_operations(std::shared_ptr<DataTypeOperations> ops)
    {
        SBuf buf;
        DataTypeTraits<T>::create_signature(buf);
        operations_[buf.str()] = ops;

        uint64_t code = TypeHash<T>::Value & 0xFFFFFFFFFFFFFF;
        operations_by_code_[code] = ops;
    }
};

class DataTypeRegistryStore {

    using CreatorFn   = typename DataTypeRegistry::CreatorFn;
    using SdnParserFn = typename DataTypeRegistry::SdnParserFn;

    std::unordered_map<U8String, std::tuple<CreatorFn, SdnParserFn>> creators_;
    std::unordered_map<U8String, std::shared_ptr<DataTypeOperations>> operations_;
    std::unordered_map<uint64_t, std::shared_ptr<DataTypeOperations>> operations_by_code_;

    mutable std::recursive_mutex mutex_;

    using LockT = std::lock_guard<std::recursive_mutex>;

public:
    friend class DataTypeRegistry;

    DataTypeRegistryStore() {}


    template <typename T>
    void register_creator(CreatorFn creator, SdnParserFn parser)
    {
        LockT lock(mutex_);

        ld::TypeSignature ts = make_datatype_signature<T>();
        ld::LDDocument doc = ts.parse();
        creators_[doc.value().as_type_decl().to_cxx_typedecl()] = std::make_tuple(creator, parser);
    }

    template <typename T>
    void register_operations(std::shared_ptr<DataTypeOperations> ops)
    {
        LockT lock(mutex_);

        ld::TypeSignature ts = make_datatype_signature<T>();
        ld::LDDocument doc = ts.parse();
        operations_[doc.value().as_type_decl().to_cxx_typedecl()] = ops;

        uint64_t code = TypeHash<T>::Value & 0xFFFFFFFFFFFFFF;
        operations_by_code_[code] = ops;
    }

    template <typename T>
    void register_notctr_operations(std::shared_ptr<DataTypeOperations> ops)
    {
        LockT lock(mutex_);

        SBuf buf;
        DataTypeTraits<T>::create_signature(buf);
        operations_[buf.str()] = ops;

        uint64_t code = TypeHash<T>::Value & 0xFFFFFFFFFFFFFF;
        operations_by_code_[code] = ops;
    }


    template <typename T>
    void unregister_creator()
    {
        LockT lock(mutex_);

        ld::TypeSignature ts = make_datatype_signature<T>();

        ld::LDDocument doc = ts.parse();
        U8String decl = doc.value().as_type_decl().to_cxx_typedecl();

        auto ii = creators_.find(decl);

        if (ii != creators_.end())
        {
            creators_.erase(ii);
        }
    }

    template <typename T, typename... ArgTypesLists>
    void register_creator_fn()
    {
        auto creator_fn = [](const DataTypeRegistry& registry, const ld::LDTypeDeclarationView& decl)
        {
            constexpr size_t declared_params_size = ListSize<DTTParameters<T>>;

            size_t actual_parameters_size = decl.params();

            if (declared_params_size == actual_parameters_size)
            {
                return detail::DataTypeCreator<
                        T,
                        DTTParameters<T>,
                        ArgTypesLists...
                >::create(registry, decl);
            }
            else {
                MMA_THROW(RuntimeException())
                        << format_ex(
                               "Actual number of parameters {} does not match expected one {} for type {}",
                               actual_parameters_size, declared_params_size,
                               decl.to_standard_string()
                           );
            }
        };

        auto parser_fn = detail::SDNSerializerFactory<T, DataTypeRegistry, SdnParserFn>::get_deserializer();

        register_creator<T>(creator_fn, parser_fn);
    }

    static DataTypeRegistryStore& global();

    template <typename T, typename... ArgsTypesLists>
    struct Initializer {
        Initializer() {
            DataTypeRegistryStore::global().register_creator_fn<T, ArgsTypesLists...>();
        }
    };


    template <typename T>
    struct OpsInitializer {
        OpsInitializer() {
            std::shared_ptr<DataTypeOperations> ops = std::make_shared<DataTypeOperationsImpl<T>>();
            DataTypeRegistryStore::global().template register_operations<T>(ops);
        }
    };

    template <typename T>
    struct NoTCtrOpsInitializer {
        NoTCtrOpsInitializer() {
            std::shared_ptr<DataTypeOperations> ops = std::make_shared<DataTypeOperationsImpl<T>>();
            DataTypeRegistryStore::global().template register_notctr_operations<T>(ops);
        }
    };

    template <typename T>
    struct DeInitializer {
        DeInitializer() {
            DataTypeRegistryStore::global().unregister_creator<T>();
        }
    };

private:
    void copy_to(DataTypeRegistry& local)
    {
        LockT lock(mutex_);
        local.creators_.clear();
        local.creators_ = creators_;

        local.operations_.clear();
        local.operations_ = operations_;


        local.operations_by_code_.clear();
        local.operations_by_code_ = operations_by_code_;
    }
};




namespace detail {

    template <typename CxxType>
    struct CtrArgsConverter {
        static bool is_convertible(const ld::LDDValueView& value) {
            return false;
        }
    };


    template <>
    struct CtrArgsConverter<bool> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_boolean();
        }

        static bool convert(const ld::LDDValueView& value) {
            return value.as_boolean();
        }
    };

    template <>
    struct CtrArgsConverter<int64_t> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_bigint();
        }

        static int64_t convert(const ld::LDDValueView& value) {
            return value.as_bigint();
        }
    };

    template <>
    struct CtrArgsConverter<double> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_double();
        }

        static double convert(const ld::LDDValueView& value) {
            return value.as_double();
        }
    };

    template <>
    struct CtrArgsConverter<U8String> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_varchar();
        }

        static U8String convert(const ld::LDDValueView& value) {
            return value.as_varchar().view();
        }
    };

    template <>
    struct CtrArgsConverter<U8StringView> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_varchar();
        }

        static U8StringView convert(const ld::LDDValueView& value) {
            return value.as_varchar().view();
        }
    };

    template <>
    struct CtrArgsConverter<ld::LDDValueView> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return true;
        }

        static ld::LDDValueView convert(const ld::LDDValueView& value) {
            return value;
        }
    };

    template <>
    struct CtrArgsConverter<ld::LDDArrayView> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_array();
        }

        static ld::LDDArrayView convert(const ld::LDDValueView& value) {
            return value.as_array();
        }
    };

    template <>
    struct CtrArgsConverter<ld::LDDMapView> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_map();
        }

        static ld::LDDMapView convert(const ld::LDDValueView& value) {
            return value.as_map();
        }
    };

    template <>
    struct CtrArgsConverter<ld::LDTypeDeclarationView> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_type_decl();
        }

        static ld::LDTypeDeclarationView convert(const ld::LDDValueView& value) {
            return value.as_type_decl();
        }
    };

    template <>
    struct CtrArgsConverter<ld::LDDTypedValueView> {
        static bool is_convertible(const ld::LDDValueView& value) {
            return value.is_typed_value();
        }

        static ld::LDDTypedValueView convert(const ld::LDDValueView& value) {
            return value.as_typed_value();
        }
    };


    template <size_t Idx, typename Types> struct DataTypeCtrArgsCheckerProc;

    template <size_t Idx, typename ArgType, typename... Types>
    struct DataTypeCtrArgsCheckerProc<Idx, TL<ArgType, Types...>> {

        static bool check(const ld::LDTypeDeclarationView& typedecl)
        {
            if (Idx < typedecl.constructor_args())
            {
                if (CtrArgsConverter<std::decay_t<ArgType>>::is_convertible(typedecl))
                {
                    return DataTypeCtrArgsCheckerProc<Idx + 1, TL<Types...>>::check(typedecl);
                }
            }

            return false;
        }
    };

    template <size_t Idx>
    struct DataTypeCtrArgsCheckerProc<Idx, TL<>> {
        static bool check(const ld::LDTypeDeclarationView& typedecl) {
            return Idx == typedecl.constructor_args();
        }
    };



    template <size_t Idx, size_t Max>
    struct FillDTTypesList {
        template <typename Tuple>
        static void process(const DataTypeRegistry& registry, const ld::LDTypeDeclarationView& typedecl, Tuple& tpl)
        {
            using ParamType = std::tuple_element_t<Idx, Tuple>;

            ld::LDTypeDeclarationView param_decl = typedecl.get_type_declration(Idx);

            std::get<Idx>(tpl) = boost::any_cast<std::decay_t<ParamType>>(registry.create_object(param_decl));

            FillDTTypesList<Idx + 1, Max>::process(registry, typedecl, tpl);
        }
    };

    template <size_t Max>
    struct FillDTTypesList<Max, Max> {
        template <typename Tuple>
        static void process(const DataTypeRegistry& registry, const ld::LDTypeDeclarationView& typedecl, Tuple& tpl){}
    };


    template <size_t Idx, size_t Max>
    struct FillDTCtrArgsList {
        template <typename Tuple>
        static void process(const ld::LDTypeDeclarationView& typedecl, Tuple& tpl)
        {
            using ArgType = std::tuple_element_t<Idx, Tuple>;

            ld::LDDValueView arg = typedecl.get_constructor_arg(Idx);

            std::get<Idx>(tpl) = CtrArgsConverter<std::decay_t<ArgType>>::convert(arg);

            FillDTCtrArgsList<Idx + 1, Max>::process(typedecl, tpl);
        }
    };

    template <size_t Max>
    struct FillDTCtrArgsList<Max, Max> {
        template <typename Tuple>
        static void process(const ld::LDTypeDeclarationView& typedecl, Tuple& tpl){}
    };



    template<typename Types>
    bool try_to_convert_args(const ld::LDTypeDeclarationView& typedecl)
    {
        constexpr int32_t list_size = ListSize<Types>;

        if (typedecl.constructor_args() > 0)
        {
            using Checker = detail::DataTypeCtrArgsCheckerProc<0, Types>;
            return Checker::check(typedecl);
        }
        else if (list_size == 0){
            return true;
        }
        else {
            return false;
        }
    }


    template <typename T, typename... ParamsList, typename... ArgsList, typename... ArgsLists>
    struct DataTypeCreator<T, TL<ParamsList...>, TL<ArgsList...>, ArgsLists...>
    {
        static T create(const DataTypeRegistry& registry, const ld::LDTypeDeclarationView& typedecl)
        {
            if (detail::try_to_convert_args<TL<ArgsList...>>(typedecl))
            {
                std::tuple<ParamsList..., ArgsList...> tpl;

                detail::FillDTTypesList<0, sizeof...(ParamsList)>
                        ::process(registry, typedecl, tpl);

                detail::FillDTCtrArgsList<sizeof...(ParamsList), sizeof...(ParamsList) + sizeof...(ArgsList)>
                        ::process(typedecl, tpl);

                auto constructorFn = [](auto... args) {
                    return T(args...);
                };

                return memoria::context::detail::apply(constructorFn, tpl);
            }
            else {
                return DataTypeCreator<T, TL<ParamsList...>, ArgsLists...>::create(registry, typedecl);
            }
        }
    };

    template <typename T, typename ParamsList>
    struct DataTypeCreator<T, ParamsList>
    {
        static T create(const DataTypeRegistry& registry, const ld::LDTypeDeclarationView& typedecl)
        {
            MMA_THROW(RuntimeException())
                    << format_ex(
                           "No constructor is defined for ({}) in type {}",
                           "aaaaaa",
                           TypeNameFactory<T>::name()
                    );
        }
    };

}


#define MMA_DEFINE_DEFAULT_DATATYPE_OPS(...) \
template <> struct DataTypeOperationsImpl<__VA_ARGS__>: CtrDataTypeOperationsImpl<__VA_ARGS__>{}

template <typename T>
void register_operations()
{
    std::shared_ptr<DataTypeOperations> ops = std::make_shared<DataTypeOperationsImpl<T>>();
    DataTypeRegistryStore::global().template register_operations<T>(ops);
    DataTypeRegistry::local().template register_operations<T>(ops);
}

template <typename T>
void register_notctr_operations()
{
    std::shared_ptr<DataTypeOperations> ops = std::make_shared<DataTypeOperationsImpl<T>>();

    DataTypeRegistryStore::global().template register_notctr_operations<T>(ops);
    DataTypeRegistry::local().template register_notctr_operations<T>(ops);
}



}
