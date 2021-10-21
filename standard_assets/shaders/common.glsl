#ifndef _COMMON_GLSL_
#define _COMMON_GLSL_


#if __VERSION__ >= 460

    #define LOCATION(i) layout(location = i)
    #define BINDING(i) layout(binding = i)

#else

    #define LOCATION(i)
    #define BINDING(i)

#endif


#endif