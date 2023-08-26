#include <iostream>
#include <thread>

#include "alloc.hpp"

int main()
{
	std::cout << "Allocate + deallocate in 1e6 iterations" << std::endl;

	alloc::Allocator<int, alloc::default_memory_size, alloc::MemoryLinear> linear;
	auto begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < 1E6; ++i)
	{
		int* p = linear.allocate(1);
		linear.deallocate(p, 1);
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time = end - begin;
	std::cout << "Linear allocator:\t" << time.count() << std::endl;

	alloc::Allocator<int, alloc::default_memory_size, alloc::MemoryStack> stack;
	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < 1E6; ++i)
	{
		int* p = stack.allocate(1);
		stack.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Stack allocator:\t" << time.count() << std::endl;

	alloc::Allocator<int, alloc::default_memory_size, alloc::MemoryPool> pool(4);
	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < 1E6; ++i)
	{
		int* p = pool.allocate(1);
		pool.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Pool allocator:\t\t" << time.count() << std::endl;

	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < 1E6; ++i)
	{
		int* p = new int;
		delete p;
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Malloc:\t\t\t" << time.count() << std::endl;

	system("pause");
	return 0;
}