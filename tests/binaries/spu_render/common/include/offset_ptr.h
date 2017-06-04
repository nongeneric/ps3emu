#ifndef COMMON_OFFSETPTR_H
#define COMMON_OFFSETPTR_H

template < class Type >
class OffsetPtr
{
	static const int32_t INVALID_OFFSETPTR	= 0;
public:
	inline Type* operator->() const { return( getPtr() ); }
	inline operator Type*() const { return( getPtr() ); }
	inline Type& operator[] (int i) const { return *(getPtr() + i);} 
	inline Type* getPtr() const
	{
		return reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(&(m_offset)) + m_offset );
	}
	inline Type* getPtrZeroChk() const
	{
		return	(INVALID_OFFSETPTR == m_offset) ? 0 
			: reinterpret_cast<Type*> (reinterpret_cast<uintptr_t>(&(m_offset)) + m_offset );
	}
	inline void setup( const Type* ptr )
	{
		m_offset = (ptr == 0) ? INVALID_OFFSETPTR
			: static_cast<int32_t>(reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(this));
	}
	inline int32_t getOffset(){ return m_offset; }
private:
	int32_t m_offset;
};

#endif // COMMON_OFFSETPTR_H