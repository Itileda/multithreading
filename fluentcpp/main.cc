#include <thread>
#include <mutex>
#include <string>
#include <vector>

using LockMutex = std::lock_guard<std::mutex>;

void Hello(std::string foo){
    std::mutex fooMutex;
    std::mutex barMutex;

    {
        LockMutex lock(fooMutex);
        foo = "Hello Ihor";
    }
    printf("And this is safe!!!, %s\n", foo.c_str());
    printf("Now foo is empty: %s\n", foo.c_str());
}

int main(int argc, char** argv){
    std::thread printFoo(Hello, "Bye!!!");
    std::thread printFooToo(Hello, "Lena!!!");
    std::vector<std::thread> threads;
    threads.reserve(100000);
    for(auto i = 0; i < 100000; ++i){
        threads.push_back(std::thread(Hello, ""));
    }
    for(auto i = 0; i < 100000; ++i){
        if(threads[i].joinable()){
            threads[i].join();
        }
    }
    printFoo.join();
    printFooToo.join();
}