#pragma once

// Similar to std::span in C++20 (I think)

#include "BetterAssert.h"
#include <cstdint>
#include <vector>
#include <array>
//#include <string_view>

// This is an abstraction over a raw pointer.
// Use the same amount of caution as you would with any raw pointer

template<typename T, typename U = uint32_t>
class ArrayPointer {
	T * p = nullptr;
	U n = 0;

public:

	ArrayPointer() {}

	ArrayPointer(T* p_, U n_) : p(p_), n(n_) {
	}

	template<typename X>
	ArrayPointer(ArrayPointer<X, U> const& other) {
		assert__((other.size()*sizeof(X)) % sizeof(T) == 0, "Invalid length");
		p = reinterpret_cast<T*>(other.get());
		n = (other.size()*sizeof(X)) / sizeof(T);
	}

	// ArrayPointer(ArrayPointer<T, U> const& ap) 
	// 	: p(ap.p), n(ap.n)
	// {}


	// ArrayPointer(std::vector<std::remove_const<>::type> const& vec) {
	// 	p = vec.data();
	// 	n = vec.size();
	// }

	template <std::size_t N>
	ArrayPointer(std::array<T, N> & arr) {
		p = arr.data();
		n = arr.size();
	}

	#define TEST() assert__(p, "Dereferenced null pointer");

	T& operator [](int i) const {
		TEST()
		assert_(i >= 0 && static_cast<U>(i) < n);
		return p[i];
	}

	bool is_null() const {
		return p == nullptr;
	}
	bool is_valid() const {
		return p != nullptr;
	}

	T* get() const {
		TEST()
		return p;
	}

	T* data() const {
		TEST()
		return p;
	}

	T* get_() const {
		return p;
	}

	U size() const {
		return n;
	}

	ArrayPointer<T, U> slice(U new_start, U new_size) {
		assert__(new_start + new_size <= size(), "Invalid slice");
		return ArrayPointer<T, U>(get()+new_start, new_size);
	}

	//std::basic_string_view<T> to_string_view() {
	//	
	//}
};