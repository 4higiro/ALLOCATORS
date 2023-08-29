#ifndef ALLOC_TYPE
#define ALLOC_TYPE

#include <type_traits>
#include <iostream>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <scoped_allocator>

namespace alloc
{
	template<typename x_Class, typename x_Enabled = void>
	class has_value_type final : public ::std::false_type {};

	template<typename x_Class>
	class has_value_type<x_Class, ::std::void_t<typename x_Class::value_type>> final : public ::std::true_type {};

	template <typename Type>
	class Allocator;

	class MemoryResource;

	template <template <typename Type> class Cont, typename Type>
	using scoped = std::scoped_allocator_adaptor<Allocator<Cont<Type>>>;

	enum bad_t
	{
		RESOURCE_OVERFLOW			= 0,
		MANAGER_MEMORY_OUT_OF_RANGE = 1,
		EMPTY_STACK					= 2,
		DISPOS_PTR					= 3,
		RESOURCE_NOT_INSTANCE		= 4,
		UNCORRECT_MANAGER_TYPE		= 5
	};

	struct bad_except
	{
		bad_t id;

		bad_except(bad_t id) noexcept : id(id) {}

		const char* what() const noexcept
		{
			switch (id)
			{
			case RESOURCE_OVERFLOW:
				return "RESOURCE OVERFLOW";
				break;
			case MANAGER_MEMORY_OUT_OF_RANGE:
				return "MEMORY MANAGER NOT INSTANCE: MEMORY OUT";
				break;
			case EMPTY_STACK:
				return "TRY DEALLOCATE AT EMPTY STACK";
				break;
			case DISPOS_PTR:
				return "ADRESS FROM NOT THIS POOL";
				break;
			case RESOURCE_NOT_INSTANCE:
				return "RESOURCE NOT INSTANCE: MEMEORY OUT";
				break;
			case UNCORRECT_MANAGER_TYPE:
				return "RESOURCE NOT KNOW OF MANAGER TYPE ARGUMENT";
				break;
			default:
				return "UNDEFINED EXCEPTION";
				break;
			}
		}

		const wchar_t* lwhat() const noexcept
		{
			switch (id)
			{
			case RESOURCE_OVERFLOW:
				return L"RESOURCE OVERFLOW";
				break;
			case MANAGER_MEMORY_OUT_OF_RANGE:
				return L"MEMORY MANAGER NOT INSTANCE: MEMORY OUT";
				break;
			case EMPTY_STACK:
				return L"TRY DEALLOCATE AT EMPTY STACK";
				break;
			case DISPOS_PTR:
				return L"ADRESS FROM NOT THIS POOL";
				break;
			case RESOURCE_NOT_INSTANCE:
				return L"RESOURCE NOT INSTANCE: MEMEORY OUT";
				break;
			case UNCORRECT_MANAGER_TYPE:
				return L"RESOURCE NOT KNOW OF MANAGER TYPE ARGUMENT";
				break;
			default:
				return L"UNDEFINED EXCEPTION";
				break;
			}
		}
	};

	struct bad_alloc : bad_except
	{
		bad_alloc(bad_t id) noexcept : bad_except(id) {}
	};

	struct bad_dealloc : bad_except
	{
		bad_dealloc(bad_t id) noexcept : bad_except(id) {}
	};

	struct bad_resource : bad_except
	{
		bad_resource(bad_t id) noexcept : bad_except(id) {}
	};

	template <typename Type>
	using vector = std::vector<Type, Allocator<Type>>;

	template <typename Type>
	using deque = std::deque<Type, Allocator<Type>>;

	template <typename Type>
	using list = std::list<Type, Allocator<Type>>;

	template <typename Type>
	using forward_list = std::forward_list<Type, Allocator<Type>>;

	template <typename Type>
	using set = std::set<Type, std::less<Type>, Allocator<Type>>;

	template <typename Type>
	using multiset = std::multiset<Type, std::less<Type>, Allocator<Type>>;

	template <typename KeyType, typename ValueType>
	using map = std::map<KeyType, ValueType,
		std::less<KeyType>, Allocator<std::pair<const KeyType, ValueType>>>;

	template <typename KeyType, typename ValueType>
	using multimap = std::multimap<KeyType, ValueType,
		std::less<KeyType>, Allocator<std::pair<const KeyType, ValueType>>>;

	template <typename Type>
	using unordered_set = std::unordered_set<Type,
		std::hash<Type>, std::equal_to<Type>, Allocator<Type>>;

	template <typename KeyType, typename ValueType>
	using unordered_map = std::unordered_map<KeyType, ValueType,
		std::hash<KeyType>, std::equal_to<KeyType>, Allocator<std::pair<const KeyType, ValueType>>>;

	template <typename Type>
	using unordered_multiset = std::unordered_multiset<Type,
		std::hash<Type>, std::equal_to<Type>, Allocator<Type>>;

	template <typename KeyType, typename ValueType>
	using unordered_multimap = std::unordered_multimap<KeyType, ValueType,
		std::hash<KeyType>, std::equal_to<KeyType>, Allocator<std::pair<const KeyType, ValueType>>>;

	using string = std::basic_string<char, std::char_traits<char>, Allocator<char>>;

	template <typename ...Args>
	std::shared_ptr<MemoryResource> make_resource(Args&&... args)
	{
		return std::make_shared<MemoryResource>(args...);
	}

	struct type_info_t
	{
		size_t size;
		size_t align;

		type_info_t(size_t size, size_t align)
			: size(size), align(align) {}

		bool operator==(const type_info_t& other)
		{
			return size == other.size && align == other.align;
		}

		bool operator!=(const type_info_t& other)
		{
			return size != other.size || align != other.align;
		}
	};

	enum memory_unit_t
	{
		BT  = 1,
		KiB = 1'024,
		KB  = 1'000,
		MiB = 1'048'576,
		MB  = 1'000'000,
		GiB = 1'073'741'824,
		GB  = 1'000'000'000
	};

	enum manager_t
	{
		HEAP   = 0,
		LINEAR = 1,
		STACK  = 2,
		POOL   = 3
	};
}

using byte_t = unsigned char;

std::ostream& operator<<(std::ostream& stream, const byte_t& num)
{
	stream << static_cast<short>(num);
	return stream;
}

#endif
