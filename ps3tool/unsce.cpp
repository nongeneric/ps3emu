#include "ps3tool.h"
#include "ps3emu/ELFLoader.h"
#include "ps3emu/utils.h"
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <zlib.h>
#include <filesystem>
#include <regex>
#include <optional>
#include <boost/algorithm/string.hpp>

#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <algorithm>
#include <numeric>

using namespace boost::endian;
using namespace std::filesystem;
using namespace boost;

#pragma pack(1)

struct sce_header_t {
    char magic[4];
    big_uint32_t version;
    big_uint16_t revision;
    big_uint16_t type;
    big_uint32_t metadata_offset;
    big_uint64_t header_size;
    big_uint64_t data_size;
};

static_assert(sizeof(sce_header_t) == 32, "");

struct self_header_t {
    big_uint64_t header_type;
    big_uint64_t appinfo_offset;
    big_uint64_t elf_offset;
    big_uint64_t program_header_offset;
    big_uint64_t section_header_offset;
    big_uint64_t section_info_offset;
    big_uint64_t sce_version_offset;
    big_uint64_t control_info_offset;
    big_uint64_t control_info_length;
    big_uint64_t padding;
};

static_assert(sizeof(self_header_t) == 80, "");

struct metadata_info_t {
    uint8_t key[16];
    uint8_t key_pad[16];
    uint8_t iv[16];
    uint8_t iv_pad[16];
};

static_assert(sizeof(metadata_info_t) == 64, "");

struct app_info_t {
    big_uint64_t auth_id;
    big_uint32_t vendor_id;
    big_uint32_t self_type;
    big_uint64_t version;
    big_uint64_t padding;
};

static_assert(sizeof(app_info_t) == 32, "");

struct metadata_header_t {
    big_uint64_t sig_input_length;
    big_uint32_t unk0;
    big_uint32_t section_count;
    big_uint32_t key_count;
    big_uint32_t opt_header_size;
    big_uint32_t unk1;
    big_uint32_t unk2;
};

static_assert(sizeof(metadata_header_t) == 32, "");

struct self_section_info_t {
    big_uint64_t offset;
    big_uint64_t size;
    big_uint32_t compressed;
    big_uint32_t unknown_0;
    big_uint32_t unknown_1;
    big_uint32_t encrypted;
};

static_assert(sizeof(self_section_info_t) == 32, "");

struct sce_version_t {
    big_uint32_t header_type;
    big_uint32_t sce_version_section;
    big_uint32_t size;
    big_uint32_t unk0;
};

static_assert(sizeof(sce_version_t) == 16, "");

struct metadata_section_header_t {
    big_uint64_t data_offset;
    big_uint64_t data_size;
    big_uint32_t type;
    big_uint32_t index;
    big_uint32_t hashed;
    big_uint32_t sha1_index;
    big_uint32_t encrypted;
    big_uint32_t key_index;
    big_uint32_t iv_index;
    big_uint32_t compressed;
};

static_assert(sizeof(metadata_section_header_t) == 48, "");

struct control_info_t {
    big_uint32_t type;
    big_uint32_t size;
    big_uint64_t next;
};

static_assert(sizeof(control_info_t) == 16, "");

struct ci_data_npdrm {
    big_uint32_t magic;
    big_uint32_t unknown_0;
    big_uint32_t license_type;
    big_uint32_t app_type;
    uint8_t content_id[0x30];
    uint8_t rndpad[0x10];
    uint8_t hash_cid_fname[0x10];
    uint8_t hash_ci[0x10];
    big_uint64_t unknown_1;
    big_uint64_t unknown_2;
};

static_assert(sizeof(ci_data_npdrm) == 128, "");

#pragma pack()

#define SELF_TYPE_LV0 1
#define SELF_TYPE_LV1 2
#define SELF_TYPE_LV2 3
#define SELF_TYPE_APP 4
#define SELF_TYPE_ISO 5
#define SELF_TYPE_LDR 6
#define SELF_TYPE_NPDRM 8

struct key_info_t {
    std::string name;
    std::string type;
    unsigned revision = 0;
    uint64_t version = 0;
    std::string self_type;
    std::vector<uint8_t> key;
    std::vector<uint8_t> erk;
    std::vector<uint8_t> riv;
    std::vector<uint8_t> pub;
    std::vector<uint8_t> priv;
    unsigned ctype = 0;
};

std::vector<uint8_t> parse_hex_str(std::string const& hex) {
    std::vector<uint8_t> res;
    for (auto i = 0u; i < hex.size(); i += 2) {
        auto byte = std::stoul(std::string(&hex[i], &hex[i] + 2), nullptr, 16);
        res.push_back(byte);
    }
    return res;
}

std::vector<key_info_t> read_keys(path data_path) {
    auto keys_path = data_path / "keys";
    std::ifstream f(keys_path.string());
    if (!f.is_open()) {
        std::cout << "can't read the keys file";
        return {};
    }

    std::regex rx_header("\\[(.*?)\\]");
    std::regex rx_pair("(.*?)=(.*)");

    std::vector<key_info_t> keys;
    key_info_t key;
    for (std::string line; std::getline(f, line); ) {
        std::smatch m;
        if (regex_match(line, m, rx_header)) {
            keys.push_back(key);
            key = key_info_t();
            key.name = m.str(1);
        } else if (regex_match(line, m, rx_pair)) {
            auto left = m.str(1);
            auto right = trim_copy(m.str(2));
            if (right.empty())
                continue;
            if (left == "type") {
                key.type = right;
            } else if (left == "revision") {
                key.revision = std::stoul(right, nullptr, 16);
            } else if (left == "version") {
                key.version = std::stoull(right, nullptr, 16);
            } else if (left == "self_type") {
                key.self_type = right;
            } else if (left == "erk") {
                key.erk = parse_hex_str(right);
            } else if (left == "riv") {
                key.riv = parse_hex_str(right);
            } else if (left == "pub") {
                key.pub = parse_hex_str(right);
            } else if (left == "priv") {
                key.priv = parse_hex_str(right);
            } else if (left == "ctype") {
                key.ctype = std::stoull(right);
            } else if (left == "key") {
                key.key = parse_hex_str(right);
            }
        }
    }
    return keys;
}

const char* get_self_type(uint32_t self_type) {
    switch(self_type) {
        case SELF_TYPE_LDR: return "LDR";
        case SELF_TYPE_LV0: return "LV0";
        case SELF_TYPE_LV1: return "LV1";
        case SELF_TYPE_LV2: return "LV2";
        case SELF_TYPE_NPDRM: return "NPDRM";
        case SELF_TYPE_APP: return "APP";
        case SELF_TYPE_ISO: return "ISO";
        default: return nullptr;
    }
}

optional<key_info_t> search_self_key(std::vector<key_info_t>& keys,
                                     uint32_t type,
                                     uint16_t revision,
                                     uint64_t version)
{
    auto it = std::find_if(keys.begin(), keys.end(), [=](auto key) {
        if (key.self_type != get_self_type(type))
            return false;
        switch(type) {
            case SELF_TYPE_LDR:
            case SELF_TYPE_LV0: return true;
            case SELF_TYPE_LV1:
            case SELF_TYPE_LV2: return version <= key.version;
            case SELF_TYPE_NPDRM:
            case SELF_TYPE_APP: return revision == key.revision;
            case SELF_TYPE_ISO: return version <= key.version && revision == key.revision;
            default: return false;
        }
    });
    if (it == keys.end())
        return none;
    return *it;
}

#define SCE_HEADER_TYPE_SELF 1

#define CONTROL_INFO_TYPE_FLAGS 1
#define CONTROL_INFO_TYPE_DIGEST 2
#define CONTROL_INFO_TYPE_NPDRM 3

#define NP_LICENSE_NETWORK 1
#define NP_LICENSE_LOCAL 2
#define NP_LICENSE_FREE 3

bool decrypt_npdrm(std::vector<key_info_t>& keys,
                   std::vector<control_info_t*> control_infos,
                   metadata_info_t* metadata_info) {
    auto klic_key = std::find_if(keys.begin(), keys.end(), [=](auto& key) {
        return key.name == "NP_klic_key";
    });
    assert(klic_key != keys.end());

    auto it = std::find_if(control_infos.begin(), control_infos.end(), [=](auto ci) {
        return ci->type == CONTROL_INFO_TYPE_NPDRM;
    });
    assert(it != control_infos.end());

    auto npdrm = reinterpret_cast<ci_data_npdrm*>(*it + 1);
    (void)npdrm;
    assert(npdrm->license_type == NP_LICENSE_FREE);
    auto key = std::find_if(keys.begin(), keys.end(), [=](auto& key) {
        return key.name == "NP_klic_free";
    });
    assert(key != keys.end());
    uint8_t npdrm_key[0x10];
    assert(key->key.size() == sizeof(npdrm_key));
    memcpy(npdrm_key, &key->key[0], sizeof(npdrm_key));

    AES_KEY aes_key;
    assert(klic_key->key.size() == 16);
    AES_set_decrypt_key(&klic_key->key[0], 128, &aes_key);
    AES_ecb_encrypt(npdrm_key, npdrm_key, &aes_key, AES_DECRYPT);

    uint8_t npdrm_iv[0x10] = { 0 };
    AES_set_decrypt_key(npdrm_key, 128, &aes_key);
    AES_cbc_encrypt(reinterpret_cast<unsigned char*>(metadata_info),
                    reinterpret_cast<unsigned char*>(metadata_info),
                    sizeof(metadata_info_t),
                    &aes_key,
                    npdrm_iv,
                    AES_DECRYPT);
    return true;
}

#define METADATA_SECTION_ENCRYPTED 3
#define METADATA_SECTION_COMPRESSED 2

bool decrypt_metadata(std::vector<key_info_t>& keys,
                      app_info_t* app_info,
                      sce_header_t* sce_header,
                      metadata_info_t* metadata_info,
                      metadata_header_t* metadata_header,
                      metadata_section_header_t* metadata_section_headers,
                      std::vector<control_info_t*> const& control_infos) {
    auto opt_key = search_self_key(keys, app_info->self_type, sce_header->revision, app_info->version);
    if (!opt_key) {
        std::cout << "can't find a suitable key";
        return false;
    }

    auto key = opt_key.get();
    std::cout << "key to decrypt metadata\n"
        << sformat("  name       {}\n", key.name)
        << sformat("  self_type  {}\n", key.self_type)
        << sformat("  revision   {:x}\n", key.revision)
        << sformat("  version    {:x}\n", key.version);

    assert(sce_header->type == SCE_HEADER_TYPE_SELF);
    if (app_info->self_type == SELF_TYPE_NPDRM) {
        decrypt_npdrm(keys, control_infos, metadata_info);
    }

    AES_KEY aes_key;
    AES_set_decrypt_key(&key.erk[0], key.erk.size() * 8, &aes_key);
    AES_cbc_encrypt(reinterpret_cast<unsigned char*>(metadata_info),
                    reinterpret_cast<unsigned char*>(metadata_info),
                    sizeof(metadata_info_t),
                    &aes_key,
                    &key.riv[0],
                    AES_DECRYPT);

    if (std::accumulate(metadata_info->iv_pad, metadata_info->iv_pad + 16, 0) != 0 ||
        std::accumulate(metadata_info->key_pad, metadata_info->key_pad + 16, 0) != 0) {
        std::cout << "metadata decryption failed, check your keys";
        return false;
    }

    std::cout << "metadata info\n"
        << sformat("  key  {}\n", print_hex(metadata_info->key, 16))
        << sformat("  iv   {}\n", print_hex(metadata_info->iv, 16));

    int outl;
    std::vector<uint8_t> tmp(sizeof(metadata_header_t) + 15);
    auto ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, metadata_info->key, metadata_info->iv);
    EVP_EncryptUpdate(ctx,
                      &tmp[0],
                      &outl,
                      reinterpret_cast<unsigned char*>(metadata_header),
                      sizeof(metadata_header_t));
    memcpy(metadata_header, &tmp[0], sizeof(metadata_header_t));

    std::cout << "metadata header\n"
        << sformat("  section_count  {}\n", metadata_header->section_count)
        << sformat("  key_count      {}\n", metadata_header->key_count);

    auto metadata_size = sizeof(metadata_section_header_t) * metadata_header->section_count +
                         16 * metadata_header->key_count;
    tmp.resize(metadata_size + 15);
    EVP_EncryptUpdate(ctx,
                      &tmp[0],
                      &outl,
                      reinterpret_cast<unsigned char*>(metadata_section_headers),
                      metadata_size);
    EVP_EncryptFinal_ex(ctx, &tmp[outl], &outl);
    EVP_CIPHER_CTX_free(ctx);
    memcpy(metadata_section_headers, &tmp[0], metadata_size);

    std::cout << "metadata section headers\n"
        << "  Idx Offset   Size     Type Index Hashed SHA1 Encrypted Key IV Compressed\n";
    for (auto i = 0u; i < metadata_header->section_count; ++i) {
        auto sh = &metadata_section_headers[i];
        std::cout <<
            sformat("  {:02}  {:08X} {:08X} {:02X}   {:02}    {}    {:02X}   {}       {}  {} {}\n",
                i, sh->data_offset, sh->data_size, sh->type, sh->index,
                sh->hashed == 2 ? "YES" : "NO ",
                sh->sha1_index,
                sh->encrypted == METADATA_SECTION_ENCRYPTED ? "YES" : "NO ",
                sh->key_index == 0xffffffff ? "--" : sformat("{:02X}", sh->key_index),
                sh->iv_index == 0xffffffff ? "--" : sformat("{:02X}", sh->iv_index),
                sh->compressed == METADATA_SECTION_COMPRESSED ? "YES" : "NO "
            );
    }
    std::cout << "";

    return true;
}

#define METADATA_SECTION_TYPE_SHDR 1
#define METADATA_SECTION_TYPE_PHDR 2
#define METADATA_SECTION_TYPE_UNK3 3

bool decrypt_sections(sce_header_t* sce_header,
                      metadata_header_t* metadata_header,
                      metadata_section_header_t* metadata_section_headers) {
    auto keys = reinterpret_cast<std::array<uint8_t, 16>*>(
        metadata_section_headers + metadata_header->section_count);

    std::cout << "sce keys\n";
    for (auto i = 0u; i < metadata_header->key_count; ++i) {
        std::cout << sformat("  {:02x}  {}\n", i, print_hex(&keys[i][0], 16));
    }

    for (auto sh = metadata_section_headers;
         sh != metadata_section_headers + metadata_header->section_count;
         ++sh) {
        if (sh->encrypted != METADATA_SECTION_ENCRYPTED)
            continue;
        if (sh->key_index >= metadata_header->key_count ||
            sh->iv_index >= metadata_header->key_count) {
            std::cout << "a section marked encrypted references a non existent key";
            continue;
        }

        auto key_ptr = &keys[sh->key_index][0];
        auto iv_ptr = &keys[sh->iv_index][0];

        std::cout << "decoding section\n"
            << sformat("  offset  {:08X}\n", sh->data_offset)
            << sformat("  size    {:08X}\n", sh->data_size)
            << sformat("  key     {}\n", print_hex(key_ptr, 16))
            << sformat("  iv      {}\n", print_hex(iv_ptr, 16));

        auto data = reinterpret_cast<uint8_t*>(sce_header) + sh->data_offset;

        int outl;
        auto ctx = EVP_CIPHER_CTX_new();
        std::vector<uint8_t> tmp(sh->data_size + 15);
        EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key_ptr, iv_ptr);
        EVP_EncryptUpdate(ctx, &tmp[0], &outl, data, sh->data_size);
        EVP_EncryptFinal_ex(ctx, &tmp[outl], &outl);
        memcpy(data, &tmp[0], sh->data_size);
    }
    return true;
}

unsigned inflate(uint8_t* in, unsigned in_len, uint8_t* out, unsigned out_len) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = in_len;
    strm.next_in = in;

    if (inflateInit(&strm) != Z_OK)
        throw std::runtime_error("zlib init failed");

    strm.avail_out = out_len;
    strm.next_out = out;

    auto err = inflate(&strm, Z_FINISH);
    if (err != Z_STREAM_END)
        throw std::runtime_error("inflate failed: ");

    inflateEnd(&strm);

    return strm.total_out;
}

bool write_elf(std::string elf_path,
               std::vector<char>& file_buf,
               sce_header_t* sce_header,
               self_header_t* self_header,
               metadata_header_t* metadata_header,
               metadata_section_header_t* metadata_section_headers) {
    std::ofstream elf_file(elf_path);
    if (!elf_file.is_open()) {
        std::cout << "can't open elf file for writing";
        return false;
    }

    auto keys = reinterpret_cast<std::array<uint8_t, 16>*>(
        metadata_section_headers + metadata_header->section_count);
    auto phs = reinterpret_cast<Elf64_be_Phdr*>(&file_buf[0] + self_header->program_header_offset);

    for (auto i = 0u; i < metadata_header->section_count; ++i) {
        auto msh = metadata_section_headers + i;
        if (msh->type != METADATA_SECTION_TYPE_PHDR)
            continue;

        auto ph = phs + msh->index;
        auto dest = ph->p_offset;
        auto src = msh->data_offset;

        char* src_ptr = &file_buf[0] + src;
        auto src_size = msh->data_size;

        unsigned mdlen;
        auto md = HMAC(EVP_sha1(), &keys[msh->sha1_index + 2], 0x40,
                       (uint8_t*)src_ptr, src_size, NULL, &mdlen);
        if (memcmp(md, &keys[msh->sha1_index], mdlen)) {
            throw std::runtime_error("section sha1 mismatch");
        }

        std::vector<uint8_t> vec(std::max(ph->p_filesz, msh->data_size));
        if (msh->compressed == METADATA_SECTION_COMPRESSED) {
            std::cout << "inflating\n";
            src_size = inflate(reinterpret_cast<uint8_t*>(src_ptr),
                               msh->data_size, &vec[0], vec.size());
            src_ptr = reinterpret_cast<char*>(&vec[0]);
        }

        std::cout << "writing self section to elf\n"
            << sformat("  self source  {:08X}\n", src)
            << sformat("  elf dest     {:08X}\n", dest)
            << sformat("  size         {:08X}\n", src_size);

        elf_file.seekp((uint32_t)dest);
        elf_file.write(src_ptr, src_size);
    }

    auto elf_header = reinterpret_cast<Elf64_be_Ehdr*>(&file_buf[0] + self_header->elf_offset);
    auto shs = &file_buf[0] + self_header->section_header_offset;
    elf_file.seekp(0);
    elf_file.write((char*)elf_header, sizeof(Elf64_be_Ehdr));
    elf_file.seekp((uint32_t)elf_header->e_phoff);
    elf_file.write((char*)phs, sizeof(Elf64_be_Phdr) * elf_header->e_phnum);
    elf_file.seekp((uint32_t)elf_header->e_shoff);
    elf_file.write(shs, sizeof(Elf64_be_Shdr) * elf_header->e_shnum);

    return true;
}

void HandleUnsce(UnsceCommand const& command) {
    auto keys = read_keys(command.data);

    std::ifstream f(command.sce);
    if (!f.is_open()) {
        throw std::runtime_error("can't read input file");
    }
    f.seekg(0, std::ios_base::end);
    auto file_size = f.tellg();
    std::vector<char> file_buf(file_size);
    f.seekg(0, std::ios_base::beg);
    f.read(file_buf.data(), file_buf.size());

    auto sce_header = reinterpret_cast<sce_header_t*>(&file_buf[0]);
    auto self_header = reinterpret_cast<self_header_t*>(sce_header + 1);
    auto app_info = reinterpret_cast<app_info_t*>(&file_buf[0] + self_header->appinfo_offset);
    auto metadata_info = reinterpret_cast<metadata_info_t*>(
        &file_buf[0] + sizeof(sce_header_t) + sce_header->metadata_offset);
    auto metadata_header = reinterpret_cast<metadata_header_t*>(metadata_info + 1);
    auto metadata_section_headers = reinterpret_cast<metadata_section_header_t*>(metadata_header + 1);

    std::vector<control_info_t*> control_infos;
    auto ptr = &file_buf[0] + self_header->control_info_offset;
    for (;;) {
        auto control_info = reinterpret_cast<control_info_t*>(ptr);
        ptr += control_info->size;
        control_infos.push_back(control_info);
        if (!control_info->next)
            break;
    }

    if (std::string("SCE") != sce_header->magic) {
        throw std::runtime_error("not an sce file");
    }

    std::cout << "sce header\n"
        << sformat("  magic:            {}\n", sce_header->magic)
        << sformat("  version:          {:<x}\n", sce_header->version)
        << sformat("  revision:         {:<x}\n", sce_header->revision)
        << sformat("  type:             {:<x}\n", sce_header->type)
        << sformat("  metadata_offset:  {:<x}\n", sce_header->metadata_offset)
        << sformat("  header_size:      {:<x}\n", sce_header->header_size)
        << sformat("  data_size:        {:<x}\n", sce_header->data_size);

    std::cout << "self header\n"
        << sformat("  header_type:            {:<x}\n", self_header->header_type)
        << sformat("  appinfo_offset:         {:<x}\n", self_header->appinfo_offset)
        << sformat("  elf_offset:             {:<x}\n", self_header->elf_offset)
        << sformat("  program_header_offset:  {:<x}\n", self_header->program_header_offset)
        << sformat("  section_header_offset:  {:<x}\n", self_header->section_header_offset)
        << sformat("  section_info_offset:    {:<x}\n", self_header->section_info_offset)
        << sformat("  sce_version_offset:     {:<x}\n", self_header->sce_version_offset)
        << sformat("  control_info_offset:    {:<x}\n", self_header->control_info_offset)
        << sformat("  control_info_length:    {:<x}\n", self_header->control_info_length);

    if (!decrypt_metadata(keys,
                          app_info,
                          sce_header,
                          metadata_info,
                          metadata_header,
                          metadata_section_headers,
                          control_infos))
        throw std::runtime_error("can't decrypt metadata");

    if (!decrypt_sections(sce_header,
                          metadata_header,
                          metadata_section_headers))
        throw std::runtime_error("can't decrypt sections");

    if (!write_elf(command.elf, file_buf, sce_header, self_header,
                   metadata_header, metadata_section_headers))
        throw std::runtime_error("can't write elf");
}
