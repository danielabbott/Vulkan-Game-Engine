#pragma once

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	#ifdef ENGINE_INTERNAL
		#define EXPORT __declspec( dllexport )
	#else
		#define EXPORT
	#endif
#endif
