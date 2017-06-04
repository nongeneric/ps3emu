#ifndef SPURS_TASK_RENDER_RENDER_STATE_CACHE
#define SPURS_TASK_RENDER_RENDER_STATE_CACHE

#define ENABLE_RENDER_STATE_CACHE

#include <string.h>

class RenderStateCache
{
private:
	static struct RenderState{
		uint32_t texture_id[16];

		uint32_t fragmentShaderId;
		uint32_t vertexShaderId;
		uint32_t padding[2];
	} m_rs __attribute__ ((aligned(16)));
public:

	enum{
		CACHE_HIT = 0,
		CACHE_MISS = 1 // need update
	};

	inline static uint32_t checkFragmentShader(uint32_t id){
#ifdef ENABLE_RENDER_STATE_CACHE
		return (m_rs.fragmentShaderId == id) ? CACHE_HIT : CACHE_MISS;
#else
		return CACHE_MISS;
#endif // ENABLE_RENDER_STATE_CACHE
	}

	inline static uint32_t setFragmentShader(uint32_t id){
#ifdef ENABLE_RENDER_STATE_CACHE
		uint32_t ret = checkFragmentShader(id);
		m_rs.fragmentShaderId = (ret == CACHE_HIT) ? m_rs.fragmentShaderId : id;
		return ret;
#else
		(void)id;
		return CACHE_MISS;
#endif // ENABLE_RENDER_STATE_CACHE
	}

	inline static uint32_t checkVertexShader(uint32_t id){
#ifdef ENABLE_RENDER_STATE_CACHE
		return (m_rs.fragmentShaderId == id) ? CACHE_HIT : CACHE_MISS;
#else
		return CACHE_MISS;
#endif // ENABLE_RENDER_STATE_CACHE
	}

	inline static uint32_t setVertexShader(uint32_t id){
#ifdef ENABLE_RENDER_STATE_CACHE
		uint32_t ret = checkVertexShader(id);
		m_rs.fragmentShaderId = (ret == CACHE_HIT) ? m_rs.fragmentShaderId : id;
		return ret;
#else
		(void)id;
		return CACHE_MISS;
#endif // ENABLE_RENDER_STATE_CACHE
	}

	inline static uint32_t checkTexture(uint32_t index, uint32_t id){
#ifdef ENABLE_RENDER_STATE_CACHE
		return (m_rs.texture_id[index] == id) ? CACHE_HIT : CACHE_MISS;
#else
		return CACHE_MISS;
#endif // ENABLE_RENDER_STATE_CACHE
	}

	inline static uint32_t setTexture(uint32_t index, uint32_t id){
		uint32_t ret = checkTexture(index, id);
#ifdef ENABLE_RENDER_STATE_CACHE
		m_rs.texture_id[index] = (ret == CACHE_HIT) ? m_rs.texture_id[index] : id;
		return ret;
#else
		return CACHE_MISS;
#endif // ENABLE_RENDER_STATE_CACHE

	}

	inline static void reset(){
		memset(&m_rs, 0x0, sizeof(m_rs));
	}

};

#undef ENABLE_RENDER_STATE_CACHE
#endif // SPURS_TASK_RENDER_RENDER_STATE_CACHE