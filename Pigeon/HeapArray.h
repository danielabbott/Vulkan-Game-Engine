/* A std::vector that cannot be resized and does not initialise data */

#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include "BetterAssert.h"
#include "ArrayPointer.h"


template <typename T>
class HeapArray {
	T* ptr = nullptr;
	uintptr_t data_size = 0; // Number of elements

	inline static const std::string EXCEPTION_OUT_OF_BOUNDS = "Array index out of bounds";
public:
	HeapArray() {
	}
	HeapArray(uintptr_t n) {
		ptr = new T[n];
		if (!ptr) {
			throw std::runtime_error("Out of memory");
		}
		data_size = n;
	}

	// It is safe to call the destructor multiple times
	~HeapArray() {
		if (ptr) {
			delete[] ptr;
		}
		ptr = nullptr;
		data_size = 0;
	}

	T* get() const {
		return ptr;
	}

	T* data() const {
		return ptr;
	}

	T& at(uintptr_t i) {
		assert__(i < data_size, EXCEPTION_OUT_OF_BOUNDS);
		return ptr[i];
	}

	T const& at(uintptr_t i) const {
		assert__(i < data_size, EXCEPTION_OUT_OF_BOUNDS);
		return ptr[i];
	}

	uintptr_t size() const {
		return data_size;
	}

	uintptr_t count() const {
		return data_size;
	}

	uintptr_t length() const {
		return data_size;
	}

	// THE ArrayPointer WILL BE INVALID WHEN THIS OBJECT IS DESTROYED
	// DO NOT CALL THIS ON R VALUES
	template <typename U= uint32_t>
	ArrayPointer<T, U> get_array_ptr() const {
		assert__(size() <= std::numeric_limits<U>::max(), "Size too large for given integer type");
		return ArrayPointer<T, U>(data(), static_cast<U>(size()));
	}

	// THE ArrayPointer WILL BE INVALID WHEN THIS OBJECT IS DESTROYED
	// DO NOT CALL THIS ON R VALUES
	template <typename U = uint32_t>
	ArrayPointer<T, U> slice(U o, U n) const {
		assert__(o+n <= data_size, "Out of bounds");
		return ArrayPointer<T, U>(data() + o, n);
	}


	HeapArray(const HeapArray&) = delete;
	HeapArray& operator=(const HeapArray&) = delete;

	void do_move(HeapArray&& other) noexcept {
		ptr = other.ptr;
		data_size = other.data_size;

		other.ptr = nullptr;
		other.data_size = 0;
	}

	HeapArray(HeapArray&& other) noexcept {
		do_move(std::move(other));
	}
	HeapArray& operator=(HeapArray&& other) noexcept {
		do_move(std::move(other));
		return *this;
	}
};