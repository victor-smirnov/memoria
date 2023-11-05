
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

#include <memoria/core/strings/format.hpp>
#include <memoria/core/tools/result.hpp>


#include <code_module.hpp>
#include <codegen.hpp>

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Serialization/ASTReader.h>

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/SmallString.h>

#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

namespace memoria {
namespace codegen {

using namespace clang;
using namespace llvm;

namespace {
std::vector<std::string> parser_options;
//std::unordered_set<std::string> enabled_profiles;
//std::unordered_set<std::string> disabled_profiles;
}

class BuildASTAction : public tooling::ToolAction {
  std::unique_ptr<ASTUnit>& ASTs;

public:
  BuildASTAction(std::unique_ptr<ASTUnit> &ASTs) : ASTs(ASTs) {}

  bool runInvocation(std::shared_ptr<CompilerInvocation> Invocation,
                     FileManager *Files,
                     std::shared_ptr<PCHContainerOperations> PCHContainerOps,
                     DiagnosticConsumer *DiagConsumer) override
  {
    Invocation->getFrontendOpts().SkipFunctionBodies = true;

    std::unique_ptr<ASTUnit> AST = ASTUnit::LoadFromCompilerInvocation(
        Invocation, std::move(PCHContainerOps),
        CompilerInstance::createDiagnostics(&Invocation->getDiagnosticOpts(),
                                            DiagConsumer,
                                            /*ShouldOwnClient=*/false),
        Files);

    if (!AST)
      return false;

    ASTs = std::move(AST);
    return true;
  }
};




namespace {

class InferenceVisitor: public RecursiveASTVisitor<InferenceVisitor> {
    using Base = RecursiveASTVisitor<InferenceVisitor>;

public:
    std::vector<std::pair<int64_t, U8String>> results;

public:

    bool shouldVisitTemplateInstantiations() const noexcept {
        return true;
    }

    bool VisitClassTemplateSpecializationDecl(ClassTemplateSpecializationDecl * RD) noexcept
    {
        if (RD->getNameAsString() == "CGValueInf")
        {
            const auto& list = RD->getTemplateArgs();
            int64_t idx = list.get(0).getAsIntegral().getExtValue();

            SmallString<16> str;

            list.get(1).getAsIntegral().toString(str, 10);

            U8String val = str.str().str();
            results.push_back(std::make_pair(idx, val));
        }
        else if (RD->getNameAsString() == "CGTypeInf")
        {
            const auto& list = RD->getTemplateArgs();
            int64_t idx = list.get(0).getAsIntegral().getExtValue();
            QualType type = list.get(1).getAsType();
            results.push_back(std::make_pair(idx, U8String(type.getAsString())));
        }

        return true;
    }


};

}

class CodeModuleImpl: public CodeModule, std::enable_shared_from_this<CodeModuleImpl> {
    std::unique_ptr<ASTUnit> ast_unit_;
public:
    CodeModuleImpl(std::unique_ptr<ASTUnit>&& ast_unit) noexcept :
        ast_unit_(std::move(ast_unit))
    {}

    clang::ASTUnit& ast_unit() noexcept override {
        return *ast_unit_.get();
    }

    const clang::ASTUnit& ast_unit() const noexcept override {
        return *ast_unit_.get();
    }

    U8String file_path() const override {
        return ast_unit_->getPCHFile()->getName().str();
    }


};


class PreCompiledHeaderImpl: public PreCompiledHeader {
    ShPtr<PreCompiledHeader> parent_;
    std::vector<U8String> options_;
    U8String file_path_;
    U8String target_folder_;
public:
    PreCompiledHeaderImpl(
        ShPtr<PreCompiledHeader> parent,
        std::vector<U8String> options,
        U8String file_path,
        U8String target_folder
    ):
        parent_(parent),
        options_(options),
        file_path_(file_path),
        target_folder_(target_folder)
    {}

    ShPtr<PreCompiledHeader> parent() const noexcept override {
        return parent_;
    }

    const std::vector<U8String>& options() const noexcept override {
        return options_;
    }

    U8String file_path() const override {
        return file_path_;
    }

    U8String target_folder() const override {
        return target_folder_;
    }

    ShPtr<CodeModule> compile_with(U8String code) const override
    {
        std::vector<U8String> opts = options_;

        opts.push_back("-include-pch");
        opts.push_back(file_path());

        return parse_file(opts, "snippet.cpp", code);
    }

    std::vector<U8String> infer(const std::vector<Inference>& inferences) const override
    {
        U8String code = R"(
template <int ID, typename T>
struct CGTypeInf {};

template <int ID, auto V>
struct CGValueInf {};

)";
        for (size_t c = 0; c < inferences.size(); c++)
        {
            const auto& inf = inferences[c];

            if (inf.type() == InferenceType::TYPE) {
                code += format_u8("using CFType_{0} = {1};\n", c, inf.expression());
                code += format_u8("CGTypeInf<{0}, CFType_{0}> cg_foo_type_inf_{0}();\n\n", c);
            }
            else {
                code += format_u8("CGValueInf<{0}, {1}> cg_foo_value_inf_{0}();\n\n", c, inf.expression());
            }
        }

        auto unit = compile_with(code);

        InferenceVisitor visitor;
        visitor.TraverseAST(unit->ast_unit().getASTContext());

        std::sort(visitor.results.begin(), visitor.results.end(), [](auto& one, auto& two){
            return one.first < two.first;
        });

        std::vector<U8String> results;
        for (auto& rr: visitor.results) {
            results.push_back(std::move(rr.second));
        }

        return results;
    }
};

ShPtr<PreCompiledHeader> PreCompiledHeader::create(
        const std::vector<U8String>& opts,
        const U8String& target_folder,
        const U8String& header_name
) {
    U8String pch_path = regenerate_pch(opts, target_folder, header_name);

    return std::make_shared<PreCompiledHeaderImpl>(
        ShPtr<PreCompiledHeader>{},
        opts,
        pch_path,
        target_folder
    );
}

ShPtr<PreCompiledHeader> PreCompiledHeader::create(
        ShPtr<PreCompiledHeader> parent,
        const U8String& target_folder,
        const U8String& header_name
) {
    U8String pch_path = regenerate_pch(parent->options(), target_folder, parent->file_path(), header_name);

    return std::make_shared<PreCompiledHeaderImpl>(
        parent,
        parent->options(),
        pch_path,
        target_folder
    );
}


void add_parser_clang_option(const U8String& text) {
    parser_options.push_back(text);
}

const std::vector<std::string>& get_parser_options() {
    return parser_options;
}


ShPtr<CodeModule> parse_file(const std::vector<U8String>& extra_opts, const U8String& file_name, const U8String& text)
{
    IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> OverlayFileSystem(
        new vfs::OverlayFileSystem(vfs::getRealFileSystem()));

    IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> InMemoryFileSystem(
        new vfs::InMemoryFileSystem);

    OverlayFileSystem->pushOverlay(InMemoryFileSystem);

    IntrusiveRefCntPtr<FileManager> Files(
        new FileManager(FileSystemOptions(), OverlayFileSystem));

    std::vector<std::string> Args;
    Args.push_back("MemoriaCodeGen");
    Args.push_back("-Wfatal-errors");
    Args.push_back("-DMEMORIA_CODEGEN");
    Args.push_back("-fsyntax-only");


    for (const auto& option: parser_options) {
        Args.push_back(option);
    }

    Args.push_back(file_name);

    for (const auto& opt: extra_opts) {
        Args.push_back(opt);
    }

    std::unique_ptr<ASTUnit> ast;

    BuildASTAction action(ast);
    tooling::ToolInvocation Invocation(
        Args, &action, Files.get(), std::make_shared<PCHContainerOperations>());

    InMemoryFileSystem->addFile(
        file_name.to_std_string(), 0, llvm::MemoryBuffer::getMemBuffer(text.to_std_string()));

    if (!Invocation.run()) {
        throw std::runtime_error("Can't execute the action");
    }

    return std::make_shared<CodeModuleImpl>(std::move(ast));
}

ShPtr<CodeModule> parse_file(const std::vector<U8String>& extra_opts, const U8String& file_name)
{
    auto text = load_text_file(file_name);
    return parse_file(extra_opts, file_name, text);
}



struct CGGeneratePCHAction: GeneratePCHAction {

    virtual bool BeginInvocation(CompilerInstance &CI)
    {
        CI.getFrontendOpts().SkipFunctionBodies = true;
        return GeneratePCHAction::BeginInvocation(CI);
    }
};

U8String generate_pch(const U8String& target_directory, const U8String& header_file_name)
{
    U8String header_path = target_directory + "/" + header_file_name;
    U8String pch_path = header_path + ".pch";

    if (std::filesystem::exists(pch_path.to_std_string()))
    {
        auto header_time = std::filesystem::last_write_time(header_path.to_std_string());
        auto pch_time = std::filesystem::last_write_time(pch_path.to_std_string());

        if (header_time < pch_time) {
            return pch_path;
        }
        else {
            return regenerate_pch({}, target_directory, header_file_name);
        }
    }
    else {
        return regenerate_pch({}, target_directory, header_file_name);
    }
}

U8String regenerate_pch(
        const std::vector<U8String>& extra_opts,
        const U8String& target_directory,
        const U8String& pch, const U8String& header_file_name
)
{
    IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> OverlayFileSystem(
        new vfs::OverlayFileSystem(vfs::getRealFileSystem()));

    IntrusiveRefCntPtr<FileManager> Files(
        new FileManager(FileSystemOptions(), OverlayFileSystem));

    std::vector<std::string> Args;
    Args.push_back("MemoriaCodeGen");

    for (const auto& option: parser_options) {
        Args.push_back(option);
    }

    Args.push_back("-DMEMORIA_CODEGEN");
    Args.push_back("-Wfatal-errors");
    Args.push_back("-include-pch");
    Args.push_back(pch);
    Args.push_back("-x");
    Args.push_back("c++-header");
    Args.push_back("-o");
    Args.push_back(target_directory + "/" + header_file_name + ".pch");

    for (auto& oo: extra_opts) {
        Args.push_back(oo);
    }

    Args.push_back(target_directory + "/" + header_file_name);

    std::unique_ptr<ASTUnit> ast;

    tooling::ToolInvocation Invocation(
        Args, std::make_unique<CGGeneratePCHAction>(), Files.get(), std::make_shared<PCHContainerOperations>());

    if (!Invocation.run()) {
        throw std::runtime_error("Can't execute the action");
    }

    return target_directory + "/" + header_file_name + ".pch";

}

U8String regenerate_pch(
        const std::vector<U8String>& extra_opts,
        const U8String& target_directory,
        const U8String& header_file_name
)
{
    IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> OverlayFileSystem(
        new vfs::OverlayFileSystem(vfs::getRealFileSystem()));

    IntrusiveRefCntPtr<FileManager> Files(
        new FileManager(FileSystemOptions(), OverlayFileSystem));

    std::vector<std::string> Args;
    Args.push_back("MemoriaCodeGen");

    for (const auto& option: parser_options) {
        Args.push_back(option);
    }

    Args.push_back("-DMEMORIA_CODEGEN");
    Args.push_back("-Wfatal-errors");
    Args.push_back("-x");
    Args.push_back("c++-header");
    Args.push_back("-o");
    Args.push_back(target_directory + "/" + header_file_name + ".pch");

    for (const auto& option: extra_opts) {
        Args.push_back(option);
    }

    Args.push_back(target_directory + "/" + header_file_name);

    std::unique_ptr<ASTUnit> ast;

    tooling::ToolInvocation Invocation(
        Args, std::make_unique<CGGeneratePCHAction>(), Files.get(), std::make_shared<PCHContainerOperations>());

    if (!Invocation.run()) {
        throw std::runtime_error("Can't execute the action");
    }

    return target_directory + "/" + header_file_name + ".pch";
}


U8String load_text_file(U8String file_name)
{
    std::fstream ff(file_name, std::ios_base::in);
    if (!ff.is_open()) {
        auto msg = format_u8("Can't open file {}", file_name);
        throw std::runtime_error(msg.to_std_string());
    }

    size_t size = std::filesystem::file_size(file_name.to_std_string());
    std::string str;
    str.resize(size, ' ');

    ff.read(str.data(), size);

    ff.close();

    return str;
}

void write_text_file(U8String file_name, U8String data)
{
    std::fstream ff(file_name, std::ios_base::out | std::ios_base::trunc);
    if (!ff.is_open()) {
        auto msg = format_u8("Can't create file {}", file_name);
        throw std::runtime_error(msg.to_std_string());
    }

    ff << data;
    ff.flush();
    ff.close();
}


void write_text_file_if_different(U8String file_name, U8String data)
{
    if (!std::filesystem::exists(file_name.to_std_string())) {
        write_text_file(file_name, data);
    }
    else {
        U8String exisitng_data = load_text_file(file_name);
        if (exisitng_data != data) {
            write_text_file(file_name, data);
        }
    }
}


}}
