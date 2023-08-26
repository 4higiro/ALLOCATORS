#ifndef CUSTOM_ALLOCATORS
#define CUSTOM_ALLOCATORS

#ifndef DEFAULT_MEMORY_SIZE 
#define DEFAULT_MEMORY_SIZE 4, MiB
#endif
#ifndef META_MEMORY_SIZE 
#define META_MEMORY_SIZE 1, KiB
#endif 

namespace alloc
{
	enum memory_unit_t
	{
		BT = 1,
		KiB = 1'024,
		KB = 1'000,
		MiB = 1'048'576,
		MB = 1'000'000,
		GiB = 1'073'741'824,
		GB = 1'000'000'000
	};

	enum loc_t
	{
		STACK = 0,
		HEAP = 1
	};

	template <size_t Count, memory_unit_t Unit>
	struct MemoryUnit
	{
		constexpr static size_t byte()     { return Count * Unit / BT;  }
		constexpr static size_t kibibyte() { return Count * Unit / KiB; }
		constexpr static size_t kilobyte() { return Count * Unit / KB;  }
		constexpr static size_t mebibyte() { return Count * Unit / MiB; }
		constexpr static size_t megabyte() { return Count * Unit / MB;  }
		constexpr static size_t gibibyte() { return Count * Unit / GiB; }
		constexpr static size_t gigabyte() { return Count * Unit / GB;  }
	};

	constexpr size_t default_memory_size = MemoryUnit<DEFAULT_MEMORY_SIZE>::byte();
	constexpr size_t meta_memory_size = MemoryUnit<META_MEMORY_SIZE>::byte();
	constexpr size_t default_pool_h = sizeof(double);

	class MemoryLinear
	{
	private:
		const size_t memory_size;
		void* next_alloc;
		size_t busy;
	public:
		MemoryLinear(void* p, size_t count) noexcept
			: memory_size(count), next_alloc(p), busy(0) {}

		void* allocate(size_t n) 
		{
			if (busy + n <= memory_size)
			{
				void* rs = next_alloc;
				next_alloc = reinterpret_cast<char*>(next_alloc) + n;
				busy += n;
				return rs;
			}
			else
				throw std::bad_alloc();
		}

		void deallocate(void*, size_t) const noexcept
		{
			return;
		}

		bool equal(const MemoryLinear& other) const noexcept
		{
			return memory_size == other.memory_size
				&& next_alloc == other.next_alloc && busy == other.busy;
		}
	};

	class MemoryStack
	{
	private:
		struct node
		{
			void* loc;
			size_t n;

			node(void* loc, size_t n)
				: loc(loc), n(n) {}
		};

		node* stack;
		size_t bias;
		const size_t memory_size;
		size_t stack_size;
		void* next_alloc;
		size_t busy;
	public:
		MemoryStack(void* p, size_t count)
			: memory_size(count), stack_size(0), next_alloc(p), busy(0) 
		{
			stack = reinterpret_cast<node*>(::operator new(meta_memory_size));
			bias = alignof(node) - reinterpret_cast<long long>(stack) % alignof(node);
			bias = bias == alignof(node) ? 0 : bias;
			stack = reinterpret_cast<node*>(reinterpret_cast<char*>(stack) + bias);
		}

		void* allocate(size_t n) 
		{
			if (busy + n <= memory_size)
			{
				void* rs = next_alloc;
				stack[stack_size] = node(rs, n);
				++stack_size;
				next_alloc = reinterpret_cast<char*>(rs) + n;
				busy += n;
				return rs;
			}
			else
				throw std::bad_alloc();
		}

		void deallocate(void*, size_t) 
		{
			if (stack_size > 0)
			{
				next_alloc = stack[stack_size - 1].loc;
				busy -= stack[stack_size - 1].n;
				--stack_size;
			}
			else
				throw std::bad_alloc();
		}

		bool equal(const MemoryStack& other) const noexcept
		{
			return stack == other.stack && memory_size == other.memory_size
				&& stack_size == other.stack_size && next_alloc == other.next_alloc
				&& busy == other.busy && bias == other.bias;
		}

		~MemoryStack()
		{
			stack = reinterpret_cast<node*>(reinterpret_cast<char*>(stack) - bias);
			delete[] stack;
		}
	};

	class MemoryPool
	{
	private:
		struct node
		{
			void* loc;
			bool lock;
			size_t next_alloc_index;

			node(void* loc, bool lock, size_t next_alloc_index)
				: loc(loc), lock(lock), next_alloc_index(next_alloc_index) {}
		};

		node* pool;
		size_t pool_size;
		size_t bias;
		const size_t memory_size;
		size_t next_alloc_index;
		size_t busy;
		const size_t h;
	public:
		MemoryPool(void* p, size_t count, size_t pool_h)
			: memory_size(count), pool_size(1), next_alloc_index(0), busy(0), h(pool_h)
		{
			pool = reinterpret_cast<node*>(::operator new(meta_memory_size));
			bias = alignof(node) - reinterpret_cast<long long>(pool) % alignof(node);
			bias = bias == alignof(node) ? 0 : bias;
			pool = reinterpret_cast<node*>(reinterpret_cast<char*>(pool) + bias);
			pool[0] = node(p, false, meta_memory_size);
		}

		void* allocate(size_t)
		{
			if (busy + h <= memory_size)
			{
				node& rs = pool[next_alloc_index];
				rs.lock = true;
				if (rs.next_alloc_index == meta_memory_size)
				{
					rs.next_alloc_index = pool_size;
					pool[pool_size] = node(reinterpret_cast<char*>(pool[0].loc)
						+ h * pool_size, false, meta_memory_size);
					next_alloc_index = pool_size;
					++pool_size;
				}
				else
					next_alloc_index = rs.next_alloc_index;
				busy += h;
				return rs.loc;
			}
			else
				throw std::bad_alloc();
		}

		void deallocate(void* p, size_t)
		{
			void* begin = pool[0].loc;
			long long diff = reinterpret_cast<char*>(p) - reinterpret_cast<char*>(begin);
			if (diff < 0 || diff > memory_size || diff % h != 0)
				throw std::bad_alloc();
			size_t index = diff / h;
			pool[index].lock = false;
			pool[index].next_alloc_index = next_alloc_index;
			next_alloc_index = index;
			busy -= h;
		}

		bool equal(const MemoryPool& other) const noexcept
		{
			return pool == other.pool && pool_size == other.pool_size
				&& bias == other.bias && memory_size == other.memory_size
				&& next_alloc_index == other.next_alloc_index
				&& busy == other.busy && h == other.h;
		}

		~MemoryPool()
		{
			pool = reinterpret_cast<node*>(reinterpret_cast<char*>(pool) - bias);
			delete[] pool;
		}
	};

	template <typename Type,
		size_t Count = default_memory_size,
		typename MemoryType = MemoryLinear, loc_t Loc = HEAP
	> class Allocator
	{
	private:
		Type* memory;
		MemoryType locations;
	public:
		using value_type = Type;

		Allocator() : memory(reinterpret_cast<Type*>(::operator new(Count / sizeof(Type)))),
			locations(memory, Count) {}

		template<typename Rebind>
		Allocator(const Allocator<Rebind, Count, MemoryType, Loc>& other) noexcept {}

		[[nodiscard]] Type* allocate(size_t n)
		{
			return reinterpret_cast<Type*>(locations.allocate(n * sizeof(Type)));
		}

		void deallocate(Type* p, size_t n)
		{
			locations.deallocate(p, n * sizeof(Type));
		}

		bool equal(const Allocator<Type, Count, MemoryType, Loc>& other) const noexcept
		{
			return locations.equal(other.locations);
		}

		~Allocator()
		{
			delete[] memory;
		}
	};

	template <typename Type, size_t Count, typename MemoryType>
	class Allocator<Type, Count, MemoryType, STACK>
	{
	private:
		Type memory[Count];
		MemoryType locations;
	public:
		using value_type = Type;

		Allocator() : locations(memory, Count) {}

		template<typename Rebind>
		Allocator(const Allocator<Rebind, Count, MemoryType, STACK>& other) noexcept {}

		[[nodiscard]] Type* allocate(size_t n)
		{
			return reinterpret_cast<Type*>(locations.allocate(n * sizeof(Type)));
		}

		void deallocate(Type* p, size_t n)
		{
			locations.deallocate(p, n * sizeof(Type));
		}

		bool equal(const Allocator<Type, Count, MemoryType, STACK>& other) const noexcept
		{
			return locations.equal(other.locations);
		}
	};

	template <typename Type, size_t Count, loc_t Loc>
	class Allocator<Type, Count, MemoryPool, Loc>
	{
	private:
		Type* memory;
		MemoryPool locations;
	public:
		using value_type = Type;

		Allocator(size_t pool_h = default_pool_h) 
			: memory(reinterpret_cast<Type*>(::operator new(Count / sizeof(Type)))),
			locations(memory, Count, pool_h) {}

		template<typename Rebind>
		Allocator(const Allocator<Rebind, Count, MemoryPool, Loc>& other) noexcept {}

		[[nodiscard]] Type* allocate(size_t n)
		{
			return reinterpret_cast<Type*>(locations.allocate(n * sizeof(Type)));
		}

		void deallocate(Type* p, size_t n)
		{
			locations.deallocate(p, n * sizeof(Type));
		}

		bool equal(const Allocator<Type, Count, MemoryPool, Loc>& other) const noexcept
		{
			return locations.equal(other.locations);
		}
	};

	template <typename Type, size_t Count>
	class Allocator<Type, Count, MemoryPool, STACK>
	{
	private:
		Type memory[Count];
		MemoryPool locations;
	public:
		using value_type = Type;

		Allocator(size_t pool_h = default_pool_h) : locations(memory, Count, pool_h) {}

		template<typename Rebind>
		Allocator(const Allocator<Rebind, Count, MemoryPool, STACK>& other) noexcept {}

		[[nodiscard]] Type* allocate(size_t n)
		{
			return reinterpret_cast<Type*>(locations.allocate(n * sizeof(Type)));
		}

		void deallocate(Type* p, size_t n)
		{
			locations.deallocate(p, n * sizeof(Type));
		}

		bool equal(const Allocator<Type, Count, MemoryPool, STACK>& other) const noexcept
		{
			return locations.equal(other.locations);
		}
	};

	template <
		typename Type_A, typename Type_B,
		size_t Count_A, size_t Count_B,
		typename MemoryType_A, typename MemoryType_B,
		loc_t Loc_A, loc_t Loc_B
	> constexpr bool operator==(const Allocator<Type_A, Count_A, MemoryType_A, Loc_A>& Allocator_A,
		const Allocator<Type_B, Count_B, MemoryType_B, Loc_B>& Allocator_B) noexcept
	{
		return false;
	}

	template <
		typename Type_A, typename Type_B,
		size_t Count_A, size_t Count_B,
		typename MemoryType_A, typename MemoryType_B,
		loc_t Loc_A, loc_t Loc_B
	> constexpr bool operator!=(const Allocator<Type_A, Count_A, MemoryType_A, Loc_A>& Allocator_A,
		const Allocator<Type_B, Count_B, MemoryType_B, Loc_B>& Allocator_B) noexcept
	{
		return true;
	}

	template < typename Type, typename MemoryType_A, size_t Count, typename MemoryType, loc_t Loc>
	bool operator==(const Allocator<Type, Count, MemoryType, Loc>& Allocator_A,
		const Allocator<Type, Count, MemoryType, Loc>& Allocator_B) noexcept
	{
		return Allocator_A.equal(Allocator_B);
	}

	template < typename Type, typename MemoryType_A, size_t Count, typename MemoryType, loc_t Loc>
	bool operator!=(const Allocator<Type, Count, MemoryType,  Loc>& Allocator_A,
		const Allocator<Type, Count, MemoryType, Loc>& Allocator_B) noexcept
	{
		return !Allocator_A.equal(Allocator_B);
	}
}

#endif
