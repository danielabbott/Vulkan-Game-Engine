#pragma once

#include <vector>
#include <array>
#include <memory>
#include <cstring>
#include <new>
#include <functional>
#include <string.h>
#include "BetterAssert.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

inline static unsigned int find_first_zero_bit(uint64_t bits)
{
	unsigned long i;
	unsigned char success = _BitScanForward64(&i, ~bits);
	return success ? i : -1;
}

#else
// Untested.
#include <string.h>
inline static int find_first_zero_bit(uint64_t bits)
{
	int i = ffsll(~bits);
	return i ? (i - 1)  : -1;
}
#endif


template <typename T, unsigned int GroupSizeDiv64=2>
class ObjectPool {
#define GroupSize (GroupSizeDiv64 * 64)

	struct Group {
		uint8_t* elements_buffer_malloc_ptr;
		uint8_t* elements_buffer;
		std::array<uint64_t, GroupSizeDiv64> bitmap;

		constexpr static unsigned int padded_object_size() {
			if (sizeof(T) % std::alignment_of_v<T>) {
				return sizeof(T) + (sizeof(T) - (sizeof(T) % std::alignment_of_v<T>));
			}
			return sizeof(T);
		}

		Group() {
			elements_buffer_malloc_ptr = reinterpret_cast<uint8_t *>(malloc(GroupSize * padded_object_size() + std::alignment_of_v<T>));
			assert__(elements_buffer_malloc_ptr, "Out of memory");

			elements_buffer = elements_buffer_malloc_ptr +
				(std::alignment_of_v<T> -(reinterpret_cast<uintptr_t>(elements_buffer_malloc_ptr) % std::alignment_of_v<T>));
			memset(bitmap.data(), 0, bitmap.size()*8);
		}
		~Group() {
			::free(elements_buffer_malloc_ptr);
		}

		Group(const Group&) = delete;
		Group& operator=(const Group&) = delete;

		void do_move(Group&& other) noexcept {
			elements_buffer_malloc_ptr = other.elements_buffer_malloc_ptr;
			elements_buffer = other.elements_buffer;
			memcpy(bitmap.data(), other.bitmap.data(), bitmap.size() * 8);
			other.elements_buffer_malloc_ptr = nullptr;
			other.elements_buffer = 0;
		}

		Group(Group&& other) noexcept
		{
			do_move(std::move(other));
		}
		Group& operator=(Group&& other) noexcept {
			do_move(std::move(other));
			return *this;
		}

		bool is_empty() {
			for (uint64_t bits : bitmap) {
				if (bits) return false;
			}
			return true;
		}

		bool is_full() {
			for (uint64_t bits : bitmap) {
				if (bits != UINT64_MAX) return false;
			}
			return true;
		}

		// Assumes is_full() == false
		T* get() {
			unsigned int i = 0;
			for (uint64_t& bits : bitmap) {
				if (bits == UINT64_MAX) { 
					i += 64;
					continue; 
				}

				unsigned int j = find_first_zero_bit(bits);

				bits |= (1ull << j);

				i += j;

				assert(elements_buffer + (i+1) * padded_object_size() <= elements_buffer_malloc_ptr+ GroupSize * padded_object_size() + std::alignment_of_v<T>);

				return reinterpret_cast<T*>(elements_buffer + i*padded_object_size());
			}
			//unreachable
			assert_(false);
			return nullptr;
		}

		// Returns true if object was in this group
		bool free_if_contains(T* o) {
			auto o_byte_ptr = reinterpret_cast<uint8_t*>(o);
			if (o_byte_ptr < elements_buffer) return false;
			if (o_byte_ptr > elements_buffer + padded_object_size() * (GroupSize-1)) return false;

			unsigned int i = (o_byte_ptr - elements_buffer) / padded_object_size();
			assert(bitmap[i / 64] & (1ull << (i % 64)));
			bitmap[i / 64] &= ~(1ull << (i % 64));
			return true;
		}
	};

	std::vector<Group> groups;

public:

	T* allocate_no_construct() {
		for (auto& group : groups) {
			if (!group.is_full()) {
				return group.get();
			}
		}

		auto& g = groups.emplace_back();

		g.bitmap[0] = 1;

		return reinterpret_cast<T *>(g.elements_buffer);
	}

	void free_no_destruct(T* o)
	{
		unsigned int empty_groups = 0;
		for (auto& group : groups) {
			group.free_if_contains(o);
			if (group.is_empty()) {
				empty_groups++;
			}
		}

		if (empty_groups >= 2) {
			unsigned int to_erase = empty_groups - 1;
			unsigned int erased = 0;
			for (auto i = groups.size(); i > 0 && erased < to_erase; i--) {
				if (groups[i-1].is_empty()) {
					groups.erase(groups.begin() + (i - 1));
					erased++;
				}
			}
		}
	}

	template <class... Types>
	T* allocate(Types&&... args)
	{		
		void* ptr = allocate_no_construct();

		T* obj = new (ptr) T(std::forward<Types>(args)...);
		
		return obj;
	}

	void free(T* o)
	{
		o->~T();
		free_no_destruct(o);
	}



	template <class... Types>
	std::shared_ptr<T> allocate_shared(Types&&... args) {
		T* ptr = allocate(std::forward<Types>(args)...);
		return std::shared_ptr<T>(ptr, [this](T* o) {
			free(o);
		});
	}

	using pool_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

	template <class... Types>
	pool_unique_ptr allocate_unique(Types&&... args) {
		T* ptr = allocate(std::forward<Types>(args)...);
		return pool_unique_ptr(ptr, [this](T* o) {
			free(o);
		});
	}

};
