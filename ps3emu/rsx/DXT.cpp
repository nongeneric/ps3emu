#include "DXT.h"

#include "nvimage/BlockDXT.h"
#include "nvimage/ColorBlock.h"

template <typename BlockType>
void decodeDXT(const uint8_t* block, glm::vec4* data16) {
    auto dxt = reinterpret_cast<const BlockType*>(block);
    nv::ColorBlock color;
    dxt->decodeBlock(&color);
    auto c = color.colors();
    for (int i = 0; i < 16; ++i) {
        data16[i] = {(float)c->r / 255.f,
                     (float)c->g / 255.f,
                     (float)c->b / 255.f,
                     (float)c->a / 255.f};
        c++;
    }
}

void decodeDXT1(const uint8_t* block, glm::vec4* data16) {
    decodeDXT<nv::BlockDXT1>(block, data16);
}

void decodeDXT23(const uint8_t* block, glm::vec4* data16) {
    decodeDXT<nv::BlockDXT3>(block, data16);
}

void decodeDXT45(const uint8_t* block, glm::vec4* data16) {
    decodeDXT<nv::BlockDXT5>(block, data16);
}
