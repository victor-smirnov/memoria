
// Copyright 2017 Victor Smirnov
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

#include <memoria/v1/core/tools/ptr_cast.hpp>
#include <memoria/v1/core/tools/pimpl_base.hpp>
#include <memoria/v1/core/tools/optional.hpp>

#include <memoria/v1/fiber/all.hpp>

#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/reactor/reactor.hpp>
#include <memoria/v1/reactor/process.hpp>
#include <memoria/v1/reactor/timer.hpp>

#ifdef MMA1_LINUX
#include <memoria/v1/reactor/linux/linux_smp.hpp>
#include <memoria/v1/reactor/linux/linux_file.hpp>
#elif defined (MMA1_MACOSX)
#include <memoria/v1/reactor/macosx/macosx_smp.hpp>
#include <memoria/v1/reactor/macosx/macosx_file.hpp>
#elif defined (MMA1_WINDOWS)
#include <memoria/v1/reactor/msvc/msvc_smp.hpp>
#include <memoria/v1/reactor/msvc/msvc_file.hpp>
#endif

#include <memoria/v1/reactor/socket.hpp>





#include <boost/program_options.hpp>

#include <functional>
#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace memoria {
namespace v1 {
namespace reactor {

class EnvironmentImpl;

class Environment : public PimplBase<EnvironmentImpl, std::shared_ptr> {
	using Base = PimplBase<EnvironmentImpl, std::shared_ptr>;
public:
	MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Environment);

	Optional<U16String> get(const U16String& name);
	void set(const U16String& name, const U16String& value);

	EnvironmentMap entries() const;
	EnvironmentList entries_list() const;

	static Environment create(const char* const* envp);
};


namespace _ {

	std::vector<U16String> arg_list_as_vector(const char* const* args);
	U16String get_image_name(const std::vector<U16String>& args);

}

class ApplicationInit {
public:
	ApplicationInit();
	~ApplicationInit();
};

class Application final: protected ApplicationInit {
    
    using options_description   = boost::program_options::options_description;
    using variables_map         = boost::program_options::variables_map;
    
    variables_map options_;
    options_description descr_;
    
    std::shared_ptr<Smp> smp_;
    std::vector<std::shared_ptr<Reactor>> reactors_;
    
    std::function<void()> shutdown_hook_;
    
    static Application* application_;
    
    bool debug_{};

	std::vector<U16String> args_;
	Environment env_;


	U16String image_name_;
	U8String  image_name_u8_;

public:

    //Application(int argc, char** argv): Application(argc, argv, nullptr) {}
    Application(int argc, char** argv, char** envp = nullptr): 
        Application(default_options(options_description()), argc, argv, envp) 
    {}
    
    Application(options_description descr, int argc, char** argv, char** envp = nullptr);
    
    
    Application(const Application&) = delete;
    Application(Application&&) = delete;
    
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

	const std::vector<U16String>& args() const { return args_; }
	const Environment& env() const { return env_; }
	Environment& env() { return env_; }

	const U16String& image_name() const { return image_name_; }
	const U8String&  image_name_u8() const { return image_name_u8_; }
    
    const variables_map& options() {return options_;}
    
    void set_shutdown_hook(const std::function<void()>& fn) {
        shutdown_hook_ = fn;
    }
    
    bool is_help() const {
        return options_.count("help") > 0;
    }
    
    bool is_debug() const {
        return debug_;
    }
    
    template<typename Fn, typename... Args> 
    auto run(Fn&& fn, Args&&... args) 
    {
        auto msg = make_special_one_way_lambda_message(0, std::forward<Fn>(fn), std::forward<Args>(args)...);
        smp_->submit_to(0, msg.get());
        
        reactors_[0]->event_loop();
        
        join();
        
        return msg->result();
    }
    
    void print_options_and_shutdown() 
    {
        std::cout << descr_;
        run([this]{
            this->shutdown();
        });
    }
    
    
    void join() const {
        for (auto& r: reactors_) 
        {
            r->join();
        }
    }
    
    friend Application& app();
    
    void shutdown() 
    {
        for (auto& r: reactors_) 
        {
            auto msg = new OneWayFunctionMessage(r->cpu(), shutdown_hook_);
            smp_->submit_to(r->cpu(), msg);
        }
    }
    
    
    static options_description default_options(options_description descr)
    {
        descr.add_options()
            ("help,h", "Prints command line switches")
            ("threads,t", boost::program_options::value<uint32_t>()->default_value(1), "Specifies number of threads to use")
            ("debug,d", boost::program_options::value<bool>()->default_value(false), "Enable debug output");
        
        return descr;
    }
    
    template <typename Fn, typename... Args>
    static int run(int argc, char** argv, Fn&& fn, Args&&... args) noexcept
    {
        return run(
            boost::program_options::options_description(),
            argc, argv,
            std::forward<Fn>(fn), std::forward<Args>(args)...
        );
    }
    
	template <typename Fn, typename... Args>
	static int run_e(int argc, char** argv, char** envp, Fn&& fn, Args&&... args) noexcept
	{
		return run_e(
			boost::program_options::options_description(),
			argc, argv, envp,
			std::forward<Fn>(fn), std::forward<Args>(args)...
		);
	}


    template <typename Fn, typename... Args>
    static int run(
        boost::program_options::options_description options,
        int argc, char** argv, Fn&& fn, Args&&... args
    ) noexcept 
    {
        try {
            Application app(default_options(options), argc, argv);
        
            if (app.is_help()) {
                app.print_options_and_shutdown();
                return 0;
            }
            else {
                return app.run(std::forward<Fn>(fn), std::forward<Args>(args)...);
            }
        }
        catch (MemoriaThrowable& ex) {
            ex.dump(std::cout);
        }
        catch (std::exception& ex) {
            std::cerr << "Exception cought main thread: " << ex.what() << std::endl;
        }
        catch (...) {
            std::cerr << "Unrecognized exception cought main thread" << std::endl;
        }
        
        return 1;
    }

	template <typename Fn, typename... Args>
	static int run_e(
		boost::program_options::options_description options,
		int argc, char** argv, char** envp, Fn&& fn, Args&&... args
	) noexcept
	{
		try {
            Application app(default_options(options), argc, argv, envp);

			if (app.is_help()) {
				app.print_options_and_shutdown();
				return 0;
			}
			else {
				return app.run(std::forward<Fn>(fn), std::forward<Args>(args)...);
			}
		}
		catch (MemoriaThrowable& ex) {
			ex.dump(std::cout);
		}
		catch (std::exception& ex) {
			std::cerr << "Exception cought main thread: " << ex.what() << std::endl;
		}
		catch (...) {
			std::cerr << "Unrecognized exception cought main thread" << std::endl;
		}

		return 1;
	}
};



Application& app();

class ShutdownOnScopeExit {
public:
    ~ShutdownOnScopeExit() {
        engine().cout() << std::flush;
        app().shutdown();
    }
};

}}}
