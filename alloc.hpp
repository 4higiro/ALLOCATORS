#ifndef CUSTOM_ALLOCATORS
#define CUSTOM_ALLOCATORS

#ifndef DEFAULT_MEMORY_SIZE 
#define DEFAULT_MEMORY_SIZE 20, KiB
#endif

#include <memory>
#include "type.hpp"

namespace alloc
{
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
	constexpr size_t default_pool_h = sizeof(long double);
	size_t meta_memory_size = default_memory_size;

	class IMemoryArray
	{
	public:
		virtual void* allocate(size_t) = 0;
		virtual void deallocate(void*, size_t) = 0;

		virtual ~IMemoryArray() {};
	};

	class MemoryHeap : public IMemoryArray
	{
	public:
		void* allocate(size_t n) override
		{
			return ::operator new(n);
		}

		void deallocate(void* p, size_t) override
		{
			::operator delete(p);
		}
	};

	class MemoryLinear : public IMemoryArray
	{
	private:
		void* next_alloc;
		size_t busy;
	public:
		const size_t memory_size;

		MemoryLinear(void* p, size_t count) noexcept
			: memory_size(count), next_alloc(p), busy(0) {}

		MemoryLinear(const MemoryLinear&) = delete;
		MemoryLinear& operator=(const MemoryLinear&) = delete;

		MemoryLinear(MemoryLinear&&) = delete;
		MemoryLinear& operator=(MemoryLinear&&) = delete;

		void* allocate(size_t n) override 
		{
			if (busy + n <= memory_size)
			{
				void* rs = next_alloc;
				next_alloc = reinterpret_cast<byte_t*>(next_alloc) + n;
				busy += n;
				return rs;
			}
			else
				throw bad_alloc(RESOURCE_OVERFLOW);
		}

		void deallocate(void*, size_t) override
		{
			return;
		}
	};

	class MemoryStack : public IMemoryArray
	{
	private:
		struct node
		{
			void* loc;
			size_t n;

			node(void* loc, size_t n) noexcept
				: loc(loc), n(n) {}
		};

		node* stack;
		size_t stack_size;
		void* next_alloc;
		size_t busy;
	public:
		const size_t memory_size;

		MemoryStack(void* p, size_t count)
			: memory_size(count), stack_size(0), next_alloc(p), busy(0) 
		{
			std::align_val_t align_val = std::align_val_t(alignof(node));
			stack = reinterpret_cast<node*>(::operator new(meta_memory_size, align_val, std::nothrow));
			if (stack == nullptr)
				throw alloc::bad_resource(MANAGER_MEMORY_OUT_OF_RANGE);
		}

		MemoryStack(const MemoryStack&) = delete;
		MemoryStack& operator=(const MemoryStack&) = delete;

		MemoryStack(MemoryStack&&) = delete;
		MemoryStack& operator=(MemoryStack&&) = delete;

		void* allocate(size_t n) override 
		{
			if (busy + n <= memory_size)
			{
				void* rs = next_alloc;
				stack[stack_size] = node(rs, n);
				++stack_size;
				next_alloc = reinterpret_cast<byte_t*>(rs) + n;
				busy += n;
				return rs;
			}
			else
				throw bad_alloc(RESOURCE_OVERFLOW);
		}

		void deallocate(void*, size_t) override
		{
			if (stack_size > 0)
			{
				next_alloc = stack[stack_size - 1].loc;
				busy -= stack[stack_size - 1].n;
				--stack_size;
			}
			else
				throw bad_dealloc(EMPTY_STACK);
		}

		~MemoryStack() override
		{
			if (stack != nullptr)
			{
				std::align_val_t align_val = std::align_val_t(alignof(node));
				::operator delete (stack, align_val, std::nothrow);
			}
		}
	};

	class MemoryPool : public IMemoryArray
	{
	private:
		struct node
		{
			void* loc;
			bool lock;
			size_t next_alloc_index;

			node(void* loc, bool lock, size_t next_alloc_index) noexcept
				: loc(loc), lock(lock), next_alloc_index(next_alloc_index) {}
		};

		node* pool;
		size_t pool_size;
		size_t next_alloc_index;
		size_t busy;
		void* memory_begin;
	public:
		const size_t memory_size;
		const size_t h;

		MemoryPool(void* p, size_t count, size_t pool_h)
			: memory_size(count), pool_size(1), next_alloc_index(0), busy(0), h(pool_h), memory_begin(p)
		{
			std::align_val_t align_val = std::align_val_t(alignof(node));
			pool = reinterpret_cast<node*>(::operator new(meta_memory_size, align_val, std::nothrow));
			if (pool == nullptr)
				throw bad_resource(MANAGER_MEMORY_OUT_OF_RANGE);
			pool[0] = node(p, false, meta_memory_size);
		}

		MemoryPool(const MemoryPool&) = delete;
		MemoryPool& operator=(const MemoryPool&) = delete;

		MemoryPool(MemoryPool&&) = delete;
		MemoryPool& operator=(MemoryPool&&) = delete;

		void* allocate(size_t) override
		{
			if (busy + h <= memory_size)
			{
				node& rs = pool[next_alloc_index];
				rs.lock = true;
				if (rs.next_alloc_index == meta_memory_size)
				{
					rs.next_alloc_index = pool_size;
					pool[pool_size] = node(reinterpret_cast<byte_t*>(memory_begin)
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
				throw bad_alloc(RESOURCE_OVERFLOW);
		}

		void deallocate(void* p, size_t) override
		{
			long long diff = reinterpret_cast<byte_t*>(p) - reinterpret_cast<byte_t*>(memory_begin);
			if (diff < 0 || diff > memory_size || diff % h != 0)
				throw bad_dealloc(DISPOS_PTR);
			size_t index = diff / h;
			pool[index].lock = false;
			pool[index].next_alloc_index = next_alloc_index;
			next_alloc_index = index;
			busy -= h;
		}

		~MemoryPool() override
		{
			if (pool != nullptr)
			{
				std::align_val_t align_val = std::align_val_t(alignof(node));
				::operator delete (pool, align_val, std::nothrow);
			}
		}
	};

	class MemoryResource
	{
	private:
		byte_t* memory;
		IMemoryArray* resource;
		bool free_state = false;

		void resource_alloc()
		{
			free_state = true;
			if (mm_type != HEAP)
			{
				std::align_val_t align_val = std::align_val_t(type_info.align);
				memory = reinterpret_cast<byte_t*>(::operator new(memory_size, align_val, std::nothrow));
				if (memory == nullptr)
					throw bad_resource(RESOURCE_NOT_INSTANCE);
			}
			switch (mm_type)
			{
			case HEAP:
				resource = new MemoryHeap();
				break;
			case LINEAR:
				resource = new MemoryLinear(memory, memory_size);
				break;
			case STACK:
				resource = new MemoryStack(memory, memory_size);
				break;
			case POOL:
				resource = new MemoryPool(memory, memory_size, pool_h);
				break;
			default:
				throw bad_resource(UNCORRECT_MANAGER_TYPE);
				break;
			}
		}

		friend class MemoryResource;
	public:
		const size_t memory_size;
		const size_t pool_h;
		const manager_t mm_type;
		const type_info_t type_info;
		const bool copy_assignment;

		MemoryResource(manager_t mm_type = HEAP,
			size_t align = alignof(std::max_align_t), size_t t_size = sizeof(std::max_align_t),
			bool copy_assignment = false, size_t n = default_memory_size, size_t h = default_pool_h) noexcept
			: memory_size(n), mm_type(mm_type), type_info({ t_size, align }), 
			copy_assignment(copy_assignment), pool_h(h) {}

		MemoryResource(const MemoryResource&) = delete;
		MemoryResource& operator=(const MemoryResource&) = delete;

		MemoryResource(MemoryResource&& other) noexcept
			: memory(other.memory), resource(other.resource),
			free_state(other.free_state), memory_size(other.memory_size),
			pool_h(other.pool_h), mm_type(other.mm_type), type_info(other.type_info),
			copy_assignment(other.copy_assignment)
		{
			other.memory = nullptr;
			other.resource = nullptr;
			other.free_state = false;
		}

		MemoryResource& operator=(MemoryResource&&) = delete;

		void* allocate(size_t n)
		{
			if (!free_state)
				resource_alloc();
			return resource->allocate(n);
		}

		void deallocate(void* p, size_t n)
		{ 
			if (!free_state)
				throw bad_dealloc(RESOURCE_NOT_INSTANCE);
			resource->deallocate(p, n);
		}

		~MemoryResource()
		{
			if (free_state && mm_type != HEAP && memory != nullptr)
			{
				free_state = false;
				std::align_val_t align_val = std::align_val_t(type_info.align);
				::operator delete(memory, align_val, std::nothrow);
				delete resource;
			}
		}
	};

	template <typename Type>
	class Allocator
	{
	private:
		std::shared_ptr<MemoryResource> resource;

		mutable std::shared_ptr<MemoryResource> other_resources[5];
		mutable size_t other_resources_count;
		mutable void* parent;

		friend class Allocator;
	public:
		using value_type = Type;

		Allocator() noexcept 
			: resource(make_resource()), parent(nullptr),
			other_resources(), other_resources_count(0) {}

		explicit Allocator(std::shared_ptr<MemoryResource> location) noexcept 
			: resource(location), parent(nullptr), other_resources(), other_resources_count(0) {}

		template <typename Rebind>
		Allocator(const Allocator<Rebind>& other)
			: other_resources(), other_resources_count(0)
		{
			parent = (void*)&other;
			Allocator<byte_t>* current = reinterpret_cast<Allocator<byte_t>*>(parent);
			while (current->parent != nullptr)
				current = reinterpret_cast<Allocator<byte_t>*>(current->parent);
			type_info_t rebind = { sizeof(Type), alignof(Type) };
			if (rebind == current->resource->type_info && false)
				resource = current->resource;
			else
			{
				for (size_t i = 0; i < current->other_resources_count; ++i)
				{
					if (rebind == current->other_resources[i]->type_info)
					{
						resource = current->other_resources[i];
						return;
					}
				}
				if constexpr (has_value_type<Type>::value)
					current->other_resources[current->other_resources_count] = make_resource
					(other.resource->mm_type, alignof(Type), sizeof(Type),
					false, other.resource->memory_size, sizeof(Type));
				else
					current->other_resources[current->other_resources_count] = make_resource
					(other.resource->mm_type, alignof(Type), sizeof(Type),
					false, meta_memory_size, sizeof(Type));
				resource = current->other_resources[current->other_resources_count];
				current->other_resources_count += 1;
			}
		}

		Allocator(const Allocator<Type>& other)
			: parent(nullptr), other_resources(), other_resources_count(0)
		{
			if (other.resource->copy_assignment)
				resource = other.resource;
			else
				resource = make_resource(other.resource->mm_type, alignof(Type), sizeof(Type),
					false, other.resource->memory_size, other.resource->pool_h);
		}

		Allocator<Type>& operator=(const Allocator<Type>& other)
		{
			resource.reset();
			for (size_t i = 0; i < other_resources_count; ++i)
				other_resources[i].reset();
			if (other.resource->copy_assignment)
				resource = other.resource;
			else
				resource = make_resource(other.resource->mm_type, alignof(Type), sizeof(Type),
					false, other.resource->memory_size, other.resource->pool_h);
			other_resources_count = 0;
			return *this;
		}

		Allocator(Allocator<Type>&& other)
			: parent(other.parent), other_resources_count(other.other_resources_count)
		{
			resource = other.resource;
			for (size_t i = 0; i < other_resources_count; ++i)
				other_resources[i] = other.other_resources[i];
		}

		Allocator<Type>& operator=(Allocator<Type>&& other)
		{
			resource.reset();
			for (size_t i = 0; i < other_resources_count; ++i)
				other_resources[i].reset();
			resource = other.resource;
			other_resources_count = other.other_resources_count;
			for (size_t i = 0; i < other_resources_count; ++i)
				other_resources[i] = other.other_resources[i];
			return *this;
		}

		Type* allocate(size_t n)
		{
			return reinterpret_cast<Type*>(resource->allocate(n * sizeof(Type)));
		}

		void deallocate(Type* p, size_t n)
		{
			resource->deallocate(p, n * sizeof(Type));
		}

		using propagate_on_container_copy_assignment = std::true_type;
		using propagate_on_container_move_assignment = std::true_type;
		using propagate_on_container_swap = std::true_type;

		constexpr Allocator<Type>& select_on_container_copy_construction(const Allocator<Type>&)
		{
			return *this;
		}

		template <typename OtherType>
		bool equal(const Allocator<OtherType>& other) const noexcept
		{
			return resource.get() == other.resource.get();
		}

		~Allocator() = default;
	};

	template<typename Type_A, typename Type_B>
	bool operator==(const Allocator<Type_A>& a, const Allocator<Type_B>& b)
	{
		return a.equal(b);
	}

	template<typename Type_A, typename Type_B>
	bool operator!=(const Allocator<Type_A>& a, const Allocator<Type_B>& b)
	{
		return !a.equal(b);
	}
}

#endif
