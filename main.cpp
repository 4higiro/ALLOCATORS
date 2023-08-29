#include <thread>

#define DEFAULT_MEMORY_SIZE 128, MiB
#include "alloc.hpp"

int main()
{
    constexpr size_t n = 1E6;
    std::cout << "Allocate + deallocate time in " << n << " iteration" << std::endl;

    auto heap = alloc::make_resource(alloc::HEAP);
    alloc::Allocator<int> heap_allocator(heap);
    alloc::list<int> l1(heap_allocator);
    auto begin = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        l1.push_back(i);
        l1.pop_back();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end - begin;
    std::cout << "cpp new/delete:\t\t" << time.count() << std::endl;

    auto linear = alloc::make_resource(alloc::LINEAR);
    alloc::Allocator<int> linear_allocator(linear);
    alloc::list<int> l2(linear_allocator);
    begin = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        l2.push_back(i);
        l2.pop_back();
    }
    end = std::chrono::high_resolution_clock::now();
    time = end - begin;
    std::cout << "linear allocator:\t" << time.count() << std::endl;
    
    auto stack = alloc::make_resource(alloc::STACK);
    alloc::Allocator<int> stack_allocator(stack);
    alloc::list<int> l3(stack_allocator);
    begin = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        l3.push_back(i);
        l3.pop_back();
    }
    end = std::chrono::high_resolution_clock::now();
    time = end - begin;
    std::cout << "stack allocator:\t" << time.count() << std::endl;
    
    auto pool = alloc::make_resource(alloc::POOL);
    alloc::Allocator<int> pool_allocator(pool);
    alloc::list<int> l4(pool_allocator);
    begin = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i)
    {
        l4.push_back(i);
        l4.pop_back();
    }
    end = std::chrono::high_resolution_clock::now();
    time = end - begin;
    std::cout << "pool allocator:\t\t" << time.count() << std::endl;

    system("pause");
    return 0;
}