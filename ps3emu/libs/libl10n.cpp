#include "libl10n.h"

#include "../IDMap.h"
#include "../ppu/PPUThread.h"
#include "../MainMemory.h"
#include "../state.h"

#include <iconv.h>
#include <errno.h>

namespace {

const char* l10n_to_iconv_map[] = {
    "UTF8", // L10N_UTF8
    "UTF16", // L10N_UTF16
    "UTF32", // L10N_UTF32
    "UCS-2", // L10N_UCS2
    "UCS-4", // L10N_UCS4
    "ISO-8859-1", // L10N_ISO_8859_1
    "ISO-8859-2", // L10N_ISO_8859_2
    "ISO-8859-3", // L10N_ISO_8859_3
    "ISO-8859-4", // L10N_ISO_8859_4
    "ISO-8859-5", // L10N_ISO_8859_5
    "ISO-8859-6", // L10N_ISO_8859_6
    "ISO-8859-7", // L10N_ISO_8859_7
    "ISO-8859-8", // L10N_ISO_8859_8
    "ISO-8859-9", // L10N_ISO_8859_9
    "ISO-8859-10", // L10N_ISO_8859_10
    "ISO-8859-11", // L10N_ISO_8859_11
    "ISO-8859-13", // L10N_ISO_8859_13
    "ISO-8859-14", // L10N_ISO_8859_14
    "ISO-8859-15", // L10N_ISO_8859_15
    "ISO-8859-16", // L10N_ISO_8859_16
    "CP437", // L10N_CODEPAGE_437
    "CP850", // L10N_CODEPAGE_850
    "CP863", // L10N_CODEPAGE_863
    "CP866", // L10N_CODEPAGE_866
    "CP932", // L10N_CODEPAGE_932
    "CP936", // L10N_CODEPAGE_936
    "CP949", // L10N_CODEPAGE_949
    "CP950", // L10N_CODEPAGE_950
    "CP1251", // L10N_CODEPAGE_1251
    "CP1252", // L10N_CODEPAGE_1252
    "EUC-CN", // L10N_EUC_CN
    "EUC-JP", // L10N_EUC_JP
    "EUC-KR", // L10N_EUC_KR
    "ISO-2022-JP", // L10N_ISO_2022_JP
    "", // L10N_ARIB
    "", // L10N_HZ
    "GB18030", // L10N_GB18030
    "", // L10N_RIS_506
    "CP852", // L10N_CODEPAGE_852
    "CP1250", // L10N_CODEPAGE_1250
    "CP737", // L10N_CODEPAGE_737
    "CP1253", // L10N_CODEPAGE_1253
    "CP857", // L10N_CODEPAGE_857
    "CP1254", // L10N_CODEPAGE_1254
    "CP775", // L10N_CODEPAGE_775
    "CP1257", // L10N_CODEPAGE_1257
    "CP855", // L10N_CODEPAGE_855
    "CP858", // L10N_CODEPAGE_858
    "CP860", // L10N_CODEPAGE_860
    "CP861", // L10N_CODEPAGE_861
    "CP865", // L10N_CODEPAGE_865
    "CP869", // L10N_CODEPAGE_869
    "", // _L10N_CODE_
};

constexpr auto map_size = sizeof(l10n_to_iconv_map) / sizeof(const char*);

typedef enum {
    ConversionOK,
    SRCIllegal,
    DSTExhausted,
    ConverterUnknown
} L10nResult;

ThreadSafeIDMap<l10n_conv_t, iconv_t> converters;

}

l10n_conv_t l10n_get_converter(uint32_t src_code, uint32_t dst_code) {
    assert(src_code < map_size && dst_code < map_size);
    auto src = l10n_to_iconv_map[src_code];
    auto dst = l10n_to_iconv_map[dst_code];
    assert(src[0] && dst[0]);
    auto converter = iconv_open(src, dst);
    assert(converter != (iconv_t)-1);
    return converters.create(converter);
}

uint32_t l10n_convert_str(l10n_conv_t cd,
                          ps3_uintptr_t src,
                          big_uint32_t* src_len,
                          ps3_uintptr_t dst,
                          big_uint32_t* dst_len,
                          PPUThread* thread) {
    assert(src && dst);
    std::vector<char> src_buf(*src_len);
    std::vector<char> dest_buf(*dst_len);
    g_state.mm->readMemory(src, &src_buf[0], src_buf.size());
    auto converter = converters.get(cd);
    auto src_ptr = &src_buf[0];
    auto dest_ptr = &dest_buf[0];
    size_t src_bytes, dest_bytes;
    auto res = iconv(converter, &src_ptr, &src_bytes, &dest_ptr, &dest_bytes);
    if (res == -1u) {
        if (errno == EILSEQ)
            return SRCIllegal;
        assert(false);
    }
    *src_len = src_bytes;
    *dst_len = dest_bytes;
    return ConversionOK;
}
