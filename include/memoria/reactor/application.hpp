
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

#include <memoria/core/tools/ptr_cast.hpp>
#include <memoria/core/tools/pimpl_base.hpp>
#include <memoria/core/tools/optional.hpp>

#include <memoria/fiber/all.hpp>

#include <memoria/core/tools/optional.hpp>
#include <memoria/reactor/reactor.hpp>
#include <memoria/reactor/process.hpp>
#include <memoria/reactor/timer.hpp>
#include <memoria/reactor/socket.hpp>

#ifdef MMA_LINUX
#include <memoria/reactor/linux/linux_smp.hpp>
#include <memoria/reactor/linux/linux_file.hpp>
#elif defined (MMA_MACOSX)
#include <memoria/reactor/macosx/macosx_smp.hpp>
#include <memoria/reactor/macosx/macosx_file.hpp>
#elif defined (MMA_WINDOWS)
#include <memoria/reactor/msvc/msvc_smp.hpp>
#include <memoria/reactor/msvc/msvc_file.hpp>
#endif

#include <boost/program_options.hpp>

#include <functional>
#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace memoria {
namespace reactor {

class EnvironmentImpl;

class Environment : public PimplBase<EnvironmentImpl, std::shared_ptr> {
	using Base = PimplBase<EnvironmentImpl, std::shared_ptr>;
public:
    MMA_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Environment)

    Optional<U8String> get(const U8String& name);
    void set(const U8String& name, const U8String& value);

	EnvironmentMap entries() const;
	EnvironmentList entries_list() const;

	static Environment create(const char* const* envp);
};


namespace _ {

    std::vector<U8String> arg_list_as_vector(const char* const* args);
    U8String get_image_name();

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

    std::vector<U8String> args_;
	Environment env_;

    uint64_t iopoll_timeout_{10}; // 10 ms

    uint32_t threads_{1};

public:

    //Application(int argc, char** argv): Application(argc, argv, nullptr) {}
    Application(int argc, char** argv, char** envp = nullptr): 
        Application(options_description(), argc, argv, envp)
    {
        default_options(descr_);
    }
    
    Application(const options_description& descr, int argc, char** argv, char** envp = nullptr);
    
    
    Application(const Application&) = delete;
    Application(Application&&) = delete;
    
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

    uint32_t threads() const {return threads_;}
    void set_threads(int threads) {
        threads_ = threads;
    }

    uint64_t iopoll_timeout() const {return iopoll_timeout_;}
    void set_iopoll_timeout(uint64_t value_ms) {
        iopoll_timeout_ = value_ms;
    }

    void start_engines();


    const std::vector<U8String>& args() const { return args_; }
	const Environment& env() const { return env_; }
	Environment& env() { return env_; }
    
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
        
        reactors_[0]->event_loop(iopoll_timeout_);
        
        join();
        
        return msg->result();
    }
    
    void print_options()
    {
        std::cout << descr_;
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
    
    int start_main_event_loop();
    
    static void default_options(options_description& descr)
    {
        descr.add_options()
            ("help,h", "Prints command line switches")
            ("threads,t", boost::program_options::value<uint32_t>()->default_value(1), "Specifies number of threads to use")
            ("debug,d", boost::program_options::value<bool>()->default_value(false), "Enable debug output")
            ("io-timeout", boost::program_options::value<uint64_t>()->default_value(20), "Event poller timeout value, in milliseconds.");
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
            default_options(options);
            Application app(options, argc, argv);

            app.start_engines();

            if (app.is_help()) {
                app.print_options();
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
            default_options(options);
            Application app(options, argc, argv, envp);

            app.start_engines();

			if (app.is_help()) {
                app.print_options();
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
        engine().cerr() << std::flush;
        app().shutdown();
    }
};

}}
