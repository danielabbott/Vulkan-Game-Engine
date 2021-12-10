#pragma once

#include <cassert>
#include <stdexcept>


#ifndef NDEBUG
#define assert__(x, s) assert(x)
#define assert_(x) assert(x)
#else
#define assert__(x, s) if (!(x)) throw std::runtime_error(s);
#define assert_(x) if (!(x)) throw std::runtime_error("Unexpected error");
#endif

#ifndef NDEBUG
#define panic__(s) assert(false)
#define panic_() assert(false)
#else
#define panic__(s) throw std::runtime_error(s);
#define panic_() throw std::runtime_error("Unexpected state");
#endif
