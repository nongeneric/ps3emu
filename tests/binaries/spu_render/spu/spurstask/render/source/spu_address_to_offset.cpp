#include <cell/dma.h>
#include "spu_render_common.h"
#include "spu_address_to_offset.h"
#include "spu_render_tag.h"
#include <spu_printf.h>

namespace SpuAddressToOffset{
	static SpuAddressToOffsetInitData data;
}

void SpuAddressToOffset::load(uint32_t ea_addressToOffsetInitContext){
	cellDmaLargeGet(&data,ea_addressToOffsetInitContext,sizeof(SpuAddressToOffsetInitData),TAG_FOR_INPUT_DATA,0,0);
	//spu_printf("SpuAddressToOffset::load(%x)\n", ea_addressToOffsetInitContext);
}

uint32_t SpuAddressToOffset::getOffset(spu_addr addr){
	uint32_t ret = data.AddressToOffset(addr);
	MY_ASSERT(ret != SpuAddressToOffsetInitData::SPU_ADDRESS_TO_OFFSET_INVALID);
	return ret;
}
