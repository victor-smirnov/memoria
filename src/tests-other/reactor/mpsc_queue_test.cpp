
#include "memoria/v1/fiber/all.hpp"
#include "memoria/v1/reactor/mpsc_queue.hpp"

#include <iostream>
#include <thread>

namespace df  = memoria::v1::fibers;
namespace dr  = memoria::v1::reactor;

int main(int argc, char **argv) 
{
    dr::MPSCQueue<int> queue;
    
    std::thread th([&](){
        size_t cnt = 0;
        bool done = false;
        while (!done) 
        {
            queue.get_all([&](auto v){
                if (v < 0) 
                {
                    done = true;
                }
                else {
                    cnt++;
                }
            });
        }
        
        std::cout << "Received: " << cnt << "\n";
    });
    
    for (int c = 0; c < 10000000; c++) {
        queue.send(c);
    }
    
    queue.send(-1);
    
    th.join();
    
    
    return 0;
}
