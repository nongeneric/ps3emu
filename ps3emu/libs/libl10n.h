#pragma once

#include "sys.h"

typedef int32_t l10n_conv_t;

class PPUThread;

l10n_conv_t l10n_get_converter(uint32_t src_code, uint32_t dst_code);
uint32_t l10n_convert_str(l10n_conv_t cd,
                          ps3_uintptr_t src,
                          big_uint32_t* src_len,
                          ps3_uintptr_t dst,
                          big_uint32_t* dst_len,
                          PPUThread* thread);