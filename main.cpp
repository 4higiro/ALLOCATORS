#include <iostream>
#include <thread>

#include "alloc.hpp"

int main()
{
	const size_t n = 1E4;
	std::cout << "Allocate + deallocate in " << n << " iterations" << std::endl;

	alloc::Allocator<char, alloc::default_memory_size, alloc::MemoryLinear> linear;
	auto begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		char* p = linear.allocate(1);
		linear.deallocate(p, 1);
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time = end - begin;
	std::cout << "Linear allocator:\t" << time.count() << std::endl;

	alloc::Allocator<char, alloc::default_memory_size, alloc::MemoryStack> stack;
	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		char* p = stack.allocate(1);
		stack.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Stack allocator:\t" << time.count() << std::endl;

	alloc::Allocator<char, alloc::default_memory_size, alloc::MemoryPool> pool(4);
	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		char* p = pool.allocate(1);
		pool.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Pool allocator:\t\t" << time.count() << std::endl;

	alloc::Allocator<char, alloc::default_memory_size, alloc::MemoryLinear, alloc::STACK> linear_static;
	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		char* p = linear_static.allocate(1);
		linear_static.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Static Linear allocator:" << time.count() << std::endl;

	alloc::Allocator<char, alloc::default_memory_size, alloc::MemoryStack, alloc::STACK> stack_static;
	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		char* p = stack_static.allocate(1);
		stack_static.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Static Stack allocator:\t" << time.count() << std::endl;

	alloc::Allocator<char, alloc::default_memory_size, alloc::MemoryPool, alloc::STACK> pool_static(4);
	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		char* p = pool_static.allocate(1);
		pool_static.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Static Pool allocator:\t" << time.count() << std::endl;

	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
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
