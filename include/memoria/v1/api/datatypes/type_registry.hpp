
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

#include <memoria/v1/api/datatypes/type_signature.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/core/types/list/typelist.hpp>

#include <boost/any.hpp>
#include <boost/context/detail/apply.hpp>

#include <unordered_map>
#include <mutex>

namespace memoria {
namespace v1 {

namespace _ {
    template <typename T, typename ParamsList, typename... CtrArgsLists> struct DataTypeCreator;
}

class DataTypeRegistry {
    using CreatorFn = std::function<boost::any (DataTypeRegistry&, const DataTypeDeclaration&)>;

    std::map<U8String, CreatorFn> creators_;

public:
    friend class DataTypeRegistryStore;

    DataTypeRegistry();

    boost::any create_object(const DataTypeDeclaration& decl)
    {
        U8String typedecl = decl.to_typedecl_string();
        auto ii = creators_.find(typedecl);
        if (ii != creators_.end())
        {
            return ii->second(*this, decl);
        }
        else {
            MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Type creator for {} is not registered", typedecl);
        }
    }

    static DataTypeRegistry& local();

    void refresh();
};

class DataTypeRegistryStore {

    using CreatorFn = std::function<boost::any (DataTypeRegistry&, const DataTypeDeclaration&)>;

    std::map<U8String, CreatorFn> creators_;

    mutable std::mutex mutex_;

    using LockT = std::lock_guard<std::mutex>;

public:
    friend class DataTypeRegistry;

    DataTypeRegistryStore() {}


    template <typename T>
    void register_creator(CreatorFn creator)
    {
        LockT lock(mutex_);

        TypeSignature ts = make_datatype_signature<T>();
        creators_[ts.parse().to_typedecl_string()] = creator;
    }

    template <typename T>
    void unregister_creator()
    {
        LockT lock(mutex_);

        TypeSignature ts = make_datatype_signature<T>();

        U8String decl = ts.parse().to_typedecl_string();

        auto ii = creators_.find(decl);

        if (ii != creators_.end())
        {
            creators_.erase(ii);
        }
    }

    template <typename T, typename... ArgTypesLists>
    void register_creator_fn()
    {
        register_creator<T>([](DataTypeRegistry& registry, const DataTypeDeclaration& decl)
        {
            constexpr size_t declared_params_size = ListSize<typename DataTypeTraits<T>::Parameters>;

            size_t actual_parameters_size = decl.parameters().has_value() ?
                        decl.parameters().get().size() : 0;

            if (declared_params_size == actual_parameters_size)
            {
                return _::DataTypeCreator<
                        T,
                        typename DataTypeTraits<T>::Parameters,
                        ArgTypesLists...
                >::create(registry, decl.parameters(), decl.constructor_args());
            }
            else {
                MMA1_THROW(RuntimeException())
                        << fmt::format_ex(
                               u"Actual number of parameters {} does not match expected one {} for type {}",
                               actual_parameters_size, declared_params_size,
                               decl.full_type_name()
                           );
            }
        });
    }

    static DataTypeRegistryStore& global();

    template <typename T, typename... ArgsTypesLists>
    struct Initializer {
        Initializer() {
            DataTypeRegistryStore::global().register_creator_fn<T, ArgsTypesLists...>();
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
    }
};




namespace _ {

    template <typename T> struct DTCtrArgTraits;

    template <>
    struct DTCtrArgTraits<U8String> {
        static constexpr int32_t which = 0;
    };

    template <>
    struct DTCtrArgTraits<int64_t> {
        static constexpr int32_t which = 1;
    };

    template <>
    struct DTCtrArgTraits<double> {
        static constexpr int32_t which = 2;
    };

    template <>
    struct DTCtrArgTraits<NameToken> {
        static constexpr int32_t which = 3;
    };


    template <size_t Idx, typename Types> struct DataTypeCtrArgsCheckerProc;

    template <size_t Idx, typename ArgType, typename... Types>
    struct DataTypeCtrArgsCheckerProc<Idx, TL<ArgType, Types...>> {

        static bool check(const std::vector<DataTypeCtrArg>& args)
        {
            if (Idx < args.size())
            {
                int32_t arg_idx = args[Idx].value().which();
                int32_t which_idx = DTCtrArgTraits<std::decay_t<ArgType>>::which;

                if (which_idx == arg_idx) {
                    return DataTypeCtrArgsCheckerProc<Idx + 1, TL<Types...>>::check(args);
                }
            }

            return false;
        }
    };

    template <size_t Idx>
    struct DataTypeCtrArgsCheckerProc<Idx, TL<>> {
        static bool check(const std::vector<DataTypeCtrArg>& args) {
            return Idx == args.size();
        }
    };



    template <size_t Idx, size_t Max>
    struct FillDTTypesList {
        template <typename Tuple>
        static void process(DataTypeRegistry& registry, const DataTypeParams& params, Tuple& tpl)
        {
            using ParamType = std::tuple_element_t<Idx, Tuple>;

            auto& vec = params.get();
            const DataTypeDeclaration& param_decl = vec[Idx];

            std::get<Idx>(tpl) = boost::any_cast<std::decay_t<ParamType>>(registry.create_object(param_decl));

            FillDTTypesList<Idx + 1, Max>::process(registry, params, tpl);
        }
    };

    template <size_t Max>
    struct FillDTTypesList<Max, Max> {
        template <typename Tuple>
        static void process(DataTypeRegistry& registry, const DataTypeParams& params, Tuple& tpl){}
    };


    template <size_t Idx, size_t Max>
    struct FillDTCtrArgsList {
        template <typename Tuple>
        static void process(const DataTypeCtrArgs& args, Tuple& tpl)
        {
            using ArgType = std::tuple_element_t<Idx, Tuple>;

            auto& vec = args.get();
            const DataTypeCtrArg& arg_decl = vec[Idx];

            std::get<Idx>(tpl) = boost::get<std::decay_t<ArgType>>(arg_decl.value());

            FillDTCtrArgsList<Idx + 1, Max>::process(args, tpl);
        }
    };

    template <size_t Max>
    struct FillDTCtrArgsList<Max, Max> {
        template <typename Tuple>
        static void process(const DataTypeCtrArgs& args, Tuple& tpl){}
    };



    template<typename Types>
    bool try_to_convert(const DataTypeCtrArgs& args)
    {
        constexpr int32_t list_size = ListSize<Types>;

        if (args.has_value()) {
            using Checker = _::DataTypeCtrArgsCheckerProc<0, Types>;
            return Checker::check(args.get());
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
        static T create(DataTypeRegistry& registry, const DataTypeParams& params, const DataTypeCtrArgs& args)
        {
            if (_::try_to_convert<TL<ArgsList...>>(args))
            {
                std::tuple<ParamsList..., ArgsList...> tpl;

                _::FillDTTypesList<0, sizeof...(ParamsList)>
                        ::process(registry, params, tpl);

                _::FillDTCtrArgsList<sizeof...(ParamsList), sizeof...(ParamsList) + sizeof...(ArgsList)>
                        ::process(args, tpl);

                auto constructorFn = [](auto... args) {
                    return T(args...);
                };

                return boost::context::detail::apply(constructorFn, tpl);
            }
            else {
                return DataTypeCreator<T, TL<ParamsList...>, ArgsLists...>::create(registry, params, args);
            }
        }
    };

    template <typename T, typename ParamsList>
    struct DataTypeCreator<T, ParamsList>
    {
        static T create(DataTypeRegistry& registry, const DataTypeParams& params, const DataTypeCtrArgs& args)
        {
            MMA1_THROW(RuntimeException())
                    << fmt::format_ex(
                           u"No constructor is defined for ({}) in type {}",
                           "aaa",
                           TypeNameFactory<T>::name()
                    );
        }
    };

}

}}
