import os
import re

def join_mc_mo(mc, mo):
    ms = []
    for mc_m, i in mc:
        for mo_m, c in mo:
            if mc_m == mo_m and not (mc_m in ms):
                ms += [mc_m]
                yield (mc_m, i, c)

host_gcm_path = os.environ['PS3_HOST_INCLUDE']
methods_text = open(host_gcm_path + '/cell/gcm/gcm_methods.h').read()
macros_text = open(host_gcm_path + '/cell/gcm/gcm_macros.h').read()
mc = re.findall(r'CELL_GCM_METHOD\(([_A-Z0-9]+).*?, ([0-9]+)', methods_text)
mc += [ 
    ('CELL_GCM_NV4097_SET_OBJECT', 1),
    ('CELL_GCM_NV0039_SET_OBJECT', 1),
    ('CELL_GCM_NV3062_SET_OBJECT', 1),
    ('CELL_GCM_NV309E_SET_OBJECT', 1),
    ('CELL_GCM_NV308A_SET_OBJECT', 1),
    ('CELL_GCM_NV3089_SET_OBJECT', 1),
    ('CELL_GCM_NV4097_SET_SURFACE_COMPRESSION', 1),
    ('CELL_GCM_NV4097_SET_CLEAR_RECT_HORIZONTAL', 1),
    ('CELL_GCM_NV4097_SET_CLIP_ID_TEST_ENABLE', 0)
]
mo = re.findall(r'#define.*?([_A-Z0-9]+).*?(0x[0-9a-fA-F]+)', macros_text)
print('struct RsxMethods {')
for m, o in mo:
    print('    static constexpr RsxMethodInfo {} {{ {}, "{}" }};'
        .format(m[len('CELL_GCM_'):], o, m))
print('};')

print('    switch (offset) {')
for m, o in mo:
    if m in ['CELL_GCM_IO_PAGE_SIZE', 'NOTIFY_IO_ADDRESS_SIZE', 'CELL_GCM_NV4097_SET_TEXTURE_IMAGE_RECT']:
        continue
    #m = 'RsxMethods::' + m[len('CELL_GCM_'):]
    print('        case {}:'.format(o))
    print('            name = "{}";'.format(m))
    #print('            len += 4 * {}.count;'.format(m))
    print('            break;');
print('        default: BOOST_LOG_TRIVIAL(fatal) << "illegal method offset " << offset;')
print('    }')
