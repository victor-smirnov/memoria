
#include "memoria/v1/fiber/all.hpp"
#include "memoria/v1/reactor/application.hpp"
#include "memoria/v1/filesystem/path.hpp"
#include "memoria/v1/filesystem/operations.hpp"
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <iostream>
#include <thread>
#include <vector>

namespace m  = memoria::v1;

namespace df  = memoria::v1::fibers;
namespace dr  = memoria::v1::reactor;
namespace mt  = memoria::v1::tools;
namespace fs  = memoria::v1::filesystem;

using namespace dr;


int main(int argc, char **argv) 
{
    Application app(argc, argv);
    
    auto vv = app.run([](){
        try {
        
            std::cout << "Hello from Intellectio!" << std::endl;
            
			fs::path pp("file.data");

			std::cout << "Data file path: " << fs::absolute(pp) << std::endl;

            fs::remove(pp);
            
            auto file = open_buffered_file(pp, FileFlags::CREATE | FileFlags::RDWR ); //| FileFlags::TRUNCATE
                        
            std::vector<uint8_t> data(100000000);
            
            m::Seed(12345);
            
            for (auto& v: data) {
                v = m::getRandomG();
            }
            
            auto out = file->ostream();
            
            int64_t t0 = m::getTimeInMillis();
            
            for (size_t c = 0; c < data.size();)
            {
                size_t len = m::getRandomG(100);
                if (c + len > data.size()) 
                {
                    len = data.size() - c;
                }
                
                out.write(data.data() + c, len);
                
                c += len;
            }
           
            out.flush_buffer();
            
            std::cout << "Done! " << m::FormatTime(m::getTimeInMillis() - t0) << std::endl;
            
            std::vector<uint8_t> buf0(256);
            
            auto in = file->istream(0);
            
            for (size_t c = 0; c < data.size();)
            {
                //std::cout << c << std::endl;
                size_t len = m::getRandomG(buf0.size());
                
                auto len1 = in.read(buf0.data(), len);
                
                if (len1 != len) {
                    std::cout << c << ": len = " << len << " len1 = " << len1 << std::endl;
                }
                
                for (size_t d = 0; d < len1; d++) 
				{
                    if (data[c + d] != buf0[d])
                    {
                        mt::rise_error(m::SBuf() << "Not equal! " << (c + d) << " " << std::hex << (c + d));
                    }
                }
                
                c += len;
            }
                        
            file->close();
        }
        catch (std::exception& ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
        }

		int ii;
		std::cin >> ii;

        dr::app().shutdown();
        return 5678;
    });

    std::cout << "vv = " << vv << std::endl;
    
    return 0;
}
