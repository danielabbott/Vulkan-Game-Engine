// Equivalent to std::shared_ptr
// This is deliberately __not__ thread-safe

#pragma once

#include <utility>
#include <string>
#include <cstddef>
#include <functional>
#include "BetterAssert.h"


template <typename T>
class Ref {
	struct Obj {
		void (*destructor_function)(T&);
		std::size_t i = 1;

		template <class... Args>
		Obj(void (*destructor_function_)(T&) , Args&&... args)
		: destructor_function(destructor_function_)
		{
			T * t = get_obj_ptr();
			new (t) T(std::forward<Args>(args)...);
		}

		~Obj() 
		{
			destructor_function(*get_obj_ptr());
		}

		T * get_obj_ptr()
		{
			return reinterpret_cast<T*>(this+1);
		}
	};

	Obj * object = nullptr;

	void nullify()
	{
		if(object) {
			assert_(object->i);
			object->i--;
			if(!object->i) {
				object->~Obj();
				free(object);
			}
			object = nullptr;
		}
	}
public:

	Ref(){}
	Ref(std::nullptr_t){}

	static void call_deconstructor(T & t)
	{
		t.~T();
	}
	
	template <class... Args>
	static Ref<T> make(Args&&... args) 
	{
		Ref<T> r;
		r.object = reinterpret_cast<Obj*>(malloc(sizeof(Obj) + sizeof(T)));
		assert__(r.object, "Memory allocation failure");
		new (r.object) Obj(call_deconstructor, std::forward<Args>(args)...);
		return r;
	}

	~Ref()
	{
		nullify();
	}

	auto ref_count() const
	{
		return object->i;
	}

	Ref<const T> as_const()
	{
		auto r = Ref<const T>();
		if(object) {
			object->i++;
			r->object = object;
		}
		return r;
	}

	static inline std::string DEREF_ERROR = "Dereferenced null pointer";

	T& operator*() const
	{
		assert__(object, DEREF_ERROR);
		return *object->get_obj_ptr();
	}
	T* operator->() const
	{
		assert__(object, DEREF_ERROR);
		return object->get_obj_ptr();
	}

	bool is_null() const
	{
		return object == nullptr;
	}
	bool is_valid() const
	{
		return object != nullptr;
	}

	operator bool() const 
	{ 
		return object != nullptr;
	}

	operator std::size_t() const 
	{ 
		assert__(object, DEREF_ERROR);
		return reinterpret_cast<std::size_t>(object->get_obj_ptr());
	}

	T* get() const
	{
		assert__(object, DEREF_ERROR);
		return object->get_obj_ptr();
	}

	Ref(const Ref<T>& other) 
	{
		if(other.object) {
			other.object->i++;
			object = other.object;
		}
	}

	Ref(Ref<T>& other) 
	{
		if(other.object) {
			other.object->i++;
			object = other.object;
		}
	}

	Ref& operator=(const Ref& other)
	{
		nullify();
		if(other.object) {
			other.object->i++;
			object = other.object;
		}
		return *this;
	}

	Ref& operator=(Ref& other)
	{
		nullify();
		if(other.object) {
			other.object->i++;
			object = other.object;
		}
		return *this;
	}

	Ref(Ref && other)
	{
		object = other.object;
		other.object = nullptr;
	}

	Ref& operator=(Ref && other)
	{
		nullify();
		object = other.object;
		other.object = nullptr;
		return *this;
	}

	bool operator==(Ref const& other) const
	{ 
		return object == other.object;
	}

	bool operator!=(Ref const& other) const
	{
		return object != other.object;
	}
	bool operator>(Ref const& other) const
	{
		return object > other.object;
	}
	bool operator<=(Ref const& other) const
	{
		return object <= other.object;
	}
	bool operator<(Ref const& other) const
	{
		return object < other.object;
	}
	bool operator>=(Ref const& other) const
	{
		return object >= other.object;
	}
};

template <typename T>
struct std::hash<Ref<T>>
{
	std::size_t operator()(const Ref<T>& k) const
	{
	  return std::hash<std::size_t>()(reinterpret_cast<std::size_t>(k.get()));
	}
};