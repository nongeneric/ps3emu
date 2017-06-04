#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

template<typename Type> inline Type CLAMP_MIN(Type x, Type min){ return (x > min) ? x : min; }
template<typename Type> inline Type CLAMP_MAX(Type x, Type max){ return (x < max) ? x : max; }
template<typename Type> inline Type CLAMP(Type x, Type min, Type max)
{
	return (x < max) ? ((x > min) ? x : min) : max;
}

inline uint32_t ALIGN(uint32_t x, uint32_t align){ return (x + (align -1)) & ~(align -1); }

#define PI 3.141592653589793238468327959288f
inline float DEG2RAD(float deg){ return PI * (1.0f / 180.0f) * deg; }

#endif // COMMON_UTIL_H