#ifndef SPU_COMMON_SPU_UTIL_H
#define SPU_COMMON_SPU_UTIL_H

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#define LIKELY(x)	(__builtin_expect(!!(x), 1))
#define UNLIKELY(x)	(__builtin_expect(!!(x), 0))
#define TAG_BIT(x) (1 << (x))

template<typename Type, int Size=sizeof(Type)> class SpuBuffer{
public:
	inline Type* getPtr() { 
		union{ char* c; Type* t; } u;
		u.c = buffer;
		return u.t; 
	}
	inline void* getLsAddress() { return buffer; }
	inline size_t getSize() { return Size; }
private:
	char buffer[Size];
};

#endif // SPU_COMMON_UTIL_H