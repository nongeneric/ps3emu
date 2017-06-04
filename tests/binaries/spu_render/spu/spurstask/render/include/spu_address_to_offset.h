#ifndef SPURS_TASK_RENDER_SPU_ADDRESS_TO_OFFSET
#define SPURS_TASK_RENDER_SPU_ADDRESS_TO_OFFSET
#include "base.h"

namespace SpuAddressToOffset{
	void		load(uint32_t ea_addressToOffsetInitContext);
	uint32_t	getOffset(spu_addr addr);
}; // 

#endif // SPURS_TASK_RENDER_SPU_ADDRESS_TO_OFFSET