#include <iostream>
#include <vector>
#include <thread>

#include "alloc.hpp"

int main()
{
	alloc::Allocator<int> linear;
	alloc::Allocator<int, alloc::MemoryStack> stack;
	alloc::Allocator<int, alloc::MemoryPool> pool;

	constexpr size_t n = 2E4;
	std::cout << "Allocate + deallocate in " << n << " iterations" << std::endl;

	auto begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		int* p = linear.allocate(1);
		linear.deallocate(p, 1);
	}
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time = end - begin;
	std::cout << "Linear allocator:\t" << time.count() << std::endl;

	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		int* p = stack.allocate(1);
		stack.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Stack allocator:\t" << time.count() << std::endl;

	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		int* p = pool.allocate(1);
		pool.deallocate(p, 1);
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Pool allocator:  \t" << time.count() << std::endl;

	begin = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < n; ++i)
	{
		int* p = new int;
		delete p;
	}
	end = std::chrono::high_resolution_clock::now();
	time = end - begin;
	std::cout << "Malloc & free:   \t" << time.count() << std::endl;

	system("pause");
	return 0;
}
